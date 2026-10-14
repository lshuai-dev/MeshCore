#if defined(RADIOCORE_BOARD_RC52)

#include "RC52Board.h"

#include <Arduino.h>
#include <Wire.h>

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include <lvgl.h>
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#include "heltec/ui/core/input_pipeline.hpp"
#endif

namespace
{
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
constexpr uint32_t kIoExpanderPollMs = 20U;
RadioCoreTCA6408A *s_navigation_expander = nullptr;

void pollNavigationExpander(lv_timer_t *timer)
{
    (void)timer;
    if (s_navigation_expander == nullptr)
    {
        return;
    }

    const uint8_t pressed_edges = s_navigation_expander->pollPressedEdges();
    if ((pressed_edges & 0x01U) != 0U)
    {
        heltec::meshcore::ui::InputPipeline::onSyntheticKey(LV_KEY_PREV, true);
    }
    if ((pressed_edges & 0x02U) != 0U)
    {
        heltec::meshcore::ui::InputPipeline::onSyntheticKey(LV_KEY_NEXT, true);
    }
}
#endif

TwoWire &sensorWire()
{
#if defined(WIRE_INTERFACES_COUNT) && WIRE_INTERFACES_COUNT > 1
    return Wire1;
#else
    return Wire;
#endif
}

void initializeUserInterface(RefCountedDigitalPin &display_power,
                             RadioCoreTCA6408A &io_expander)
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
    if (io_expander.isReady())
    {
        s_navigation_expander = &io_expander;
        (void)lv_timer_create(pollNavigationExpander, kIoExpanderPollMs, nullptr);
    }
#else
    (void)display_power;
    (void)io_expander;
#endif
}
} // namespace

HeltecRadioCoreRC52Board::HeltecRadioCoreRC52Board()
    : NRF52Board("RC52_OTA"),
      display_power(PIN_TFT_VDD_CTL, PIN_TFT_VDD_CTL_ACTIVE),
      io_expander(sensorWire())
{
}

void HeltecRadioCoreRC52Board::begin()
{
    MESH_DEBUG_PRINTLN("[boot][rc52] board begin");
    NRF52BoardDCDC::begin();
#if defined(WIRE_INTERFACES_COUNT) && WIRE_INTERFACES_COUNT > 1
    MESH_DEBUG_PRINTLN("[boot][rc52] Wire1 begin");
    Wire1.begin();
    MESH_DEBUG_PRINTLN("[boot][rc52] Wire1 ready");
#else
    MESH_DEBUG_PRINTLN("[boot][rc52] Wire begin");
    Wire.begin();
    MESH_DEBUG_PRINTLN("[boot][rc52] Wire ready");
#endif
    display_power.begin();


#if defined(PIN_LORA_POWER) && (PIN_LORA_POWER >= 0)
    pinMode(PIN_LORA_POWER, OUTPUT);
    digitalWrite(PIN_LORA_POWER, PIN_LORA_POWER_ACTIVE);
#endif
#if defined(PIN_SENSOR_EN) && (PIN_SENSOR_EN >= 0) && defined(PIN_SENSOR_EN_ACTIVE)
    pinMode(PIN_SENSOR_EN, OUTPUT);
    digitalWrite(PIN_SENSOR_EN, PIN_SENSOR_EN_ACTIVE);
    MESH_DEBUG_PRINTLN("[boot][rc52] sensor power enabled pin=%d level=%d", PIN_SENSOR_EN,
                       PIN_SENSOR_EN_ACTIVE);
    delay(5);
#endif
#if defined(PIN_SENSOR_RST) && defined(PIN_SENSOR_INT)
    const bool io_expander_ready = io_expander.begin(PIN_SENSOR_RST, PIN_SENSOR_INT);
    MESH_DEBUG_PRINTLN("[boot][rc52] TCA6408A ready=%u dip=0x%02X",
                       io_expander_ready ? 1U : 0U,
                       static_cast<unsigned int>(io_expander.getDipSwitchState()));
#endif
    initializeUserInterface(display_power, io_expander);
    MESH_DEBUG_PRINTLN("[boot][rc52] board ready");
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
