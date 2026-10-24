#pragma once

/**
 * @file MomentaryButton.hpp
 * @brief Physical GPIO button driver with click and long-press gesture detection.
 */

#include <stdint.h>

namespace Heltec::MeshCore::Dal::MomentaryButton
{

enum class ButtonGesture : uint8_t
{
    None = 0,
    Click,
    Double,
    Triple,
    Long,
    LongRepeat,
};

using GestureHandler = void (*)(uint8_t slot, ButtonGesture gesture);

struct ButtonConfig
{
    int8_t pin = -1;
    uint8_t pin_mode = 0;
    uint8_t active_level = 1;
    uint16_t long_repeat_ms = 0;
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
void setGestureHandler(GestureHandler handler);
bool initialize();
/** Process queued GPIO edges without requiring the LVGL input timer. */
void poll();
/** True if any configured button GPIO is currently pressed (e.g. backlight wake). */
bool anyPhysicalPressed();
/** True while GPIO edges, a held key, or a multi-click decision is pending. */
bool hasPendingGesture();

} // namespace Heltec::MeshCore::Dal::MomentaryButton
