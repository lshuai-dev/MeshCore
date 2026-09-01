#include "HeltecE213Board.h"

#if defined(HELTEC_E213_LVGL) && HELTEC_E213_LVGL
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

void HeltecE213Board::begin() {
    ESP32Board::begin();

    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, LOW); // Initially inactive

    periph_power.begin();

    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_DEEPSLEEP) {
      long wakeup_source = esp_sleep_get_ext1_wakeup_status();
      if (wakeup_source & (1 << P_LORA_DIO_1)) {  // received a LoRa packet (while in deep sleep)
        startup_reason = BD_STARTUP_RX_PACKET;
      }

      rtc_gpio_hold_dis((gpio_num_t)P_LORA_NSS);
      rtc_gpio_deinit((gpio_num_t)P_LORA_DIO_1);
    }

#if defined(HELTEC_E213_LVGL) && HELTEC_E213_LVGL
    (void)heltec::meshcore::dal::display_port::init();

    using heltec::meshcore::dal::momentary_button::Config;
    using heltec::meshcore::dal::momentary_button::KeyMap;

    Config cnf{};
    // The side key is easy to bump and its line can produce short transients.
    // Require a level to remain stable before accepting a press or release.
    cnf.debounce_ms = 80;
    cnf.stable_level_debounce = true;
    cnf.multi_click_window_ms = 350;
    cnf.long_press_ms = 1000;
    cnf.buttons[0].pin = E213_LVGL_USER_BUTTON_PIN;
    cnf.buttons[0].pin_mode = INPUT_PULLUP;
    cnf.buttons[0].active_level = LOW;
    cnf.buttons[0].map =
        KeyMap(E213_LV_KEY_NEXT, E213_LV_KEY_ESC, E213_LV_KEY_PREV, E213_LV_KEY_ENTER);
    heltec::meshcore::dal::momentary_button::configure(cnf);
    (void)heltec::meshcore::dal::momentary_button::initialize();
#endif
  }

  void HeltecE213Board::enterDeepSleep(uint32_t secs, int pin_wake_btn) {
#if defined(HELTEC_E213_LVGL) && HELTEC_E213_LVGL
    // Flush pending UI changes, then remove panel power. E-ink keeps the last
    // image without holding the shared peripheral rail during deep sleep.
    heltec::meshcore::dal::display_port::setBacklightOn(false);
#endif
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Make sure the DIO1 and NSS GPIOs are hold on required levels during deep sleep
    rtc_gpio_set_direction((gpio_num_t)P_LORA_DIO_1, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en((gpio_num_t)P_LORA_DIO_1);

    rtc_gpio_hold_en((gpio_num_t)P_LORA_NSS);

    if (pin_wake_btn < 0) {
      esp_sleep_enable_ext1_wakeup( (1L << P_LORA_DIO_1), ESP_EXT1_WAKEUP_ANY_HIGH);  // wake up on: recv LoRa packet
    } else {
      esp_sleep_enable_ext1_wakeup( (1L << P_LORA_DIO_1) | (1L << pin_wake_btn), ESP_EXT1_WAKEUP_ANY_HIGH);  // wake up on: recv LoRa packet OR wake btn
    }

    if (secs > 0) {
      esp_sleep_enable_timer_wakeup(secs * 1000000);
    }

    // Finally set ESP32 into sleep
    esp_deep_sleep_start();   // CPU halts here and never returns!
  }

  void HeltecE213Board::powerOff()  {
    enterDeepSleep(0);
  }

  uint16_t HeltecE213Board::getBattMilliVolts()  {
    analogReadResolution(10);
    digitalWrite(PIN_ADC_CTRL, HIGH);

    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / 8;

    digitalWrite(PIN_ADC_CTRL, LOW);

    return (5.42 * (3.3 / 1024.0) * raw) * 1000;
  }

  const char* HeltecE213Board::getManufacturerName() const {
    return "Heltec E213";
  }
