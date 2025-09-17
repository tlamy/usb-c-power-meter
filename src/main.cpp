#include <Arduino.h>
#include <INA226_WE.h>
#include <U8g2lib.h>
#include <Wire.h>

#define MY_BLUE_LED_PIN D4
#define RELEASE_VERSION "1.2.2"

#define DEBUG_LED_PEAK_DETECT 0
#define DEBUG_INA 0
#define SCREENSAVER_DELAY 10000

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
INA226_WE ina226;

void splash() {
  char buf[64];
  u8g2.firstPage();
  u8g2.setFont(u8g2_font_profont29_tr);
  sprintf(buf, "MacWake");
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2,
               (u8g2.getDisplayHeight() / 2) + u8g2.getFontAscent() / 2, buf);
  u8g2.setFont(u8g2_font_profont12_tr);
  sprintf(buf, "USB Power Meter");
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2,
               (u8g2.getDisplayHeight() / 2) + u8g2.getFontAscent() / 2 + 16,
               buf);
  u8g2.setFont(u8g2_font_profont10_tr);
  sprintf(buf, "V%s", RELEASE_VERSION);
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2, 62, buf);
  u8g2.nextPage();
  delay(1500);
}

static void scan_i2c() {
  uint8_t error, address;
  int nDevices;
  Serial.println("Scanning for devices...");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      nDevices++;
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  Serial.println("done\n");
  Serial.print("Found ");
  Serial.print(nDevices);
  Serial.println(" devices.");
}

static uint8_t findInaAddress() {
  uint8_t error, address;

  for (address = 0x40; address < 0x48; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      return address;
    }
  }
  return 0;
}
void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.printf("MacWake USB-Power V%s\n", RELEASE_VERSION);

  pinMode(MY_BLUE_LED_PIN,
          OUTPUT); // Initialize the LED_BUILTIN pin as an output

  u8g2.begin();

  Wire.begin();

  uint8_t ina_address = findInaAddress();
  if (ina_address == 0) {
    Serial.println("INA226 not found");
    scan_i2c();
    delay(10000);
    while (1)
      ; // halt
  }
  ina226 = INA226_WE(&Wire, ina_address);
  if (!ina226.init()) {
    Serial.println("INA226 init failed");
    scan_i2c();
    delay(10000);
    while (1)
      ; // halt
  }
  ina226.setAverage(AVERAGE_16);
  ina226.setConversionTime(CONV_TIME_2116);
  ina226.setMeasureMode(CONTINUOUS);
  ina226.setResistorRange(0.01, 6.0);
  ina226.setCorrectionFactor(0.975); // must be aligned with good load

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

void serial_out(int milliamps, int millivolt) {
  float shuntval = -milliamps / 0.2;
  float voltval = millivolt / 3.125;
  int16_t shunt_ser = shuntval;
  int16_t volt_ser = voltval;

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

  ina226.readAndClearFlags();
  shuntVoltage_mV = ina226.getShuntVoltage_mV();
  busVoltage_V = ina226.getBusVoltage_V();
  current_mA = ina226.getCurrent_mA();
#if DEBUG_INA
  float power_mW = ina226.getBusPower();
  float loadVoltage_V = busVoltage_V + (shuntVoltage_mV / 1000);

  Serial.print("Shunt Voltage [mV]: ");
  Serial.println(shuntVoltage_mV);
  Serial.print("Bus Voltage [V]: ");
  Serial.println(busVoltage_V);
  Serial.print("Load Voltage [V]: ");
  Serial.println(loadVoltage_V);
  Serial.print("Current[mA]: ");
  Serial.println(current_mA);
  Serial.print("Bus Power [mW]: ");
  Serial.println(power_mW);
  if (ina226.overflow) {
    Serial.println("! OVERFLOW !");
  }
  if (ina226.convAlert) {
    Serial.println("! CONVALERT !");
  }
  if (ina226.limitAlert) {
    Serial.println("! LIMIT_ALERT !");
  }
  Serial.println();
#endif
  *shunt = static_cast<int>(shuntVoltage_mV);
  *millivolt = static_cast<int>(busVoltage_V * 1000.0);
  *current = static_cast<int>(current_mA);
}

uint8_t normalize_volt(int millivolt) {
  if (millivolt < 1000)
    return 0;
  if (millivolt < 6500)
    return 5;
  if (millivolt < 11000)
    return 9;
  if (millivolt < 17000)
    return 16;
  if (millivolt < 22000)
    return 20;
  if (millivolt < 30000)
    return 28;
  if (millivolt < 39000)
    return 36;
  return 48;
}

#define AMP_BUF_SIZE 32
uint8_t last_volts = 0;
int last_currents[AMP_BUF_SIZE];
uint8_t current_ptr = 0;

int get_max_current(int current, uint8_t volts) {
  static int max_current = 0;
  int absCurrent = abs(current);
  if (volts != last_volts) {
    current_ptr = 0;
    for (int & last_current : last_currents) {
      last_current = 0;
    }
    last_volts = volts;
    max_current = 0;
  }
  if (max_current > 0 && last_currents[current_ptr] == max_current) {
    // digitalWrite(MY_BLUE_LED_PIN, LOW); // Turn the LED on (Note that LOW is
    // the voltage level
    last_currents[current_ptr] = absCurrent;
    max_current = 0;
    for (const int last_current : last_currents) {
      if (last_current > max_current)
        max_current = last_current;
    }
  } else {
    last_currents[current_ptr] = absCurrent;
    if (absCurrent > max_current) {
      max_current = absCurrent;
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

void display(int millivolt, uint8_t volt_norm, int milliamps, int maxcurrent) {
  char buf[32];
  char buf2[32];
  static unsigned long last_millis = 0;

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont17_tr);
    if (volt_norm == 0 && last_millis < millis() - SCREENSAVER_DELAY) {
      int x, y;
      screensaver(&x, &y);
      u8g2.drawPixel(x, y);
      continue;
    }
    if (volt_norm != 0) {
      last_millis = millis();
    }
    float amps = static_cast<float>(milliamps) / 1000.0F;
    float volts = static_cast<float>(millivolt) / 1000.0F;

    // if (volt_norm == 5)
    //   u8g2.drawStr(10, 17, "5V");
    // else if (volt_norm == 9)
    //   u8g2.drawStr(25, 17, "9V");
    // else if (volt_norm == 15)
    //   u8g2.drawStr(40, 17, "15V");
    // else if (volt_norm == 20)
    //   u8g2.drawStr(55, 17, "20V");
    // else if (volt_norm == 28)
    //   u8g2.drawStr(70, 17, "28V");
    // else if (volt_norm == 36)
    //   u8g2.drawStr(85, 17, "36V");
    // else if (volt_norm == 48)
    //   u8g2.drawStr(100, 17, "48V");
    sprintf(buf, "%0.2fV", volts);
    u8g2.drawStr(0, 17, buf);
    sprintf(buf, "%0.2fW", volts * amps);
    u8g2.drawStr(128 - u8g2.getStrWidth(buf), 17, buf);

    u8g2.setFont(u8g2_font_profont12_tr);
    sprintf(buf, "%0.3fA", ((float)maxcurrent / 1000.0F));
    u8g2.drawStr(127 - u8g2.getStrWidth(buf), 32, buf);
    u8g2.drawLine(127, 33, 127, 35);
    int bar = (int)((float)128 * ((float)abs(milliamps) / (float)maxcurrent));
    u8g2.drawLine(0, 34, bar, 34);

    u8g2.setFont(u8g2_font_profont29_tr);
    sprintf(buf2, "%0.3fA", amps);
    u8g2.drawStr(128 - u8g2.getStrWidth(buf2), 62, buf2);
  } while (u8g2.nextPage());
}

void loop() {
  int millivolt;
  int shunt;
  int current;

  digitalWrite(MY_BLUE_LED_PIN,
               HIGH); // Turn the LED on (Note that LOW is the voltage level

  read_ina(&shunt, &millivolt, &current);
  uint8_t volt_norm = normalize_volt(millivolt);

  serial_out(current, millivolt);

  int max_current = get_max_current(current, volt_norm);

  display(millivolt, volt_norm, current, max_current);

  delay(50);
}
