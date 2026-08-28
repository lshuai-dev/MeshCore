#include "HeltecRadioCoreC6Board.h"

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI && defined(HELTEC_DISPLAY_NV3001B)
#include <lvgl.h>
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

void HeltecRadioCoreC6Board::begin() {
    ESP32Board::begin();

#if defined(P_LORA_TX_LED)
    pinMode(P_LORA_TX_LED, OUTPUT);
    digitalWrite(P_LORA_TX_LED, LOW);
#endif

#if defined(PIN_ADC_CTRL) && PIN_ADC_CTRL >= 0
    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, !PIN_ADC_CTRL_ACTIVE);
#endif

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

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI && defined(HELTEC_DISPLAY_NV3001B)
    periph_power.claim();
    delay(50);  // let TFT rail settle before SPI init
    (void)heltec::meshcore::dal::display_port::init();

    using heltec::meshcore::dal::momentary_button::Config;
    using heltec::meshcore::dal::momentary_button::KeyMap;

    Config cnf{};
    cnf.debounce_ms = 50;
    cnf.multi_click_window_ms = 350;
    cnf.long_press_ms = 1000;
    cnf.buttons[0].pin = PIN_USER_BTN;
    cnf.buttons[0].pin_mode = INPUT;
    cnf.buttons[0].active_level = LOW;
    cnf.buttons[0].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_PREV, LV_KEY_ENTER);
    heltec::meshcore::dal::momentary_button::configure(cnf);
    heltec::meshcore::dal::momentary_button::initialize();
#endif
  }

  void HeltecRadioCoreC6Board::onBeforeTransmit(void) {
#if defined(P_LORA_TX_LED)
    digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED on
#endif
  }

  void HeltecRadioCoreC6Board::onAfterTransmit(void) {
#if defined(P_LORA_TX_LED)
    digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED off
#endif
  }

  void HeltecRadioCoreC6Board::enterDeepSleep(uint32_t secs, int pin_wake_btn) {
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

  void HeltecRadioCoreC6Board::powerOff()  {
    enterDeepSleep(0);
  }

  uint16_t HeltecRadioCoreC6Board::getBattMilliVolts()  {
#if defined(PIN_ADC_CTRL) && PIN_ADC_CTRL >= 0 && defined(PIN_VBAT_READ) && \
    PIN_VBAT_READ >= 0
    analogReadResolution(10);
    digitalWrite(PIN_ADC_CTRL, PIN_ADC_CTRL_ACTIVE);
    delay(10);
    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / 8;

    digitalWrite(PIN_ADC_CTRL, !PIN_ADC_CTRL_ACTIVE);

    return (5.42 * (3.3 / 1024.0) * raw) * 1000;
#else
    return 0;
#endif
  }

  const char* HeltecRadioCoreC6Board::getManufacturerName() const {
    return "Heltec Tracker V2";
  }
