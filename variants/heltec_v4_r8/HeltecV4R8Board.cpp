#include "HeltecV4R8Board.h"

#include <MeshCore.h>
#include <Wire.h>

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include <SPI.h>
#include <lvgl.h>
#include "target.h"
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

#ifndef HELTEC_BOARD_I2C_HZ
#define HELTEC_BOARD_I2C_HZ 100000
#endif

#if defined(HELTEC_V4_R8_TFT)
namespace {

const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt-wdt";
    case ESP_RST_TASK_WDT: return "task-wdt";
    case ESP_RST_WDT: return "other-wdt";
    case ESP_RST_DEEPSLEEP: return "deep-sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: return "unknown";
  }
}

}  // namespace
#endif

void HeltecV4R8Board::begin() {
  ESP32Board::begin();

  const esp_reset_reason_t reason = esp_reset_reason();
#if defined(HELTEC_V4_R8_TFT)
  Serial.printf("[boot] reset reason=%d (%s)\n", (int)reason, resetReasonName(reason));
#endif

  periph_power.begin();
  periph_power.claim();
  delay(10);

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
#if PIN_BOARD_SDA >= 0 && PIN_BOARD_SCL >= 0
#if defined(MESH_DEBUG) && MESH_DEBUG
  MESH_DEBUG_PRINTLN("[i2c] Wire.begin SDA=%d SCL=%d %luHz (before SDA=%d SCL=%d)", PIN_BOARD_SDA,
                     PIN_BOARD_SCL, (unsigned long)HELTEC_BOARD_I2C_HZ, digitalRead(PIN_BOARD_SDA),
                     digitalRead(PIN_BOARD_SCL));
#endif
  Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL, HELTEC_BOARD_I2C_HZ);
#if defined(ARDUINO_ARCH_ESP32)
  Wire.setTimeOut(50);
#endif
#if defined(MESH_DEBUG) && MESH_DEBUG
  delay(1);
  MESH_DEBUG_PRINTLN("[i2c] Wire.begin done (idle SDA=%d SCL=%d)", digitalRead(PIN_BOARD_SDA),
                     digitalRead(PIN_BOARD_SCL));
#endif
#endif
#endif

  loRaFEMControl.init();

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#if defined(SPI_INTERFACES_COUNT) && (SPI_INTERFACES_COUNT >= 2) && defined(PIN_SPI1_SCK) && \
    (PIN_SPI1_SCK >= 0) && defined(PIN_SPI1_MOSI) && (PIN_SPI1_MOSI >= 0)
#if defined(PIN_SPI1_MISO) && (PIN_SPI1_MISO >= 0)
  SPI1.begin(PIN_SPI1_SCK, PIN_SPI1_MISO, PIN_SPI1_MOSI, -1);
#else
  SPI1.begin(PIN_SPI1_SCK, -1, PIN_SPI1_MOSI, -1);
#endif
  SPI1.setFrequency(40000000);
#endif

  heltec::meshcore::dal::display_port::init();

  using heltec::meshcore::dal::momentary_button::Config;
  using heltec::meshcore::dal::momentary_button::KeyMap;

  Config cnf{};
  cnf.multi_click_window_ms = 350;
  cnf.long_press_ms = 1000;
  cnf.buttons[0].pin = BUTTON_PIN;
  cnf.buttons[0].pin_mode = INPUT_PULLDOWN;
  cnf.buttons[0].active_level = LOW;
#if MOMENTARY_BUTTON_MAX == 1
  cnf.buttons[0].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_PREV, LV_KEY_ENTER);
#else
  cnf.buttons[0].map = KeyMap(LV_KEY_PREV, LV_KEY_ESC, LV_KEY_LEFT, LV_KEY_ENTER);
#endif
#if MOMENTARY_BUTTON_MAX >= 2
  cnf.buttons[1].pin = PIN_USER_BTN;
  cnf.buttons[1].pin_mode = INPUT_PULLUP;
  cnf.buttons[1].active_level = HIGH;
  cnf.buttons[1].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_RIGHT, LV_KEY_ENTER);
#endif
  heltec::meshcore::dal::momentary_button::configure(cnf);
  heltec::meshcore::dal::momentary_button::initialize();
#endif

  if (reason == ESP_RST_DEEPSLEEP) {
    long wakeup_source = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_source & (1 << P_LORA_DIO_1)) {
      startup_reason = BD_STARTUP_RX_PACKET;
    }

    rtc_gpio_hold_dis((gpio_num_t)P_LORA_NSS);
    rtc_gpio_deinit((gpio_num_t)P_LORA_DIO_1);
  }
}

void HeltecV4R8Board::onBeforeTransmit(void) {
#if defined(P_LORA_TX_LED)
  digitalWrite(P_LORA_TX_LED, HIGH);
#endif
  loRaFEMControl.setTxModeEnable();
}

void HeltecV4R8Board::onAfterTransmit(void) {
#if defined(P_LORA_TX_LED)
  digitalWrite(P_LORA_TX_LED, LOW);
#endif
  loRaFEMControl.setRxModeEnable();
}

bool HeltecV4R8Board::setLNAEnable(bool enabled) {
  if (!isLnaCanControl()) return false;
  loRaFEMControl.setLNAEnable(enabled);
  loRaFEMControl.setRxModeEnable();
  return true;
}

void HeltecV4R8Board::enterDeepSleep(uint32_t secs, int pin_wake_btn) {
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  rtc_gpio_set_direction((gpio_num_t)P_LORA_DIO_1, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en((gpio_num_t)P_LORA_DIO_1);

  rtc_gpio_hold_en((gpio_num_t)P_LORA_NSS);
  loRaFEMControl.setRxModeEnableWhenMCUSleep();

  if (pin_wake_btn < 0) {
    esp_sleep_enable_ext1_wakeup((1L << P_LORA_DIO_1), ESP_EXT1_WAKEUP_ANY_HIGH);
  } else {
    esp_sleep_enable_ext1_wakeup((1L << P_LORA_DIO_1) | (1L << pin_wake_btn), ESP_EXT1_WAKEUP_ANY_HIGH);
  }

  if (secs > 0) {
    esp_sleep_enable_timer_wakeup(secs * 1000000);
  }

  esp_deep_sleep_start();
}

void HeltecV4R8Board::powerOff() {
  enterDeepSleep(0);
}

uint16_t HeltecV4R8Board::getBattMilliVolts() {
  analogReadResolution(12);

  uint32_t raw = 0;
  for (int i = 0; i < 8; i++) {
    raw += analogReadMilliVolts(PIN_VBAT_READ);
  }
  raw = raw / 8;

  return (adc_mult * raw);
}

const char* HeltecV4R8Board::getManufacturerName() const {
#if defined(HELTEC_V4_R8_TFT)
  return "Heltec V4 R8 TFT";
#elif defined(HELTEC_V4_R8_OLED)
  return "Heltec V4 R8 OLED";
#else
  return "Heltec V4 R8";
#endif
}
