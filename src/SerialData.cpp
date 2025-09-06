//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#include "SerialData.h"
#include <Arduino.h> // Required for Serial.print, highByte, lowByte, etc.

SerialData::SerialData(HardwareSerial &serialRef) : serial(&serialRef) {
}

#if defined(ESP32C3) && defined(ARDUINO_USB_MODE) &&                           \
    defined(ARDUINO_USB_CDC_ON_BOOT)
SerialData::SerialData(HWCDC &serialRef) : serial(&serialRef) {
}
#endif

// Helper function to convert a nibble to its hex character representation
static inline char hexdigit(uint8_t nibble) {
    return nibble < 10 ? nibble + '0' : nibble - 10 + 'A';
}

// Helper function to convert an int16_t to a 4-character hex string
static inline void int2hex(int16_t val, char *buf) {
    buf[0] = hexdigit((highByte(val) >> 4) & 0x0f);
    buf[1] = hexdigit((highByte(val)) & 0x0f);
    buf[2] = hexdigit((lowByte(val) >> 4) & 0x0f);
    buf[3] = hexdigit((lowByte(val)) & 0x0f);
}

void SerialData::out_pld(PowerMeasurement measurement) {
    float shuntval = measurement.shunt_mv / -0.002F;
    float voltval = measurement.voltage / 0.0031250F;
    int16_t shunt_ser = static_cast<int16_t>(shuntval);
    int16_t volt_ser = static_cast<int16_t>(voltval);

    //serial->printf("sv=%f vv=%f sw=%04x vw=%04x\n", shuntval,voltval,shunt_ser,volt_ser);
    char buf[16];
    int2hex(shunt_ser, buf);
    int2hex(volt_ser, buf + 4);
    buf[8] = 28;
    buf[9] = '\n';
    buf[10] = '\0';
    serial->print(buf);
}
