#pragma once

// Heltec Tracker V2 build configuration.
// Force-included from platformio.ini to keep Windows g++ command lines below
// the CreateProcess limit while preserving the existing compile-time options.

#ifndef HELTEC_TRACKER_V2
#define HELTEC_TRACKER_V2 1
#endif
#ifndef HAS_LNA_CONTROL
#define HAS_LNA_CONTROL 1
#endif
#ifndef ESP32_CPU_FREQ
#define ESP32_CPU_FREQ 160
#endif

#ifndef USE_SX1262
#define USE_SX1262 1
#endif
#ifndef RADIO_CLASS
#define RADIO_CLASS CustomSX1262
#endif
#ifndef WRAPPER_CLASS
#define WRAPPER_CLASS CustomSX1262Wrapper
#endif

#ifndef P_LORA_TX_LED
#define P_LORA_TX_LED 18
#endif
#ifndef P_LORA_DIO_1
#define P_LORA_DIO_1 14
#endif
#ifndef P_LORA_NSS
#define P_LORA_NSS 8
#endif
#ifndef P_LORA_RESET
#define P_LORA_RESET 12
#endif
#ifndef P_LORA_BUSY
#define P_LORA_BUSY 13
#endif
#ifndef P_LORA_SCLK
#define P_LORA_SCLK 9
#endif
#ifndef P_LORA_MISO
#define P_LORA_MISO 11
#endif
#ifndef P_LORA_MOSI
#define P_LORA_MOSI 10
#endif
#ifndef P_LORA_PA_POWER
#define P_LORA_PA_POWER 7
#endif
#ifndef P_LORA_KCT8103L_PA_CSD
#define P_LORA_KCT8103L_PA_CSD 4
#endif
#ifndef P_LORA_KCT8103L_PA_CTX
#define P_LORA_KCT8103L_PA_CTX 5
#endif

#ifndef LORA_TX_POWER
#define LORA_TX_POWER 9
#endif
#ifndef MAX_LORA_TX_POWER
#define MAX_LORA_TX_POWER 22
#endif
#ifndef SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO2_AS_RF_SWITCH true
#endif
#ifndef SX126X_DIO3_TCXO_VOLTAGE
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#endif
#ifndef SX126X_CURRENT_LIMIT
#define SX126X_CURRENT_LIMIT 140
#endif
#ifndef SX126X_RX_BOOSTED_GAIN
#define SX126X_RX_BOOSTED_GAIN 1
#endif
#ifndef SX126X_REGISTER_PATCH
#define SX126X_REGISTER_PATCH 1
#endif

#ifndef PIN_BOARD_SDA
#define PIN_BOARD_SDA 6
#endif
#ifndef PIN_BOARD_SCL
#define PIN_BOARD_SCL 17
#endif
#ifndef PIN_USER_BTN
#define PIN_USER_BTN 0
#endif
#ifndef PIN_TFT_SDA
#define PIN_TFT_SDA 42
#endif
#ifndef PIN_TFT_SCL
#define PIN_TFT_SCL 41
#endif
#ifndef PIN_TFT_DC
#define PIN_TFT_DC 40
#endif
#ifndef PIN_TFT_RST
#define PIN_TFT_RST 39
#endif
#ifndef PIN_TFT_CS
#define PIN_TFT_CS 38
#endif
#ifndef USE_PIN_TFT
#define USE_PIN_TFT 1
#endif
#ifndef PIN_VEXT_EN
#define PIN_VEXT_EN 3
#endif
#ifndef PIN_VEXT_EN_ACTIVE
#define PIN_VEXT_EN_ACTIVE HIGH
#endif
#ifndef PIN_TFT_LEDA_CTL
#define PIN_TFT_LEDA_CTL 21
#endif
#ifndef PIN_TFT_LEDA_CTL_ACTIVE
#define PIN_TFT_LEDA_CTL_ACTIVE HIGH
#endif
#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION 1
#endif

#ifndef PIN_GPS_RX
#define PIN_GPS_RX 34
#endif
#ifndef PIN_GPS_TX
#define PIN_GPS_TX 33
#endif
#ifndef PIN_GPS_RESET
#define PIN_GPS_RESET 35
#endif
#ifndef PIN_GPS_RESET_ACTIVE
#define PIN_GPS_RESET_ACTIVE LOW
#endif
#ifndef GPS_UC6580
#define GPS_UC6580 1
#endif
#ifndef GPS_UC6580_CONFIGURE
#define GPS_UC6580_CONFIGURE 1
#endif
#ifndef GPS_BAUD_RATE
#define GPS_BAUD_RATE 115200
#endif
#ifndef ENV_SKIP_GPS_DETECT
#define ENV_SKIP_GPS_DETECT 1
#endif

#ifndef PIN_ADC_CTRL
#define PIN_ADC_CTRL 2
#endif
#ifndef PIN_VBAT_READ
#define PIN_VBAT_READ 1
#endif

#if defined(HELTEC_TRACKER_V2_TFT) && HELTEC_TRACKER_V2_TFT

#ifndef HELTEC_USE_ORLP_ED25519_VERIFY
#define HELTEC_USE_ORLP_ED25519_VERIFY 1
#endif
#ifndef HELTEC_MESH_UI
#define HELTEC_MESH_UI 1
#endif
#ifndef HELTEC_LICENSE_BOOT_GATE
#define HELTEC_LICENSE_BOOT_GATE 0
#endif
#ifndef HELTEC_DISPLAY_ST7735
#define HELTEC_DISPLAY_ST7735 1
#endif
#ifndef ST7735_LVGL_INVERT
#define ST7735_LVGL_INVERT 0
#endif
#ifndef HELTEC_ST7735_Y_GAP_ADJUST
#define HELTEC_ST7735_Y_GAP_ADJUST 0
#endif
#ifndef HELTEC_ST7735_EXTRA_CLEAR_RIGHT
#define HELTEC_ST7735_EXTRA_CLEAR_RIGHT 1
#endif
#ifndef HELTEC_ST7735_EXTRA_CLEAR_BOTTOM
#define HELTEC_ST7735_EXTRA_CLEAR_BOTTOM 2
#endif
#ifndef UI_THEME_MONO
#define UI_THEME_MONO 1
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

#endif  // HELTEC_TRACKER_V2_TFT
