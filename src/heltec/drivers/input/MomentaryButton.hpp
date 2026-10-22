#pragma once

/**
 * @file MomentaryButton.hpp
 * @brief Physical GPIO button driver with click and long-press gesture detection.
 */

#include <stdint.h>

#ifndef HELTEC_UI_BUTTON_COUNT
#define HELTEC_UI_BUTTON_COUNT 1
#endif

namespace heltec::meshcore::dal::momentary_button
{

struct KeyMap
{
    uint32_t click = 0;
    uint32_t dbl = 0;
    uint32_t tri = 0;
    uint32_t long_press = 0;

    constexpr KeyMap() = default;
    constexpr KeyMap(uint32_t click_, uint32_t dbl_, uint32_t tri_, uint32_t long_)
        : click(click_), dbl(dbl_), tri(tri_), long_press(long_)
    {
    }
};

struct ButtonConfig
{
    int8_t pin = -1;
    uint8_t pin_mode = 0;
    uint8_t active_level = 1;
    uint16_t long_repeat_ms = 0;
    KeyMap map;
};

struct Config
{
    uint32_t debounce_ms = 30;
    uint32_t multi_click_window_ms = 350;
    uint32_t long_press_ms = 1000;
    bool stable_level_debounce = false;
    ButtonConfig buttons[HELTEC_UI_BUTTON_COUNT];
};

void configure(const Config &cfg);
bool initialize();
/** Process queued GPIO edges without requiring the LVGL input timer. */
void poll();
/** True if any configured button GPIO is currently pressed (e.g. backlight wake). */
bool anyPhysicalPressed();
/** True while GPIO edges, a held key, or a multi-click decision is pending. */
bool hasPendingGesture();

} // namespace heltec::meshcore::dal::momentary_button
