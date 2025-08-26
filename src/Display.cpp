//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//

#include "Display.h"

// Constructor
Display::Display() : u8g2(nullptr) {
  // We'll fully initialize u8g2 in begin()
}

// Destructor - clean up the u8g2 instance
Display::~Display() {
  if (u8g2 != nullptr) {
    delete u8g2;
    u8g2 = nullptr;
  }
}

void Display::begin(uint8_t sda, uint8_t scl) {
  // Create the u8g2 instance with the custom SDA and SCL pins
  // U8g2 will use the global Wire instance, but we can specify pins
  u8g2 =
      new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, scl, sda);

  // Initialize the display
  u8g2->begin();

  // Reset the current tracking arrays
  for (float &last_current: last_currents) {
    last_current = 0.0;
  }

  for (float &current: currents) {
    current = 0.0;
  }
}

void Display::drawStrCentered(int line, const char *buf) {
  if (u8g2 == nullptr)
    return;

  int width = u8g2->getStrWidth(buf);
  u8g2->drawStr((128 - width) / 2, line, buf);
}

void Display::drawStrRight(int line, const char *buf) {
  if (u8g2 == nullptr)
    return;

  int width = u8g2->getStrWidth(buf);
  u8g2->drawStr(128 - width, line, buf);
}

void Display::splash(const char *version) {
  char buf[64];
  u8g2->firstPage();
  u8g2->setFont(u8g2_font_profont29_tr);
  sprintf(buf, "MacWake");
  u8g2->drawStr((u8g2->getDisplayWidth() - u8g2->getStrWidth(buf)) / 2,
                (u8g2->getDisplayHeight() / 2) + (u8g2->getFontAscent() / 2),
                buf);
  u8g2->setFont(u8g2_font_profont12_tr);
  sprintf(buf, "USB Power Meter");
  u8g2->drawStr(
    (u8g2->getDisplayWidth() - u8g2->getStrWidth(buf)) / 2,
    (u8g2->getDisplayHeight() / 2) + (u8g2->getFontAscent() / 2) + 16, buf);
  u8g2->setFont(u8g2_font_profont10_tr);
  sprintf(buf, "V%s", version);
  drawStrCentered(62, buf);
  u8g2->nextPage();
  delay(1500);
}

void Display::screensaver(int *col, int *line) {
  *col = last_x + x_dir;
  if (*col > 127) {
    x_dir = -x_dir;
    *col = 126;
  } else if (*col < 0) {
    x_dir = -x_dir;
    *col = 0;
  }
  *line = last_y + y_dir;
  if (*line > 63) {
    y_dir = -y_dir;
    *line = 62;
  } else if (*line < 0) {
    y_dir = -y_dir;
    *line = 0;
  }
  last_x = *col;
  last_y = *line;
}

static uint8_t normalize_volt(float voltage) {
  if (voltage < 1.0) {
    return 0;
  }
  if (voltage < 6.0) {
    return 5;
  }
  if (voltage < 10.0) {
    return 9;
  }
  if (voltage < 16.0) {
    return 16;
  }
  if (voltage < 21.0) {
    return 20;
  }
  if (voltage < 29.0) {
    return 28;
  }
  if (voltage < 37.0) {
    return 36;
  }
  return 48;
}

float Display::get_max_current(float current, uint8_t volts) {
  if (volts != last_volts) {
    current_ptr = 0;
    for (float &last_current: last_currents) {
      last_current = 0.0;
    }
    last_volts = volts;
    max_current = 0.0;
  }

  if (max_current > 0.0 && last_currents[current_ptr] == max_current) {
    last_currents[current_ptr] = current;
    max_current = 0.0;
    for (float last_current: last_currents) {
      max_current = std::max(last_current, max_current);
    }
  } else {
    last_currents[current_ptr] = current;
    max_current = std::max(current, max_current);
  }
  current_ptr = (current_ptr + 1) % AMP_BUF_SIZE;
  return max_current;
}

void Display::display_measurements(PowerMeasurement measurement) {
  char buf[32];
  char buf2[32];

  const auto volt_norm = normalize_volt(measurement.voltage);
  // Update current smoothing
  currents[counter] = measurement.current;
  counter = (counter + 1) % COUNTS;
  if (counter == 0) {
    display_current = 0.0;
    for (float current: currents) {
      display_current = std::max(current, display_current);
    }
  }
  if (display_current == -1) {
    display_current = measurement.current;
  }
  auto maxcurrent = get_max_current(measurement.current, volt_norm);

  // Calculate bar scale
  float bar_maxcurrent;
  if (maxcurrent < .100) {
    bar_maxcurrent = .100;
  } else if (maxcurrent < .250) {
    bar_maxcurrent = .250;
  } else if (maxcurrent < .500) {
    bar_maxcurrent = .500;
  } else if (maxcurrent < 1.000) {
    bar_maxcurrent = 1.000;
  } else {
    bar_maxcurrent = ceil(maxcurrent);
  }

  u8g2->firstPage();
  do {
    if (volt_norm == 0) {
      int x;
      int y;
      screensaver(&x, &y);
      u8g2->drawPixel(x, y);
    } else {
      u8g2->setFont(u8g2_font_profont17_tr);
      u8g2_uint_t y_top = u8g2->getFontAscent();
      u8g2_uint_t y_line2 = 31;
      u8g2_uint_t y_bar = 36;
      sprintf(buf, "%dV", volt_norm);
      u8g2->drawStr(0, y_top, buf);
      // } else if (volt_norm == 5) {
      //     u8g2->drawStr(10, 17, "5V");
      // } else if (volt_norm == 9) {
      //     u8g2->drawStr(25, 17, "9V");
      // } else if (volt_norm == 15) {
      //     u8g2->drawStr(40, 17, "15V");
      // } else if (volt_norm == 20) {
      //     u8g2->drawStr(55, 17, "20V");
      // } else if (volt_norm == 28) {
      //     u8g2->drawStr(70, 17, "28V");
      // } else if (volt_norm == 36) {
      //     u8g2->drawStr(85, 17, "36V");
      // } else if (volt_norm == 48) {
      //     u8g2->drawStr(100, 17, "48V");

      sprintf(buf, "%0.3fW", measurement.power);
      drawStrRight(y_top, buf);

      float amps = display_current;
      u8g2->setFont(u8g2_font_profont10_tr);

      sprintf(buf, "M:%0.3fA", maxcurrent);
      drawStrCentered(y_line2, buf);
      sprintf(buf, "%0.2fA", bar_maxcurrent);
      drawStrRight(y_line2, buf);
      u8g2->drawLine(127, y_bar - 1, 127, y_bar + 1);
      int bar = static_cast<int>(128.0F * measurement.current / bar_maxcurrent);
      u8g2->drawLine(0, y_bar, bar, y_bar);
      bar = static_cast<int>(128.0F * display_current / bar_maxcurrent);
      u8g2->drawLine(bar, y_bar - 1, bar, y_bar + 1);

      u8g2->setFont(u8g2_font_profont29_mf);
      sprintf(buf2, "%0.3fA", amps);
      drawStrRight(63, buf2);
    }
  } while (u8g2->nextPage() > 0);
}
