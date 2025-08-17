//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#include "PowerSensor.h"

PowerSensor::PowerSensor(uint8_t address, int sda, int scl, bool debug) 
    : ina228(address, &Wire), i2c_address(address), sda_pin(sda), scl_pin(scl), debug_enabled(debug) {
}

bool PowerSensor::begin() {
    // Initialize I2C
    if (!Wire.begin(sda_pin, scl_pin)) {
        Serial.println("Failed to initialize I2C!");
        return false;
    }
    Serial.println("I2C init done.");

    // Initialize INA228
    if (!ina228.begin()) {
        Serial.println("Failed to initialize INA228!");
        return false;
    }
    Serial.println("INA228 init done.");

    return true;
}

bool PowerSensor::isConnected() {
    return ina228.isConnected();
}

void PowerSensor::configure(float maxCurrent, float shuntResistance) {
    // Configure INA228 with optimal settings
    ina228.setADCRange(false);
    ina228.setMaxCurrentShunt(maxCurrent, shuntResistance);
    ina228.setAverage(INA228_16_SAMPLES);
    ina228.setBusVoltageConversionTime(INA228_50_us);
    ina228.setShuntVoltageConversionTime(INA228_1052_us);
    ina228.setTemperatureConversionTime(INA228_50_us);
    ina228.setMode(INA228_MODE_CONT_BUS_SHUNT);
    
    Serial.println("PowerSensor configured successfully");
}

PowerMeasurement PowerSensor::readMeasurement() {
    PowerMeasurement measurement = {0};
    
    measurement.shunt_mv = ina228.getShuntMilliVolt();
    measurement.voltage = ina228.getBusVolt();
    measurement.current = ina228.getCurrent();
    measurement.temperature_c = ina228.getTemperature();
    measurement.power = ina228.getPower();
    measurement.valid = true;
    
    if (debug_enabled) {
        printMeasurement(measurement);
    }
    
    return measurement;
}

float PowerSensor::getBusVoltage() {
    return ina228.getBusVolt();
}

float PowerSensor::getShuntMilliVolts() {
    return ina228.getShuntMilliVolt();
}

float PowerSensor::getCurrent() {
    return ina228.getCurrent();
}

float PowerSensor::getTemperature() {
    return ina228.getTemperature();
}

float PowerSensor::getPower() {
    return ina228.getPower();
}

void PowerSensor::printMeasurement(const PowerMeasurement& measurement) {
    if (!measurement.valid) {
        Serial.println("Invalid measurement");
        return;
    }
    
    Serial.printf("Shunt Voltage [mV]: %0.2f\n", measurement.shunt_mv);
    Serial.printf("Bus Voltage    [V]: %0.3f\n", measurement.voltage);
    Serial.printf("Current        [A]: %0.3f\n", measurement.current);
    Serial.printf("Power          [W]: %0.3f\n", measurement.power);
    Serial.printf("Temperature   [°C]: %0.2f\n", measurement.temperature_c);
    Serial.println();
}
