#if defined(RADIOCORE_BOARD_RC32)

#include "Rc32Board.h"

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
#include <lvgl.h>
#include "heltec/drivers/display/DisplayPort.hpp"
#include "heltec/drivers/input/MomentaryButton.hpp"
#include "heltec/ui/core/InputPipeline.hpp"
#endif

namespace
{
#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
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
        Heltec::MeshCore::Ui::InputPipeline::onSyntheticKey(LV_KEY_PREV, true);
    }
    if ((pressed_edges & 0x02U) != 0U)
    {
        Heltec::MeshCore::Ui::InputPipeline::onSyntheticKey(LV_KEY_NEXT, true);
    }
}
#endif

void initializeUserInterface(RefCountedDigitalPin &display_power,
                             RadioCoreTCA6408A &io_expander)
{
#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
    display_power.claim();
    delay(50);
    (void)Heltec::MeshCore::Dal::DisplayPort::init();

    using Heltec::MeshCore::Dal::MomentaryButton::Config;
    using Heltec::MeshCore::Dal::MomentaryButton::KeyMap;
    Config config{};
    config.debounce_ms = 50;
    config.multi_click_window_ms = 350;
    config.long_press_ms = 1000;
    config.buttons[0].pin = PIN_USER_BTN;
    config.buttons[0].pin_mode = INPUT_PULLUP;
    config.buttons[0].active_level = LOW;
    config.buttons[0].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_PREV, LV_KEY_ENTER);
    Heltec::MeshCore::Dal::MomentaryButton::configure(config);
    Heltec::MeshCore::Dal::MomentaryButton::initialize();
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

HeltecRadioCoreRC32Board::HeltecRadioCoreRC32Board()
    : periph_power(PIN_TFT_VDD_CTL, PIN_TFT_VDD_CTL_ACTIVE),
      io_expander(Wire)
{
}

void HeltecRadioCoreRC32Board::begin()
{
    ESP32Board::begin();
    periph_power.begin();
#if defined(PIN_SENSOR_EN) && (PIN_SENSOR_EN >= 0) && defined(PIN_SENSOR_EN_ACTIVE)
    pinMode(PIN_SENSOR_EN, OUTPUT);
    digitalWrite(PIN_SENSOR_EN, PIN_SENSOR_EN_ACTIVE);
    delay(5);
#endif
#if defined(PIN_SENSOR_RST) && defined(PIN_SENSOR_INT)
    const bool io_expander_ready = io_expander.begin(PIN_SENSOR_RST, PIN_SENSOR_INT);
    MESH_DEBUG_PRINTLN("[boot][rc32] TCA6408A ready=%u dip=0x%02X",
                       io_expander_ready ? 1U : 0U,
                       static_cast<unsigned int>(io_expander.getDipSwitchState()));
#endif
#if defined(PIN_LORA_POWER) && (PIN_LORA_POWER >= 0)
    pinMode(PIN_LORA_POWER, OUTPUT);
    digitalWrite(PIN_LORA_POWER, PIN_LORA_POWER_ACTIVE);
#endif
    initializeUserInterface(periph_power, io_expander);
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
