//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_V1_2_POWERMEASUREMENT_H
#define USB_POWER_V1_2_POWERMEASUREMENT_H

struct PowerMeasurement {
    unsigned long timestamp_ms;
    float shunt_mv;
    float voltage;
    float current;
    float power;
};

#endif //USB_POWER_V1_2_POWERMEASUREMENT_H