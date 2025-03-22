#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <INA226_WE.h>
#define INA_I2C_ADDRESS 0x41
#define MY_BLUE_LED_PIN D4

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
INA226_WE ina226 = INA226_WE(&Wire, INA_I2C_ADDRESS);

void splash() {
  char buf[64];
  u8g2.firstPage();
  u8g2.setFont(u8g2_font_profont29_tr);
  sprintf(buf, "MacWake");
  u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(buf))/2, (u8g2.getDisplayHeight()/2)+u8g2.getFontAscent()/2, buf);
  u8g2.nextPage();
  delay(1000);
}
void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("MacWake USB-Power V1.0");

  pinMode(MY_BLUE_LED_PIN, OUTPUT);     // Initialise the LED_BUILTIN pin as an output

  u8g2.begin();
  randomSeed(analogRead(A0)); 
  
  Wire.begin();
  ina226.init();
  ina226.setAverage(AVERAGE_4);
  ina226.setConversionTime(CONV_TIME_2116);
  ina226.setMeasureMode(CONTINUOUS);
  ina226.setResistorRange(0.01,20.0);
  ina226.setCorrectionFactor(0.95); // must be aligned with good load

  splash();  
}

char hexdigit(uint8_t nibble) {
  return nibble < 10 ? nibble+'0' : nibble-10+'A';
}
void int2hex(int16_t val, char *buf) {
  buf[0] = hexdigit((highByte(val)>>4) & 0x0f);
  buf[1] = hexdigit((highByte(val)) & 0x0f);
  buf[2] = hexdigit((lowByte(val)>>4) & 0x0f);
  buf[3] = hexdigit((lowByte(val)) & 0x0f);
}
void serial_out(int shunt, int millivolt) {
  float shuntval = -shunt/0.2;
  float voltval = millivolt/3.125;
  int16_t shunt_ser = shuntval;
  int16_t volt_ser = voltval;

  char buf[16];
  int2hex(shunt_ser, buf);
  int2hex(volt_ser, buf+4);
  buf[8] = 28;
  buf[9] = '\n';
  buf[10] = '\0';
  Serial.print(buf);
}

void read_ina(int *shunt, int *millivolt) {
//  float shuntVoltage_mV = 0.0;
//  float loadVoltage_V = 0.0;
  float busVoltage_V = 0.0;
  float current_mA = 0.0;
//  float power_mW = 0.0; 

  ina226.readAndClearFlags();
//  shuntVoltage_mV = ina226.getShuntVoltage_mV();
  busVoltage_V = ina226.getBusVoltage_V();
  current_mA = ina226.getCurrent_mA();
//  power_mW = ina226.getBusPower();
//  loadVoltage_V  = busVoltage_V + (shuntVoltage_mV/1000);
  
//  Serial.print("Shunt Voltage [mV]: "); Serial.println(shuntVoltage_mV);
//  Serial.print("Bus Voltage [V]: "); Serial.println(busVoltage_V);
//  Serial.print("Load Voltage [V]: "); Serial.println(loadVoltage_V);
//  Serial.print("Current[mA]: "); Serial.println(current_mA);
//  Serial.print("Bus Power [mW]: "); Serial.println(power_mW);
//  if(!ina226.overflow){
//    Serial.println("Values OK - no overflow");
//  }
//  else{
//    Serial.println("Overflow! Choose higher current range");
//  }
//  Serial.println();
  *shunt = current_mA/2;
  *millivolt = busVoltage_V*1000;
}

uint8_t normalize_volt(int millivolt) {
  if(millivolt < 1000)
    return 0;
  if(millivolt < 6000)
    return 5;
  if(millivolt < 10000)
    return 9;
  if(millivolt < 16000)
    return 16;
  if(millivolt < 21000)
    return 20;
  if(millivolt < 29000)
    return 28;
  if(millivolt < 37000)
    return 36;
   return 48;
}
#define AMP_BUF_SIZE 32
uint8_t last_volts = 0;
int last_currents[AMP_BUF_SIZE];
uint8_t current_ptr=0;
int max_current = 0;

int get_max_current(int current, uint8_t volts) {
  if(volts != last_volts) {
    current_ptr=0;
    for(int i=0; i < AMP_BUF_SIZE; ++i) {
      last_currents[i]=0;
    }
    last_volts = volts;
    max_current = 0;
  }
  if(max_current > 0 && last_currents[current_ptr] == max_current) {
      digitalWrite(MY_BLUE_LED_PIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      last_currents[current_ptr] = current;
      max_current = 0;
      for(int i=0; i < AMP_BUF_SIZE; ++i) {
        if(last_currents[i]>max_current) max_current = last_currents[i];
      }
  } else {
    last_currents[current_ptr] = current;
    if(current > max_current) {
      max_current = current;
    }
  }
  current_ptr = (current_ptr+1)%AMP_BUF_SIZE;
  return max_current;
}

void display(uint8_t volt_norm, int shunt, int maxcurrent) {
  char buf[32];
  char buf2[32];
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont17_tf);
    if(volt_norm == 0)
      u8g2.drawStr(0, 17,  "-");
    else if(volt_norm == 5)
      u8g2.drawStr(10, 17, "5V");
    else if(volt_norm == 9)
      u8g2.drawStr(30, 17, "9V");
    else if(volt_norm == 15)
      u8g2.drawStr(50, 17, "15V");
    else if(volt_norm == 20)
      u8g2.drawStr(70, 17, "20V");
    else if(volt_norm == 28)
      u8g2.drawStr(90, 17,  "28V");
    else if(volt_norm == 36)
      u8g2.drawStr(100, 17,  "36V");
    
    sprintf(buf, "%0.3fA", ((float)maxcurrent/1000.0));
    u8g2.drawStr(128-u8g2.getStrWidth(buf),33,buf);

    u8g2.setFont(u8g2_font_profont29_mf);
    float amps = ((float)shunt/1000.0);
    sprintf(buf2, "%0.3fA", amps);
    u8g2.drawStr(128-u8g2.getStrWidth(buf2),60,buf2);
    
  } while ( u8g2.nextPage() );
}
void loop() {
  int millivolt;
  int shunt;

      digitalWrite(MY_BLUE_LED_PIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level

read_ina(&shunt, &millivolt);
uint8_t volt_norm = normalize_volt(millivolt);
//  float randomNumber = 500 + random(1500);
//  shunt = (int)randomNumber;
//  if(millis() < 15000) {
//    millivolt = 5000 + (random(200))-100;
//  } else {
//    millivolt = 20000 + (random(200))-100;
//  }

  serial_out(shunt,millivolt);

  int max_current = get_max_current(shunt,volt_norm);

  display(volt_norm, shunt, max_current);

  delay(25);
}
