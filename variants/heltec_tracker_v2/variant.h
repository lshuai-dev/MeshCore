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
