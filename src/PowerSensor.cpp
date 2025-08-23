//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#include "PowerSensor.h"

PowerSensor::PowerSensor(uint8_t address) : ina(nullptr), i2c_address(address) {}

PowerSensor::~PowerSensor() {
    if (ina != nullptr) {
        delete ina;
        ina = nullptr;
    }
}

bool PowerSensor::begin() {
    ina = new INA226_WE(i2c_address);
    if (!ina->init()) {
        Serial.printf("Failed to initialize INA at 0x%02x !", i2c_address);
        return false;
    }
    ina->setAverage(AVERAGE_16);
    ina->setConversionTime(CONV_TIME_2116);
    ina->setMeasureMode(CONTINUOUS);
    ina->setResistorRange(0.01, 6.0);
    ina->setCorrectionFactor(0.975); // must be aligned with good load

    return true;
}

void PowerSensor::configure(float maxCurrent, float shuntResistance) {
    if (ina == nullptr) {
        return;
    }

    ina->setResistorRange(shuntResistance, maxCurrent);
}

PowerMeasurement PowerSensor::readMeasurement() {
    PowerMeasurement measurement = {0};

    measurement.voltage = getBusVoltage();
    measurement.shunt_mv = getShuntMilliVolts();
    measurement.current = getCurrent();
    measurement.power = getPower();
    measurement.timestamp_ms = millis();

    return measurement;
}

float PowerSensor::getBusVoltage() { return ina->getBusVoltage_V(); }

float PowerSensor::getShuntMilliVolts() { return ina->getShuntVoltage_mV(); }

float PowerSensor::getCurrent() { return ina->getCurrent_A(); }

float PowerSensor::getPower() { return ina->getBusPower() / 1000.0F; }

void PowerSensor::printMeasurement(const PowerMeasurement &measurement) {
    Serial.println("Power Measurement:");
    Serial.printf("  Voltage: %.3fV\n", measurement.voltage);
    Serial.printf("  Current: %.3fA\n", measurement.current);
    Serial.printf("  Power: %.3fW\n", measurement.power);
    Serial.printf("  Shunt: %.3fmV\n", measurement.shunt_mv);
}
