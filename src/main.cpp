#include <Arduino.h>
#include <INA226_WE.h>
#include <U8g2lib.h>
#include <Wire.h>
#define INA_I2C_ADDRESS 0x41
#define MY_BLUE_LED_PIN D4

#include "Display.h"
#include "PowerSensor.h"
#include "SerialData.h"
#include "version.h"

// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
//INA226_WE ina226 = INA226_WE(&Wire, INA_I2C_ADDRESS);
SerialData serialOutput(Serial);
Display *display;
PowerSensor *powerSensor;

void scanI2C() {
    Serial.println("Scanning I2C bus...");
    int nDevices = 0;

    for (uint8_t address = 0x01; address < 0x7f; address++) {
        Serial.printf("Checking address 0x%02x... ", address);
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.println("FOUND");
            nDevices++;
        } else {
            Serial.println("not found");
        }
    }

    Serial.printf("I2C scan complete. Found %d device(s).\n", nDevices);
}

void setup() {
    Serial.begin(9600);
    delay(500);
    Serial.println();
    Serial.printf("MacWake USB-Power V%s\n", RELEASE_VERSION);

    pinMode(MY_BLUE_LED_PIN, OUTPUT); // Initialise the LED_BUILTIN pin as an output

    Wire.begin();

    display = new Display();
    display->begin();


    powerSensor = new PowerSensor(INA_I2C_ADDRESS);

    // Initialize power sensor
    if (!powerSensor->begin()) {
        Serial.println("Failed to initialize PowerSensor!");
        delay(10000);
        ESP.restart();
    }

    display->splash(RELEASE_VERSION);
}

void loop() {
    digitalWrite(MY_BLUE_LED_PIN, HIGH); // Turn the LED on (Note that LOW is the voltage level

    auto measurement = powerSensor->readMeasurement();
    serialOutput.send_measurement(measurement);
    display->display_measurements(measurement);

    delay(10);
}
