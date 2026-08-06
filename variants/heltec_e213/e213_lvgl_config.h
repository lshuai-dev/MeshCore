#pragma once

// E213 LVGL configuration. This file is force-included only by the LVGL UI
// base; examples-based E213 targets continue to use the hardware-only base.
#define HELTEC_E213_LVGL 1
#define HELTEC_DISPLAY_E213 1
#define UI_EINK_DISPLAY 1

#define HELTEC_LICENSE_BOOT_GATE 0
#define MOMENTARY_BUTTON_MAX 1
// E-paper has no backlight. Keep the panel powered while the UI is running;
// the board shutdown path still releases the peripheral rail for deep sleep.
#define UI_DISPLAY_BACKLIGHT 0
#define HELTEC_DISPLAY_AUTO_OFF_DEFAULT_SEC 0
#define AUTO_SHUTDOWN_MILLIVOLTS 3400

// E213 uses one full-screen LVGL buffer in PSRAM. LV_COLOR_DEPTH=1 still uses
// one byte per lv_color_t, so the 250 x 122 buffer occupies 30,500 bytes.
#define LV_COLOR_DEPTH 1
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE "drivers/display/lv_mem_psram.h"
#define LV_MEM_CUSTOM_ALLOC lv_mem_psram_alloc
#define LV_MEM_CUSTOM_FREE lv_mem_psram_free
#define LV_MEM_CUSTOM_REALLOC lv_mem_psram_realloc
#define LV_MEM_BUF_MAX_NUM 32
#define LV_MEMCPY_MEMSET_STD 1

// LVGL checks invalidated regions every 60 ms. With no invalidated object,
// the display flush callback is not called and the e-paper panel is untouched.
#define LV_DISP_DEF_REFR_PERIOD 60
#define LV_USE_LOG 0
#define LV_LAYER_SIMPLE_BUF_SIZE 4096U

#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

// Override the common UI profile because E213 has no text-entry UI.
#ifdef LV_USE_KEYBOARD
#undef LV_USE_KEYBOARD
#endif
#define LV_USE_KEYBOARD 0
#ifdef LV_USE_TEXTAREA
#undef LV_USE_TEXTAREA
#endif
#define LV_USE_TEXTAREA 0

#define LV_USE_BAR 0
#define LV_USE_ROLLER 0
#define LV_USE_MENU 1
#define LV_USE_SLIDER 0

#define UI_THEME_MONO 1
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_BASIC 0
#define LV_USE_THEME_MONO 1
#define LV_FONT_HELTEC_7 1
#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(lv_font_heltec_7)
#define LV_FONT_DEFAULT &lv_font_heltec_7

// E-paper should render only the final radial-navigation position instead of
// generating intermediate animation frames and physical refresh requests.
#define UI_NAV_RING_ANIM_MS 0
// Reuse the denser indexed page icons on E213. The transparent one-pixel ring
// variants are too fragile under the fast differential waveform.
#define UI_NAV_USE_SCREEN_ICONS 1

#define UI_INCLUDE_GPS_SCREEN 0
#define UI_INCLUDE_KEYBOARD_OVERLAY 0

#define ENV_INCLUDE_GPS 0
#define ENV_INCLUDE_COMPASS 0
#define ENV_INCLUDE_MAP 0

#define HELTEC_TOUCH_INPUT 0
#define HELTEC_TOUCH_GESTURE_INPUT 0
#define HELTEC_HAS_TOUCH 0

// GPIO0 is the Boot/PRG key. The independent user-programmable key on E213
// is wired to GPIO21 and is the key used by the LVGL single-button profile.
#define E213_LVGL_USER_BUTTON_PIN 21

// LVGL 8 key values are kept local so hardware-only E213 dependency discovery
// does not pull LVGL into examples-based targets through the board source.
#define E213_LV_KEY_ESC 27
#define E213_LV_KEY_ENTER 10
#define E213_LV_KEY_NEXT 9
#define E213_LV_KEY_PREV 11

#define E213_LVGL_FULL_REFRESH_INTERVAL 20
// Avoid the multi-phase black/white waveform on the first visible frame.
// fastmodeOn(true) initializes both controller buffers before the initial
// LVGL frame is committed with the differential waveform.
#define E213_LVGL_STARTUP_FULL_REFRESH 0
#define E213_LVGL_REFRESH_QUIET_MS 1000
#define E213_LVGL_CRITICAL_REFRESH_MAX_WAIT_MS 2000
#define E213_LVGL_DEFERRED_REFRESH_MAX_WAIT_MS 10000
#define E213_LVGL_FULL_REFRESH_AREA_PCT 0
// LVGL still renders into the E213 cache, but physical panel updates happen
// only after an explicit critical/deferred request.
#define E213_LVGL_AUTO_REFRESH 0
