//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H
#define USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H

#include <Arduino.h>
#include <INA226_WE.h>
#include "PowerMeasurement.h"

class PowerSensor {
private:
    INA226_WE *ina;
    uint8_t i2c_address;

public:
    explicit PowerSensor(uint8_t address);

    ~PowerSensor();

    bool begin();

    void configure(float maxCurrent = 10.0, float shuntResistance = 10e-3);

    PowerMeasurement readMeasurement();

    // Individual measurement methods
    float getBusVoltage();

    float getShuntMilliVolts();

    float getCurrent();

    float getPower();

    static void printMeasurement(const PowerMeasurement &measurement);
};

#endif // USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H
