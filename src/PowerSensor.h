//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H
#define USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H

#include <Arduino.h>
#include <INA228.h>
#include <Wire.h>

struct PowerMeasurement {
  float shunt_mv;
  float voltage;
  float current;
  float power;
  double charge;
  float temperature_c;
  bool valid;
};

class PowerSensor {
private:
  INA228 *ina228; // Changed to pointer
  uint8_t i2c_address;
  TwoWire *wire; // Store reference to Wire instance
  bool debug_enabled;

public:
  // Updated constructor to take Wire reference instead of pins
  PowerSensor(uint8_t address, TwoWire &wireRef, bool debug = false);

  ~PowerSensor(); // Added destructor to clean up pointer

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

  double getCharge();

  // Utility methods
  static void printMeasurement(const PowerMeasurement &measurement);

  // Access to underlying INA228 if needed
  INA228 &getINA228() { return *ina228; }
};

#endif // USB_POWER_FIRMWARE_ESP8266_POWERSENSOR_H
