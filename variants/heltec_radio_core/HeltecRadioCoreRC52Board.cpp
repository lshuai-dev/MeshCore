#if defined(RADIOCORE_BOARD_RC52)

#include "HeltecRadioCoreRC52Board.h"

#include <Arduino.h>
#include <Wire.h>

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

HeltecRadioCoreRC52Board::HeltecRadioCoreRC52Board()
    : NRF52Board("RC52_OTA"),
      display_power(PIN_TFT_VDD_CTL, PIN_TFT_VDD_CTL_ACTIVE)
{
}

void HeltecRadioCoreRC52Board::begin()
{
    NRF52BoardDCDC::begin();
    Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
    Wire.begin();

#if defined(WIRE_INTERFACES_COUNT) && WIRE_INTERFACES_COUNT > 1
    Wire1.setPins(ENV_PIN_SDA, ENV_PIN_SCL);
    Wire1.begin();
#endif
    display_power.begin();


#if defined(PIN_LORA_POWER) && (PIN_LORA_POWER >= 0)
    pinMode(PIN_LORA_POWER, OUTPUT);
    digitalWrite(PIN_LORA_POWER, PIN_LORA_POWER_ACTIVE);
#endif
#if defined(PIN_SENSOR_EN) && (PIN_SENSOR_EN >= 0) && defined(PIN_SENSOR_EN_ACTIVE) 
    pinMode(PIN_SENSOR_EN, OUTPUT);
    digitalWrite(PIN_SENSOR_EN, PIN_SENSOR_EN_ACTIVE);
#endif
    initializeUserInterface(display_power);
}

void HeltecRadioCoreRC52Board::onBeforeTransmit()
{
}

void HeltecRadioCoreRC52Board::onAfterTransmit()
{
}

void HeltecRadioCoreRC52Board::powerOff()
{
    display_power.release();
#if defined(PIN_LORA_POWER) && (PIN_LORA_POWER >= 0)
    digitalWrite(PIN_LORA_POWER, !PIN_LORA_POWER_ACTIVE);
#endif
    sd_power_system_off();
}

uint16_t HeltecRadioCoreRC52Board::getBattMilliVolts()
{
    analogReadResolution(12);
    analogReference(AR_INTERNAL_3_0);
    pinMode(PIN_ADC_CTRL, OUTPUT);
    digitalWrite(PIN_ADC_CTRL, PIN_ADC_CTRL_ACTIVE);
    delay(10);
    const uint32_t raw = analogRead(PIN_VBAT_READ);
    digitalWrite(PIN_ADC_CTRL, !PIN_ADC_CTRL_ACTIVE);
    return static_cast<uint16_t>(raw * (3000.0F / 4095.0F) * BATTERY_ADC_MULTIPLIER);
}

const char *HeltecRadioCoreRC52Board::getManufacturerName() const
{
    return "Heltec RadioCore RC52";
}

#endif
