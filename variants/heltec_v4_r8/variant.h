#pragma once

// Board pins for Heltec WiFi LoRa 32 V4 R8.
// -include'd to shorten Windows g++ command line (CreateProcess limit).
// Profiles: HELTEC_V4_R8_TFT / HELTEC_V4_R8_OLED.

#ifndef ESP32_CPU_FREQ
#define ESP32_CPU_FREQ 240
#endif
#ifndef RADIO_CLASS
#define RADIO_CLASS CustomSX1262
#endif
#ifndef WRAPPER_CLASS
#define WRAPPER_CLASS CustomSX1262Wrapper
#endif

#ifndef P_LORA_DIO_1
#define P_LORA_DIO_1              (14)
#endif
#ifndef P_LORA_NSS
#define P_LORA_NSS                (8)
#endif
#ifndef P_LORA_RESET
#define P_LORA_RESET              (12)
#endif
#ifndef P_LORA_BUSY
#define P_LORA_BUSY               (13)
#endif
#ifndef P_LORA_SCLK
#define P_LORA_SCLK               (9)
#endif
#ifndef P_LORA_MISO
#define P_LORA_MISO               (11)
#endif
#ifndef P_LORA_MOSI
#define P_LORA_MOSI               (10)
#endif

// FSPI (default SPI bus 0): LoRa SX1262
#ifndef PIN_SPI_SCK
#define PIN_SPI_SCK               P_LORA_SCLK
#endif
#ifndef PIN_SPI_MOSI
#define PIN_SPI_MOSI              P_LORA_MOSI
#endif
#ifndef PIN_SPI_MISO
#define PIN_SPI_MISO              P_LORA_MISO
#endif
#ifndef PIN_SPI_NSS
#define PIN_SPI_NSS               P_LORA_NSS
#endif

#ifndef P_LORA_PA_POWER
#define P_LORA_PA_POWER           (7)
#endif
#ifndef P_LORA_KCT8103L_PA_CSD
#define P_LORA_KCT8103L_PA_CSD    (2)
#endif
#ifndef P_LORA_KCT8103L_PA_CTX
#define P_LORA_KCT8103L_PA_CTX    (5)
#endif
// #ifndef P_LORA_TX_LED
#ifndef PIN_BUTTON1
#define PIN_BUTTON1             (0)
#endif
#ifndef BUTTON_PIN
#define BUTTON_PIN              PIN_BUTTON1
#endif
#ifndef PIN_BUTTON2
#define PIN_BUTTON2             (46)
#endif
#ifndef PIN_USER_BTN
#define PIN_USER_BTN            PIN_BUTTON2
#endif
#ifndef PIN_VEXT_EN
#define PIN_VEXT_EN               (40)
#endif
#ifndef PIN_VEXT_EN_ACTIVE
#define PIN_VEXT_EN_ACTIVE        LOW
#endif
#ifndef ADC_MULTIPLIER
#define ADC_MULTIPLIER            5.0715f
#endif
#ifndef PIN_VBAT_READ
#define PIN_VBAT_READ             (1)
#endif

#ifndef LORA_TX_POWER
#define LORA_TX_POWER             22
#endif
#ifndef MAX_LORA_TX_POWER
#define MAX_LORA_TX_POWER         22
#endif
#ifndef SX126X_REGISTER_PATCH
#define SX126X_REGISTER_PATCH     1
#endif
#ifndef SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO2_AS_RF_SWITCH  true
#endif
#ifndef SX126X_DIO3_TCXO_VOLTAGE
#define SX126X_DIO3_TCXO_VOLTAGE  1.8
#endif
#ifndef SX126X_CURRENT_LIMIT
#define SX126X_CURRENT_LIMIT      140
#endif
#ifndef SX126X_RX_BOOSTED_GAIN
#define SX126X_RX_BOOSTED_GAIN    1
#endif

#ifndef PIN_GPS_RX
#define PIN_GPS_RX                (38)
#endif

#ifndef PIN_GPS_TX
#define PIN_GPS_TX                (39)
#endif
#ifndef PIN_GPS_EN
#define PIN_GPS_EN                (42)
#endif
#ifndef PIN_GPS_EN_ACTIVE
#define PIN_GPS_EN_ACTIVE         LOW
#endif
#ifndef PIN_GPS_RESET
#define PIN_GPS_RESET             (-1)
#endif

#if defined(HELTEC_V4_R8_TFT)
#ifndef GPS_L76K
#define GPS_L76K
#endif
#ifndef PIN_BOARD_SDA
#define PIN_BOARD_SDA             (17)
#endif
#ifndef PIN_BOARD_SCL
#define PIN_BOARD_SCL             (18)
#endif

#ifndef PIN_BUZZER
#define PIN_BUZZER                (4)
#endif
#ifndef PIN_TFT_RST
#define PIN_TFT_RST               (21)
#endif
#ifndef PIN_TFT_VDD_CTL
#define PIN_TFT_VDD_CTL           (-1)
#endif
#ifndef PIN_TFT_LEDA_CTL
#define PIN_TFT_LEDA_CTL          (44)
#endif
#ifndef PIN_TFT_LEDA_CTL_ACTIVE
#define PIN_TFT_LEDA_CTL_ACTIVE   HIGH
#endif
#ifndef PIN_TFT_CS
#define PIN_TFT_CS                (47)
#endif
#ifndef PIN_TFT_DC
#define PIN_TFT_DC                (48)
#endif

// HSPI (SPI1): ST7789 + microSD share one bus; begin() in HeltecV4R8Board.cpp
#undef SPI_INTERFACES_COUNT
#define SPI_INTERFACES_COUNT      (2)
#ifndef PIN_SPI1_SCK
#define PIN_SPI1_SCK              (16)
#endif
#ifndef PIN_SPI1_MOSI
#define PIN_SPI1_MOSI             (15)
#endif
#ifndef PIN_SPI1_MISO
#define PIN_SPI1_MISO             (45)
#endif

#ifndef PIN_TFT_SCL
#define PIN_TFT_SCL               PIN_BOARD_SCL
#endif
#ifndef PIN_TFT_SDA
#define PIN_TFT_SDA               PIN_BOARD_SDA
#endif
#ifndef PIN_TFT_MISO
#define PIN_TFT_MISO              PIN_SPI1_MISO
#endif

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
#ifndef PIN_MAP_SD_CS
#define PIN_MAP_SD_CS             (3)
#endif
#ifndef PIN_MAP_SD_SCK
#define PIN_MAP_SD_SCK            PIN_SPI1_SCK
#endif
#ifndef PIN_MAP_SD_MOSI
#define PIN_MAP_SD_MOSI           PIN_SPI1_MOSI
#endif
#ifndef PIN_MAP_SD_MISO
#define PIN_MAP_SD_MISO           PIN_SPI1_MISO
#endif
#endif

#ifndef DISPLAY_SCALE_X
#define DISPLAY_SCALE_X           2.5f
#endif
#ifndef DISPLAY_SCALE_Y
#define DISPLAY_SCALE_Y           3.75f
#endif
#ifndef HELTEC_TFT_HOR_RES
#define HELTEC_TFT_HOR_RES        240
#endif
#ifndef HELTEC_TFT_VER_RES
#define HELTEC_TFT_VER_RES        320
#endif
#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION          2
#endif
#ifndef ST7789_LVGL_INVERT
#define ST7789_LVGL_INVERT        1
#endif
#ifndef ST7789_DRAW_PART_LINES
#define ST7789_DRAW_PART_LINES    160
#endif

#ifndef PIN_TOUCH_RST
#define PIN_TOUCH_RST             (-1)
#endif
#ifndef PIN_TOUCH_IRQ
#define PIN_TOUCH_IRQ             (-1)
#endif
#ifndef HELTEC_TOUCH_SWAP_XY
#define HELTEC_TOUCH_SWAP_XY      0
#endif
#ifndef HELTEC_TOUCH_MIRROR_X
#define HELTEC_TOUCH_MIRROR_X     0
#endif
#ifndef HELTEC_TOUCH_MIRROR_Y
#define HELTEC_TOUCH_MIRROR_Y     0
#endif
#ifndef HELTEC_TOUCH_CAL_X_MIN
#define HELTEC_TOUCH_CAL_X_MIN    14
#endif
#ifndef HELTEC_TOUCH_CAL_X_MAX
#define HELTEC_TOUCH_CAL_X_MAX    231
#endif
#ifndef HELTEC_TOUCH_CAL_Y_MIN
#define HELTEC_TOUCH_CAL_Y_MIN    12
#endif
#ifndef HELTEC_TOUCH_CAL_Y_MAX
#define HELTEC_TOUCH_CAL_Y_MAX    319
#endif

#elif defined(HELTEC_V4_R8_OLED)

#ifndef PIN_BOARD_SDA
#define PIN_BOARD_SDA             (17)
#endif
#ifndef PIN_BOARD_SCL
#define PIN_BOARD_SCL             (18)
#endif
#ifndef PIN_OLED_RESET
#define PIN_OLED_RESET            (21)
#endif

#endif /* HELTEC_V4_R8_TFT / HELTEC_V4_R8_OLED */
