#include "PowerSensor.h"

PowerSensor::PowerSensor(uint8_t address, TwoWire& wireRef, bool debug)
    : ina228(nullptr), i2c_address(address), wire(&wireRef), debug_enabled(debug) {}

PowerSensor::~PowerSensor() {
  if (ina228 != nullptr) {
    delete ina228;
    ina228 = nullptr;
  }
}

bool PowerSensor::begin() {
  Serial.println("Initializing INA228...");
  // Create an INA228 instance with the provided Wire reference
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

bool PowerSensor::isConnected() const {
  if (ina228 == nullptr) {
    return false;
  }
  return ina228->isConnected();
}

void PowerSensor::configure(float maxCurrent, float shuntResistance) const {
  if (ina228 == nullptr) {
    return;
  }

  ina228->setMaxCurrentShunt(maxCurrent, shuntResistance);
}

PowerMeasurement PowerSensor::readMeasurement() const {
  PowerMeasurement measurement = {0};
  measurement.valid = false;

  if (ina228 == nullptr || !ina228->isConnected()) {
    return measurement;
  }

  measurement.voltage = getBusVoltage();
  measurement.shunt_mv = getShuntMilliVolts();
  measurement.current = getCurrent();
  measurement.power = getPower();
  measurement.charge = getCharge();
  measurement.temperature_c = getTemperature();
  measurement.valid = true;

  // Apply stored shunt offset (zero-calibration)
  if (_shuntOffsetMv != 0.0F) {
    measurement.shunt_mv -= _shuntOffsetMv;
    measurement.current -= _shuntOffsetMv / 1000.0F / ina228->getShunt();
    measurement.power = measurement.current * measurement.voltage;
  }

  // Zero-normalize below noise floor
  if (fabsf(measurement.shunt_mv) <= 0.015F) {
    measurement.current = 0.0F;
    measurement.power = 0.0F;
    measurement.shunt_mv = 0.0F;
  }

  return measurement;
}

float PowerSensor::getBusVoltage() const { return ina228->getBusVoltage(); }

float PowerSensor::getShuntMilliVolts() const {
  return ina228->getShuntVoltage() * 1000;  // Convert to millivolts
}

float PowerSensor::getCurrent() const { return ina228->getCurrent(); }

float PowerSensor::getTemperature() const { return ina228->getTemperature(); }

float PowerSensor::getPower() const { return ina228->getPower(); }
double PowerSensor::getCharge() const { return ina228->getCharge(); }

void PowerSensor::printMeasurement(const PowerMeasurement& measurement) {
  Serial.println("Power Measurement:");
  Serial.printf("  Voltage: %.3fV\n", measurement.voltage);
  Serial.printf("  Current: %.3fA\n", measurement.current);
  Serial.printf("  Power: %.3fW\n", measurement.power);
  Serial.printf("  Shunt: %.3fmV\n", measurement.shunt_mv);
  Serial.printf("  Temperature: %.1f°C\n", measurement.temperature_c);
  Serial.printf("  Valid: %s\n", measurement.valid ? "true" : "false");
}
