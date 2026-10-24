#include "HeltecTrackerV2Board.h"

#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
#include <lvgl.h>
#include "heltec/drivers/display/DisplayPort.hpp"
#include "heltec/drivers/input/MomentaryButton.hpp"
#endif

void HeltecTrackerV2Board::begin() {
    ESP32Board::begin();

    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, LOW); // Initially inactive

    loRaFEMControl.init();

    esp_reset_reason_t reason = esp_reset_reason();
    periph_power.begin();
    if (reason == ESP_RST_DEEPSLEEP) {
      long wakeup_source = esp_sleep_get_ext1_wakeup_status();
      if (wakeup_source & (1 << P_LORA_DIO_1)) {  // received a LoRa packet (while in deep sleep)
        startup_reason = BD_STARTUP_RX_PACKET;
      }

      rtc_gpio_hold_dis((gpio_num_t)P_LORA_NSS);
      rtc_gpio_deinit((gpio_num_t)P_LORA_DIO_1);
    }

#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
    periph_power.claim();
    delay(50);  // let TFT rail settle before SPI init
    (void)Heltec::MeshCore::Dal::DisplayPort::init();

    using Heltec::MeshCore::Dal::MomentaryButton::Config;

    Config cnf{};
    cnf.debounce_ms = 50;
    cnf.multi_click_window_ms = 350;
    cnf.long_press_ms = 1000;
    cnf.buttons[0].pin = PIN_USER_BTN;
    cnf.buttons[0].pin_mode = INPUT;
    cnf.buttons[0].active_level = LOW;
    Heltec::MeshCore::Dal::MomentaryButton::configure(cnf);
    Heltec::MeshCore::Dal::MomentaryButton::initialize();
#endif
  }

  void HeltecTrackerV2Board::onBeforeTransmit(void) {
    digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED on
    loRaFEMControl.setTxModeEnable();
  }

  void HeltecTrackerV2Board::onAfterTransmit(void) {
    digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED off
    loRaFEMControl.setRxModeEnable();
  }

  bool HeltecTrackerV2Board::setLNAEnable(bool enabled) {
    if (!isLnaCanControl()) return false;
    loRaFEMControl.setLNAEnable(enabled);
    loRaFEMControl.setRxModeEnable();
    return true;
  }

  void HeltecTrackerV2Board::enterDeepSleep(uint32_t secs, int pin_wake_btn) {
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    rtc_gpio_set_direction((gpio_num_t)P_LORA_DIO_1, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en((gpio_num_t)P_LORA_DIO_1);
    rtc_gpio_hold_en((gpio_num_t)P_LORA_NSS);
    loRaFEMControl.setRxModeEnableWhenMCUSleep();

    uint64_t wake_mask = 1ULL << P_LORA_DIO_1;
    if (pin_wake_btn >= 0) {
      wake_mask |= 1ULL << pin_wake_btn;
    }
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    if (secs > 0) {
      esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(secs) * 1000000ULL);
    }
    esp_deep_sleep_start();
  }

  void HeltecTrackerV2Board::powerOff() {
    // Turn off PA
    digitalWrite(P_LORA_PA_POWER, LOW);
    rtc_gpio_hold_en((gpio_num_t)P_LORA_PA_POWER);

    ESP32Board::powerOff();
  }

  uint16_t HeltecTrackerV2Board::getBattMilliVolts()  {
    analogReadResolution(10);
    digitalWrite(PIN_ADC_CTRL, HIGH);
    delay(10);
    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / 8;

    digitalWrite(PIN_ADC_CTRL, LOW);

    return (5.42 * (3.3 / 1024.0) * raw) * 1000;
  }

  const char* HeltecTrackerV2Board::getManufacturerName() const {
    return "Heltec Tracker V2";
  }

  bool HeltecTrackerV2Board::setLoRaFemLnaEnabled(bool enable) {
    if (!loRaFEMControl.isLnaCanControl()) {
      return false;
    }

    loRaFEMControl.setLNAEnable(enable);
    loRaFEMControl.setRxModeEnable();
    return true;
  }

  bool HeltecTrackerV2Board::canControlLoRaFemLna() const {
    return loRaFEMControl.isLnaCanControl();
  }

  bool HeltecTrackerV2Board::isLoRaFemLnaEnabled() const {
    return loRaFEMControl.isLNAEnabled();
  }
