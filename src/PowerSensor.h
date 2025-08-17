//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H
#define USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <INA228.h>

struct PowerMeasurement {
    float shunt_mv;
    float voltage;
    float current;
    float power;
    float temperature_c;
    bool valid;
};

class PowerSensor {
private:
    INA228 ina228;
    uint8_t i2c_address;
    int sda_pin;
    int scl_pin;
    bool debug_enabled;

public:
    PowerSensor(uint8_t address, int sda, int scl, bool debug = false);
    
    bool begin();
    bool isConnected();
    void configure(float maxCurrent = 10.0, float shuntResistance = 10e-3);
    
    PowerMeasurement readMeasurement();
    
    // Individual measurement methods
    float getBusVoltage();
    float getShuntMilliVolts();
    float getCurrent();
    float getTemperature();
    float getPower();

    // Utility methods
    static void printMeasurement(const PowerMeasurement& measurement);

    // Access to underlying INA228 if needed
    INA228& getINA228() { return ina228; }
};

#endif //USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H
