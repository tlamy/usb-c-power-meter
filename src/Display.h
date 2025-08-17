//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//


#ifndef USB_POWER_FIRMWARE_ESP8266_DISPLAY_H
#define USB_POWER_FIRMWARE_ESP8266_DISPLAY_H

#include <U8g2lib.h>
#include <Arduino.h>

#include "PowerSensor.h"

#define COUNTS 5
#define AMP_BUF_SIZE 32

class Display {
private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

    // Screensaver variables
    uint8_t last_x = 1, last_y = 2;
    int8_t x_dir = 3, y_dir = 2;

    // Current tracking variables
    uint8_t last_volts = 0;
    float last_currents[AMP_BUF_SIZE]{};
    uint8_t current_ptr = 0;
    float max_current = 0.0;

    // Display current smoothing
    uint8_t counter = 0;
    float currents[COUNTS]{};
    float display_current = 0.0;

    // Private methods
    void screensaver(int *col, int *line);

    float get_max_current(float current, uint8_t volts);

public:
    Display();
    void begin();

    void drawStrCentered(int line, const char *buf);

    void drawStrRight(int line, const char *buf);

    void splash(const char* version);
    void display_measurements(PowerMeasurement measurement);
};

#endif //USB_POWER_FIRMWARE_ESP8266_DISPLAY_H