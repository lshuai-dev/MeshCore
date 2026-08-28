#pragma once

// Heltec Radio Core build configuration.
// Force-included from platformio.ini to keep Windows g++ command lines below
// the CreateProcess limit while preserving the existing compile-time options.

#ifndef HELTEC_RADIO_CORE
#define HELTEC_RADIO_CORE 1
#endif

#define RADIOCORE_HAS_NV3001B_DISPLAY 1
#if defined(ARDUINO_ARCH_ESP32)
#define RADIOCORE_NV3001B_USE_ESP32_SPI 1
#define RADIOCORE_NV3001B_USE_HARDWARE_SPI 0
#else
#define RADIOCORE_NV3001B_USE_ESP32_SPI 0
#if defined(RADIOCORE_BOARD_RCC6)
#define RADIOCORE_NV3001B_USE_HARDWARE_SPI 0
#else
#define RADIOCORE_NV3001B_USE_HARDWARE_SPI 1
#endif
#endif
#define RADIOCORE_NV3001B_USE_SOFTWARE_SPI 0
#define RADIOCORE_NV3001B_DC PIN_TFT_DC
#define RADIOCORE_NV3001B_CS PIN_TFT_CS
#define RADIOCORE_NV3001B_SCK PIN_TFT_SCL
#define RADIOCORE_NV3001B_MOSI PIN_TFT_SDA
#define RADIOCORE_NV3001B_MISO -1
#if RADIOCORE_NV3001B_USE_HARDWARE_SPI
#define RADIOCORE_NV3001B_SPI_INSTANCE SPI
#endif
#if defined(RADIOCORE_NV3001B_SKIP_HARD_RESET)
#define RADIOCORE_NV3001B_RST -1
#else
#define RADIOCORE_NV3001B_RST PIN_TFT_RST
#endif
#define RADIOCORE_NV3001B_ENABLE PIN_TFT_VDD_CTL
#define RADIOCORE_NV3001B_ENABLE_ACTIVE PIN_TFT_VDD_CTL_ACTIVE
#define RADIOCORE_NV3001B_BACKLIGHT PIN_TFT_LEDA_CTL
#define RADIOCORE_NV3001B_BACKLIGHT_ACTIVE PIN_TFT_LEDA_CTL_ACTIVE
#define RADIOCORE_NV3001B_SPI_FREQUENCY 40000000U

#define DISPLAY_ROTATION 3

#if defined(HELTEC_RADIO_CORE_TFT) && HELTEC_RADIO_CORE_TFT

#ifndef HELTEC_USE_ORLP_ED25519_VERIFY
#define HELTEC_USE_ORLP_ED25519_VERIFY 1
#endif
#ifndef HELTEC_MESH_UI
#define HELTEC_MESH_UI 1
#endif
#ifndef HELTEC_LICENSE_BOOT_GATE
#define HELTEC_LICENSE_BOOT_GATE 0
#endif
#ifndef HELTEC_DISPLAY_NV3001B
#define HELTEC_DISPLAY_NV3001B 1
#endif

#ifndef HELTEC_UI_DISPLAY_MONO
#define HELTEC_UI_DISPLAY_MONO 1
#endif
#ifndef HELTEC_UI_INPUT_TOUCH
#define HELTEC_UI_INPUT_TOUCH 0
#endif
#ifndef MOMENTARY_BUTTON_MAX
#define MOMENTARY_BUTTON_MAX 1
#endif
#ifndef ED25519_NO_SEED
#define ED25519_NO_SEED 1
#endif

#ifndef LV_CONF_SKIP
#define LV_CONF_SKIP 1
#endif
#ifndef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH 16
#endif
#ifndef LV_DRAW_COMPLEX
#define LV_DRAW_COMPLEX 1
#endif
#if defined(RADIOCORE_BOARD_RC32)
#ifndef LV_MEM_CUSTOM
#define LV_MEM_CUSTOM 1
#endif
#ifndef LV_MEM_CUSTOM_INCLUDE
#define LV_MEM_CUSTOM_INCLUDE "drivers/display/lv_mem_psram.h"
#endif
#ifndef LV_MEM_CUSTOM_ALLOC
#define LV_MEM_CUSTOM_ALLOC lv_mem_psram_alloc
#endif
#ifndef LV_MEM_CUSTOM_FREE
#define LV_MEM_CUSTOM_FREE lv_mem_psram_free
#endif
#ifndef LV_MEM_CUSTOM_REALLOC
#define LV_MEM_CUSTOM_REALLOC lv_mem_psram_realloc
#endif
#else
#define LV_MEM_CUSTOM 0
#endif
#ifndef LV_MEM_BUF_MAX_NUM
#define LV_MEM_BUF_MAX_NUM 32
#endif
#ifndef LV_MEMCPY_MEMSET_STD
#define LV_MEMCPY_MEMSET_STD 1
#endif
#ifndef LV_USE_LOG
#define LV_USE_LOG 0
#endif
#ifndef LV_TICK_CUSTOM
#define LV_TICK_CUSTOM 1
#endif
#ifndef LV_LAYER_SIMPLE_BUF_SIZE
#define LV_LAYER_SIMPLE_BUF_SIZE 3200U
#endif
#ifndef LV_SHADOW_CACHE_SIZE
#define LV_SHADOW_CACHE_SIZE 0
#endif
#ifndef LV_GRAD_CACHE_DEF_SIZE
#define LV_GRAD_CACHE_DEF_SIZE 0
#endif

#ifndef LV_USE_GRID
#define LV_USE_GRID 0
#endif
#ifndef LV_USE_ANIMIMG
#define LV_USE_ANIMIMG 0
#endif
#ifndef LV_USE_THEME_DEFAULT
#define LV_USE_THEME_DEFAULT 0
#endif
#ifndef LV_USE_THEME_BASIC
#define LV_USE_THEME_BASIC 0
#endif
#ifndef LV_USE_THEME_MONO
#define LV_USE_THEME_MONO 1
#endif
#ifndef LV_BUILD_EXAMPLES
#define LV_BUILD_EXAMPLES 0
#endif
#ifndef LV_USE_DEMO_WIDGETS
#define LV_USE_DEMO_WIDGETS 0
#endif
#ifndef LV_USE_DEMO_KEYPAD_AND_ENCODER
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#endif
#ifndef LV_USE_DEMO_BENCHMARK
#define LV_USE_DEMO_BENCHMARK 0
#endif
#ifndef LV_USE_DEMO_STRESS
#define LV_USE_DEMO_STRESS 0
#endif
#ifndef LV_USE_DEMO_MUSIC
#define LV_USE_DEMO_MUSIC 0
#endif

#ifndef LV_USE_KEYBOARD
#define LV_USE_KEYBOARD 1
#endif
#ifndef LV_USE_TEXTAREA
#define LV_USE_TEXTAREA 1
#endif
#ifndef LV_USE_ARC
#define LV_USE_ARC 0
#endif
#ifndef LV_USE_BAR
#define LV_USE_BAR 0
#endif
#ifndef LV_USE_CALENDAR
#define LV_USE_CALENDAR 0
#endif
#ifndef LV_USE_CALENDAR_HEADER_ARROW
#define LV_USE_CALENDAR_HEADER_ARROW 0
#endif
#ifndef LV_USE_CALENDAR_HEADER_DROPDOWN
#define LV_USE_CALENDAR_HEADER_DROPDOWN 0
#endif
#ifndef LV_USE_CANVAS
#define LV_USE_CANVAS 0
#endif
#ifndef LV_USE_CHART
#define LV_USE_CHART 0
#endif
#ifndef LV_USE_CHECKBOX
#define LV_USE_CHECKBOX 0
#endif
#ifndef LV_USE_ROLLER
#define LV_USE_ROLLER 0
#endif
#ifndef LV_USE_LED
#define LV_USE_LED 0
#endif
#ifndef LV_USE_LIST
#define LV_USE_LIST 0
#endif
#ifndef LV_USE_MENU
#define LV_USE_MENU 1
#endif
#ifndef LV_USE_METER
#define LV_USE_METER 0
#endif
#ifndef LV_USE_MSGBOX
#define LV_USE_MSGBOX 0
#endif
#ifndef LV_USE_SLIDER
#define LV_USE_SLIDER 0
#endif
#ifndef LV_USE_SPAN
#define LV_USE_SPAN 0
#endif
#ifndef LV_USE_SPINBOX
#define LV_USE_SPINBOX 0
#endif
#ifndef LV_USE_SPINNER
#define LV_USE_SPINNER 0
#endif
#ifndef LV_USE_TABLE
#define LV_USE_TABLE 0
#endif
#ifndef LV_USE_TABVIEW
#define LV_USE_TABVIEW 0
#endif
#ifndef LV_USE_WIN
#define LV_USE_WIN 0
#endif
#ifndef LV_SPRINTF_USE_FLOAT
#define LV_SPRINTF_USE_FLOAT 0
#endif

#ifndef LV_FONT_MONTSERRAT_10
#define LV_FONT_MONTSERRAT_10 0
#endif
#ifndef LV_FONT_ARIAL_10
#define LV_FONT_ARIAL_10 1
#endif
#ifndef LV_FONT_CUSTOM_DECLARE
#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(lv_font_arial_10)
#endif
#ifndef LV_FONT_DEFAULT
#define LV_FONT_DEFAULT &lv_font_arial_10
#endif

#endif  // HELTEC_RADIO_CORE_TFT
