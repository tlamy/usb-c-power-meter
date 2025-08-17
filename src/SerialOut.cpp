//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#include "SerialOut.h"
#include <Arduino.h> // Required for Serial.print, highByte, lowByte, etc.

void SerialOut::out_pld(PowerMeasurement measurement) {
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
    ::Serial.print(buf); // Use ::Serial to distinguish from the Serial class
}