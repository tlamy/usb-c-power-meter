//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#include "SerialData.h"
#include <Arduino.h> // Required for Serial.print, highByte, lowByte, etc.

SerialData::SerialData(HardwareSerial& serialRef) : serial(serialRef) {
}

void SerialData::out_pld(PowerMeasurement measurement) {
    float shuntval = measurement.shunt_mv / -0.2F;
    float voltval = measurement.voltage / 3125.0F;
    int16_t shunt_ser = static_cast<int16_t>(shuntval);
    int16_t volt_ser = static_cast<int16_t>(voltval);

    char buf[16];
    int2hex(shunt_ser, buf);
    int2hex(volt_ser, buf + 4);
    buf[8] = 28;
    buf[9] = '\n';
    buf[10] = '\0';
    serial.print(buf); // Use the injected serial reference
}