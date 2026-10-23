#include "InputPipeline.hpp"

#include "BacklightPolicy.hpp"
#include "heltec/drivers/input/KeyInput.hpp"
#include "heltec/drivers/input/MomentaryButtonDetail.hpp"
#include "InputAdapter.hpp"
#include "InputDispatcher.hpp"
#include "UiInputHost.hpp"
#include "ui/app/App.hpp"

#include <Arduino.h>

namespace Heltec::MeshCore::Ui
{

namespace
{

void emitKey(uint32_t key, bool pulse)
{
    if (key == 0)
        return;
    using Heltec::MeshCore::Dal::KeyInput::KeyDelivery;
    Heltec::MeshCore::Dal::KeyInput::post(key, pulse ? KeyDelivery::Pulse : KeyDelivery::Tracked);
}

void dispatchAdapted(IUiInputHost &host, const InputEvent &event, const AdaptedInput &input,
                     bool pulse)
{
    if (input.command == InputCommand::ToggleBacklight ||
        input.command == InputCommand::WakeBacklight)
    {
        if (BacklightPolicy::handleCommand(host, input.command, event.timestamp_ms))
            return;
    }

    if (input.command != InputCommand::None)
    {
        if (InputDispatcher::dispatch(host, input.command, event.timestamp_ms))
            return;
    }

    if (input.lv_key != 0)
    {
        emitKey(input.lv_key, pulse);
    }
}

} // namespace

void InputPipeline::init()
{
    Heltec::MeshCore::Dal::MomentaryButton::set_gesture_emit(
        +[](uint8_t slot, Heltec::MeshCore::Dal::MomentaryButton::ButtonGesture gesture)
        {
            InputPipeline::onButtonGesture(
                slot, static_cast<ButtonGesture>(static_cast<uint8_t>(gesture)), millis());
        });
}

void InputPipeline::onButtonGesture(uint8_t slot, ButtonGesture gesture, uint32_t now_ms)
{
    InputEvent ev;
    ev.source = InputSource::Button;
    ev.timestamp_ms = now_ms;
    ev.button.slot = slot;
    ev.button.gesture = gesture;

    IUiInputHost &host = UiApp::instance();
    if (BacklightPolicy::handle(host, ev))
        return;

    dispatchAdapted(host, ev, InputAdapter::active().adapt(ev), gesture == ButtonGesture::Long);
}

void InputPipeline::onSyntheticKey(uint32_t lv_key, bool pulse)
{
    const uint32_t now_ms = millis();
    InputEvent ev;
    ev.source = InputSource::Keyboard;
    ev.timestamp_ms = now_ms;
    ev.keyboard.code = lv_key;
    ev.keyboard.pressed = true;

    IUiInputHost &host = UiApp::instance();
    if (BacklightPolicy::handle(host, ev))
        return;
    dispatchAdapted(host, ev, InputAdapter::active().adapt(ev), pulse);
}

} // namespace Heltec::MeshCore::Ui
