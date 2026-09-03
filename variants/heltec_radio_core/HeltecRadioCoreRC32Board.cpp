#if defined(RADIOCORE_BOARD_RC32)

#include "HeltecRadioCoreRC32Board.h"

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include <lvgl.h>
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

namespace
{
void initializeUserInterface(RefCountedDigitalPin &display_power)
{
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
    display_power.claim();
    delay(50);
    (void)heltec::meshcore::dal::display_port::init();

    using heltec::meshcore::dal::momentary_button::Config;
    using heltec::meshcore::dal::momentary_button::KeyMap;
    Config config{};
    config.debounce_ms = 50;
    config.multi_click_window_ms = 350;
    config.long_press_ms = 1000;
    config.buttons[0].pin = PIN_USER_BTN;
    config.buttons[0].pin_mode = INPUT_PULLUP;
    config.buttons[0].active_level = LOW;
    config.buttons[0].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_PREV, LV_KEY_ENTER);
    heltec::meshcore::dal::momentary_button::configure(config);
    heltec::meshcore::dal::momentary_button::initialize();
#else
    (void)display_power;
#endif
}
} // namespace

HeltecRadioCoreRC32Board::HeltecRadioCoreRC32Board()
    : periph_power(PIN_TFT_VDD_CTL, PIN_TFT_VDD_CTL_ACTIVE)
{
}

void HeltecRadioCoreRC32Board::begin()
{
    ESP32Board::begin();
    periph_power.begin();
#if defined(PIN_LORA_POWER) && (PIN_LORA_POWER >= 0)
    pinMode(PIN_LORA_POWER, OUTPUT);
    digitalWrite(PIN_LORA_POWER, PIN_LORA_POWER_ACTIVE);
#endif
    initializeUserInterface(periph_power);
}

void HeltecRadioCoreRC32Board::onBeforeTransmit()
{
}

void HeltecRadioCoreRC32Board::onAfterTransmit()
{
}

void HeltecRadioCoreRC32Board::enterDeepSleep(uint32_t secs, int pin_wake_btn)
{
    uint64_t wake_mask = 1ULL << P_LORA_DIO_1;
    if (pin_wake_btn >= 0)
    {
        wake_mask |= 1ULL << pin_wake_btn;
    }
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    if (secs > 0)
    {
        esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(secs) * 1000000ULL);
    }
    esp_deep_sleep_start();
}

void HeltecRadioCoreRC32Board::powerOff()
{
    enterDeepSleep(0);
}

uint16_t HeltecRadioCoreRC32Board::getBattMilliVolts()
{
    analogReadResolution(12);
    const int previous_level = digitalRead(PIN_ADC_CTRL);
    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, PIN_ADC_CTRL_ACTIVE);
    delay(10);
    uint32_t raw = 0;
    for (uint8_t index = 0; index < 8; ++index)
    {
        raw += analogRead(PIN_VBAT_READ);
    }
    digitalWrite(PIN_ADC_CTRL, previous_level);
    raw /= 8;
    return static_cast<uint16_t>(raw * (3300.0F / 4095.0F) * BATTERY_ADC_MULTIPLIER);
}

const char *HeltecRadioCoreRC32Board::getManufacturerName() const
{
    return "Heltec RadioCore RC32";
}

#endif
