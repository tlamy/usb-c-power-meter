//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#ifndef USB_POWER_FIRMWARE_ESP8266_DISPLAY_H
#define USB_POWER_FIRMWARE_ESP8266_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <vector>

#include "PowerSensor.h"

#define COUNTS 6
#define AMP_BUF_SIZE 128

class Display {
private:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C *u8g2;

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
  float display_power = 0.0;

  // Private methods
  void screensaver(int *col, int *line);

  float get_max_current(float current, uint8_t volts);

public:
  Display();

  ~Display(); // Destructor to clean up the u8g2 instance

  void begin(uint8_t sda, uint8_t scl, uint8_t i2cAddress = 0);

  void drawStrLeft(int line, const char *buf) const;
  void drawStrRight(int line, const char *buf) const;
  void drawStrCentered(int line, const char *buf) const;

  void splash(const char *version) const;

  void showDiagnostics(const char *version, float shunt, float maxCurrent, const std::vector<std::pair<uint8_t, String>> &devices) const;

  void display_measurements(const PowerMeasurement &measurement);

private:
  int last_display_time = 0;
  int last_valid_time = 0;
  uint32_t last_active_time = 0;
};

#endif // USB_POWER_FIRMWARE_ESP8266_DISPLAY_H
