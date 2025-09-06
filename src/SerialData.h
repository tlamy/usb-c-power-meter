//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_SERIAL_H
#define USB_POWER_FIRMWARE_ESP8266_SERIAL_H

#include "PowerSensor.h" // Include PowerSensor.h for PowerMeasurement struct
#include <Arduino.h>     // Include Arduino.h for int16_t, uint8_t

#if defined(ESP32C3) && defined(ARDUINO_USB_MODE) &&                           \
    defined(ARDUINO_USB_CDC_ON_BOOT)
#include <HWCDC.h>
#endif

class SerialData {
private:
  // Use Stream as base type to support both HardwareSerial and HWCDC
  Stream *serial;

public:
  // Constructor for HardwareSerial
  explicit SerialData(HardwareSerial &serialRef);

#if defined(ESP32C3) && defined(ARDUINO_USB_MODE) &&                           \
    defined(ARDUINO_USB_CDC_ON_BOOT)
  // Constructor for HWCDC (USB Serial for ESP32-C3)
  explicit SerialData(HWCDC &serialRef);
#endif

  // Method to output PowerMeasurement data to serial in a specific format
  void out_pld(PowerMeasurement measurement);
};

#endif // USB_POWER_FIRMWARE_ESP8266_SERIAL_H
