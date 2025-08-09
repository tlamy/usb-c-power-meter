#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <INA228.h>
#define INA_I2C_ADDRESS 0x41
#define RELEASE_VERSION "2.0.0"

#define DEBUG_INA 0

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
INA228 ina228 = INA228(INA_I2C_ADDRESS, &Wire);

void splash() {
    char buf[64];
    u8g2.firstPage();
    u8g2.setFont(u8g2_font_profont29_tr);
    sprintf(buf, "MacWake");
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2,
                 (u8g2.getDisplayHeight() / 2) + u8g2.getFontAscent() / 2, buf);
    u8g2.setFont(u8g2_font_profont12_tf);
    sprintf(buf, "USB Power Meter");
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2,
                 (u8g2.getDisplayHeight() / 2) + u8g2.getFontAscent() / 2 + 16, buf);
    u8g2.setFont(u8g2_font_profont10_tf);
    sprintf(buf, "V%s", RELEASE_VERSION);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2, 62, buf);
    u8g2.nextPage();
    delay(1500);
}

void scan_i2c() {
    uint8_t error, address;
    int nDevices;
    Serial.println("Scanning...");
    nDevices = 0;
    for (address = 1; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Serial.printf("Connect to 0x%02x", address);
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.println(" FOUND");
            nDevices++;
        } else {
            //Serial.println(" nope.");
        }
    }
    Serial.printf("Found %d devices.\n", nDevices);
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.printf("MacWake USB-Power V%s\n", RELEASE_VERSION);

    u8g2.begin();

    if (!Wire.begin(SDA_PIN,SCL_PIN)) {
        Serial.println("Failed to initialize i2c!");
    } else {
        Serial.println("i2c init done.");
    }
    if (!ina228.begin()) {
        Serial.println("Failed to initialize INA228!");
        scan_i2c();
        sleep(2);
        esp_cpu_reset(0);
    } else {
        Serial.println("INA228 init done.");
    }
    ina228.setADCRange(false);
    ina228.setMaxCurrentShunt(10.0, 10e-3);
    sleep(30);
    ina228.setAverage(INA228_16_SAMPLES);
    ina228.setBusVoltageConversionTime(INA228_50_us);
    ina228.setShuntVoltageConversionTime(INA228_1052_us);
    ina228.setTemperatureConversionTime(INA228_50_us);
    ina228.setMode(INA228_MODE_CONT_BUS_SHUNT);
    Serial.println("showing splash");
    splash();
}

char hexdigit(uint8_t nibble) {
    return nibble < 10 ? nibble + '0' : nibble - 10 + 'A';
}

void int2hex(int16_t val, char *buf) {
    buf[0] = hexdigit((highByte(val) >> 4) & 0x0f);
    buf[1] = hexdigit((highByte(val)) & 0x0f);
    buf[2] = hexdigit((lowByte(val) >> 4) & 0x0f);
    buf[3] = hexdigit((lowByte(val)) & 0x0f);
}

void serial_out_pld(int milliamps, int millivolt) {
    float shuntval = -milliamps / 0.2F;
    float voltval = static_cast<float>(millivolt) / 3.125F;
    int16_t shunt_ser = static_cast<int16_t>(shuntval);
    int16_t volt_ser = static_cast<int16_t>(voltval);

    char buf[16];
    int2hex(shunt_ser, buf);
    int2hex(volt_ser, buf + 4);
    buf[8] = 28;
    buf[9] = '\n';
    buf[10] = '\0';
    Serial.print(buf);
}

void read_ina(int *shunt, int *millivolt, int *current) {
    float shuntVoltage_mV = 0.0;
    float busVoltage_V = 0.0;
    float current_mA = 0.0;

    shuntVoltage_mV = ina228.getShuntMilliVolt();
    busVoltage_V = ina228.getBusVolt();
    current_mA = ina228.getCurrent()*1000.0f;
#if DEBUG_INA
    float power_mW = ina228.getPower()*1000.0f;
    //float loadVoltage_V = busVoltage_V + (shuntVoltage_mV / 1000);

    Serial.print("Shunt Voltage [mV]: ");
    Serial.println(shuntVoltage_mV);
    Serial.print("Bus Voltage [V]: ");
    Serial.println(busVoltage_V);
    //Serial.print("Load Voltage [V]: ");
    //Serial.println(loadVoltage_V);
    Serial.print("Current[mA]: ");
    Serial.println(current_mA);
    // Serial.print("Bus Power [mW]: ");
    // Serial.println(power_mW);
    // Serial.print("Charge [C]: ");
    // Serial.println(ina228.getCharge());
    Serial.println();
    //sleep(1);
#endif
    *shunt = static_cast<int>(shuntVoltage_mV);
    *millivolt = static_cast<int>(busVoltage_V * 1000.0);
    *current = static_cast<int>(current_mA);
}

uint8_t normalize_volt(int millivolt) {
    if (millivolt < 1000)
        return 0;
    if (millivolt < 6000)
        return 5;
    if (millivolt < 10000)
        return 9;
    if (millivolt < 16000)
        return 16;
    if (millivolt < 21000)
        return 20;
    if (millivolt < 29000)
        return 28;
    if (millivolt < 37000)
        return 36;
    return 48;
}

#define AMP_BUF_SIZE 32
uint8_t last_volts = 0;
int last_currents[AMP_BUF_SIZE];
uint8_t current_ptr = 0;
int max_current = 0;

int get_max_current(int current, uint8_t volts) {
    if (volts != last_volts) {
        current_ptr = 0;
        for (int i = 0; i < AMP_BUF_SIZE; ++i) {
            last_currents[i] = 0;
        }
        last_volts = volts;
        max_current = 0;
    }
    if (max_current > 0 && last_currents[current_ptr] == max_current) {
        //digitalWrite(MY_BLUE_LED_PIN, LOW); // Turn the LED on (Note that LOW is the voltage level
        last_currents[current_ptr] = current;
        max_current = 0;
        for (int i = 0; i < AMP_BUF_SIZE; ++i) {
            if (last_currents[i] > max_current) max_current = last_currents[i];
        }
    } else {
        last_currents[current_ptr] = current;
        if (current > max_current) {
            max_current = current;
        }
    }
    current_ptr = (current_ptr + 1) % AMP_BUF_SIZE;
    return max_current;
}

void screensaver(int *x, int *y) {
    static int last_x = 1, last_y = 2;
    static signed char x_dir = 3, y_dir = 2;

    *x = last_x + x_dir;
    if (*x > 127) {
        x_dir = -x_dir;
        *x = 126;
    } else if (*x < 0) {
        x_dir = -x_dir;
        *x = 0;
    }
    *y = last_y + y_dir;
    if (*y > 63) {
        y_dir = -y_dir;
        *y = 62;
    } else if (*y < 0) {
        y_dir = -y_dir;
        *y = 0;
    }
    last_x = *x;
    last_y = *y;
}

#define COUNTS 5
void display(int millivolt, uint8_t volt_norm, int current, int maxcurrent) {
    char buf[32];
    char buf2[32];
    static uint8_t counter=0;
    static float currents[COUNTS];
    static float display_current = -1;
    currents[counter] = static_cast<float>(current);
    counter = (counter + 1) % COUNTS;
    if (counter == 0) {
        display_current = 0.0;
        for (float c : currents) {
            if (c > display_current) display_current = c;
        }
    }
    if (display_current == -1) {
        display_current = static_cast<float>(current);
    }

    int bar_max;
        if (maxcurrent < 100) {
            bar_max = 100;
        } else if (maxcurrent< 250) {
            bar_max = 250;
        } else if (maxcurrent < 500) {
            bar_max = 500;
        } else if (maxcurrent < 1000) {
        bar_max = 1000;
    } else {
        bar_max = (1+(maxcurrent / 1000)) * 1000;
    }
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_profont17_tf);
        if (volt_norm == 0) {
            int x, y;
            screensaver(&x, &y);
            u8g2.drawPixel(x, y);
            //u8g2.drawStr(0, 17, "---");
        } else if (volt_norm == 5)
            u8g2.drawStr(10, 17, "5V");
        else if (volt_norm == 9)
            u8g2.drawStr(25, 17, "9V");
        else if (volt_norm == 15)
            u8g2.drawStr(40, 17, "15V");
        else if (volt_norm == 20)
            u8g2.drawStr(55, 17, "20V");
        else if (volt_norm == 28)
            u8g2.drawStr(70, 17, "28V");
        else if (volt_norm == 36)
            u8g2.drawStr(85, 17, "36V");
        else if (volt_norm == 48)
            u8g2.drawStr(100, 17, "48V");

        if (volt_norm > 0) {
            float amps = ((float) display_current / 1000.0f);
            u8g2.setFont(u8g2_font_profont12_tf);
            sprintf(buf, "%0.3fA", ((float) maxcurrent / 1000.0));
            u8g2.drawStr(127 - u8g2.getStrWidth(buf), 32, buf);
            u8g2.drawLine(127, 33, 127, 35);
            int bar = (int) ((float) 128 * (float) ((float) current / (float) bar_max));
            u8g2.drawLine(0, 34, bar, 34);
            bar = (int) ((float) 128 * (float) ((float) display_current / (float) bar_max));
            u8g2.drawLine(bar, 33, bar, 35);

            u8g2.setFont(u8g2_font_profont29_mf);
            sprintf(buf2, "%0.3fA", amps);
            u8g2.drawStr(128 - u8g2.getStrWidth(buf2), 62, buf2);
        }
    } while (u8g2.nextPage());
}

void loop() {
    int millivolt;
    int shunt;
    int current;

    read_ina(&shunt, &millivolt, &current);
    uint8_t volt_norm = normalize_volt(millivolt);
#if !  DEBUG_INA
    serial_out_pld(current, millivolt);
#endif
    int max_current = get_max_current(current, volt_norm);

    display(millivolt, volt_norm, current, max_current);

    //delay(50);
}
