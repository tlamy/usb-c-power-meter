#include "PowerSensor.h"

PowerSensor::PowerSensor(uint8_t address, TwoWire &wireRef, bool debug)
    : i2c_address(address), wire(&wireRef), debug_enabled(debug),
      ina228(nullptr) {}

PowerSensor::~PowerSensor() {
  if (ina228 != nullptr) {
    delete ina228;
    ina228 = nullptr;
  }
}

bool PowerSensor::begin() {
  Serial.println("Initializing INA228...");
  // Create INA228 instance with the provided Wire reference
  ina228 = new INA228(i2c_address, wire);

  if (debug_enabled) {
    Serial.printf("PowerSensor using I2C address: 0x%02X\n", i2c_address);
  }

  // Initialize INA228
  if (!ina228->begin()) {
    Serial.println("Failed to initialize INA228!");
    return false;
  }

  // Serial.println("INA228 init done.");
  return true;
}

bool PowerSensor::isConnected() {
  if (ina228 == nullptr) {
    return false;
  }
  return ina228->isConnected();
}

void PowerSensor::configure(float maxCurrent, float shuntResistance) {
  if (ina228 == nullptr) {
    return;
  }

  ina228->setMaxCurrentShunt(maxCurrent, shuntResistance);
}

PowerMeasurement PowerSensor::readMeasurement() {
  PowerMeasurement measurement = {0};
  measurement.valid = false;

  if (ina228 == nullptr || !ina228->isConnected()) {
    return measurement;
  }

  measurement.voltage = getBusVoltage();
  measurement.shunt_mv = getShuntMilliVolts();
  measurement.current = getCurrent();
  measurement.power = getPower();
  measurement.temperature_c = getTemperature();
  measurement.valid = true;

  return measurement;
}

float PowerSensor::getBusVoltage() { return ina228->getBusVoltage() : 0; }

float PowerSensor::getShuntMilliVolts() {
  return ina228->getShuntVoltage() * 1000; // Convert to millivolts
}

float PowerSensor::getCurrent() { return ina228->getCurrent() : 0; }

float PowerSensor::getTemperature() { return ina228->getTemperature() : 0; }

float PowerSensor::getPower() { return ina228->getPower() : 0; }

void PowerSensor::printMeasurement(const PowerMeasurement &measurement) {
  Serial.println("Power Measurement:");
  Serial.printf("  Voltage: %.3fV\n", measurement.voltage);
  Serial.printf("  Current: %.3fA\n", measurement.current);
  Serial.printf("  Power: %.3fW\n", measurement.power);
  Serial.printf("  Shunt: %.3fmV\n", measurement.shunt_mv);
  Serial.printf("  Temperature: %.1f°C\n", measurement.temperature_c);
  Serial.printf("  Valid: %s\n", measurement.valid ? "true" : "false");
}
