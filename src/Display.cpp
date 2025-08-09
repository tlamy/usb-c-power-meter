//
// Created by Thomas Lamy on 09.08.25.
//

#include "Display.h"

Display::Display() : u8g2(U8G2_R0) {
    // Initialize arrays
    for (int i = 0; i < AMP_BUF_SIZE; ++i) {
        last_currents[i] = 0;
    }
    for (int i = 0; i < COUNTS; ++i) {
        currents[i] = 0;
    }
}

void Display::begin() {
    u8g2.begin();
}

void Display::splash(const char* version) {
    char buf[64];
    u8g2.firstPage();
    u8g2.setFont(u8g2_font_profont29_tr);
    sprintf(buf, "MacWake");
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2,
                 (u8g2.getDisplayHeight() / 2) + u8g2.getFontAscent() / 2, buf);
    u8g2.setFont(u8g2_font_profont12_tf);
    sprintf(buf, "USB Power Meter");
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2,
                 (u8g2.getDisplayHeight() / 2) + u8g2.getFontAscent() / 2 + 16, buf);
    u8g2.setFont(u8g2_font_profont10_tf);
    sprintf(buf, "V%s", version);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(buf)) / 2, 62, buf);
    u8g2.nextPage();
    delay(1500);
}

void Display::screensaver(int *x, int *y) {
    *x = last_x + x_dir;
    if (*x > 127) {
        x_dir = -x_dir;
        *x = 126;
    } else if (*x < 0) {
        x_dir = -x_dir;
        *x = 0;
    }
    *y = last_y + y_dir;
    if (*y > 63) {
        y_dir = -y_dir;
        *y = 62;
    } else if (*y < 0) {
        y_dir = -y_dir;
        *y = 0;
    }
    last_x = *x;
    last_y = *y;
}

static uint8_t normalize_volt(float voltage) {
    if (voltage < 1.0)
        return 0;
    if (voltage < 6.0)
        return 5;
    if (voltage < 10.0)
        return 9;
    if (voltage < 16.0)
        return 16;
    if (voltage < 21.0)
        return 20;
    if (voltage < 29.0)
        return 28;
    if (voltage < 37.0)
        return 36;
    return 48;
}

float Display::get_max_current(float current, uint8_t volts) {
    if (volts != last_volts) {
        current_ptr = 0;
        for (int i = 0; i < AMP_BUF_SIZE; ++i) {
            last_currents[i] = 0;
        }
        last_volts = volts;
        max_current = 0;
    }
    if (max_current > 0 && last_currents[current_ptr] == max_current) {
        last_currents[current_ptr] = current;
        max_current = 0;
        for (int i = 0; i < AMP_BUF_SIZE; ++i) {
            if (last_currents[i] > max_current) max_current = last_currents[i];
        }
    } else {
        last_currents[current_ptr] = current;
        if (current > max_current) {
            max_current = current;
        }
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
        for (float c : currents) {
            if (c > display_current) display_current = c;
        }
    }
    if (display_current == -1) {
        display_current = measurement.current;
    }
    auto maxcurrent = get_max_current(measurement.current,volt_norm);

    // Calculate bar scale
    float bar_maxcurrent;
    if (maxcurrent < .100) {
        bar_maxcurrent = .100;
    } else if (maxcurrent< .250) {
        bar_maxcurrent = .250;
    } else if (maxcurrent < .500) {
        bar_maxcurrent = .500;
    } else if (maxcurrent < 1.000) {
        bar_maxcurrent = 1.000;
    } else {
        bar_maxcurrent = ceil(maxcurrent);
    }

    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_profont17_tf);
        if (volt_norm == 0) {
            int x, y;
            screensaver(&x, &y);
            u8g2.drawPixel(x, y);
        } else if (volt_norm == 5)
            u8g2.drawStr(10, 17, "5V");
        else if (volt_norm == 9)
            u8g2.drawStr(25, 17, "9V");
        else if (volt_norm == 15)
            u8g2.drawStr(40, 17, "15V");
        else if (volt_norm == 20)
            u8g2.drawStr(55, 17, "20V");
        else if (volt_norm == 28)
            u8g2.drawStr(70, 17, "28V");
        else if (volt_norm == 36)
            u8g2.drawStr(85, 17, "36V");
        else if (volt_norm == 48)
            u8g2.drawStr(100, 17, "48V");

        if (volt_norm > 0) {
            float amps = display_current;
            u8g2.setFont(u8g2_font_profont12_tf);
            sprintf(buf, "%0.3fA", maxcurrent);
            u8g2.drawStr(127 - u8g2.getStrWidth(buf), 32, buf);
            u8g2.drawLine(127, 33, 127, 35);
            int bar = (int) ((float) 128 * (float) ((float) measurement.current / (float) bar_maxcurrent));
            u8g2.drawLine(0, 34, bar, 34);
            bar = (int) ((float) 128 * (float) ((float) display_current / (float) bar_maxcurrent));
            u8g2.drawLine(bar, 33, bar, 35);

            u8g2.setFont(u8g2_font_profont29_mf);
            sprintf(buf2, "%0.3fA", amps);
            u8g2.drawStr(128 - u8g2.getStrWidth(buf2), 62, buf2);
        }
    } while (u8g2.nextPage());
}
