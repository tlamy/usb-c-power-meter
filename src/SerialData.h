//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_SERIAL_H
#define USB_POWER_FIRMWARE_ESP8266_SERIAL_H

#include <Arduino.h> // Include Arduino.h for int16_t, uint8_t
#include "PowerSensor.h" // Include PowerSensor.h for PowerMeasurement struct

#if defined(ESP32C3) && defined(ARDUINO_USB_MODE) && defined(ARDUINO_USB_CDC_ON_BOOT)
#include <HWCDC.h>
#endif

class SerialData {
private:
    Stream *serial;
    bool pld_compatible = true;

    void send_pld(PowerMeasurement measurement);

    // Helper function to convert a nibble to its hex character representation
    static inline char hexdigit(uint8_t nibble) { return nibble < 10 ? nibble + '0' : nibble - 10 + 'A'; }

    // Helper function to convert an int16_t to a 4-character hex string
    static inline void int2hex(int16_t val, char *buf) {
        buf[0] = hexdigit((highByte(val) >> 4) & 0x0f);
        buf[1] = hexdigit((highByte(val)) & 0x0f);
        buf[2] = hexdigit((lowByte(val) >> 4) & 0x0f);
        buf[3] = hexdigit((lowByte(val)) & 0x0f);
    }

public:
    // Constructor for HardwareSerial
    explicit SerialData(HardwareSerial &serialRef);

#if defined(ESP32C3) && defined(ARDUINO_USB_MODE) && defined(ARDUINO_USB_CDC_ON_BOOT)
    // Constructor for HWCDC (USB Serial for ESP32-C3)
    explicit SerialData(HWCDC &serialRef);
#endif

    void send_measurement(PowerMeasurement measurement);
};

#endif // USB_POWER_FIRMWARE_ESP8266_SERIAL_H
