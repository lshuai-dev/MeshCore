#include "InputPipeline.hpp"

#include "BacklightPolicy.hpp"
#include "Capabilities.hpp"
#include "UiInputHost.hpp"
#include "heltec/drivers/input/KeyInput.hpp"
#include "heltec/drivers/input/MomentaryButtonDetail.hpp"
#include "ui/app/App.hpp"
#include "ui/core/HtEvent.h"

#include <Arduino.h>
#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

namespace
{

#if HELTEC_UI_BUTTON_COUNT != 1
AdaptedInput adaptDualKeyButton(uint8_t slot, ButtonGesture gesture)
{
    AdaptedInput input;
    if (gesture == ButtonGesture::Click)
    {
        input.lv_key = (slot == 0) ? LV_KEY_UP : LV_KEY_DOWN;
    }
    else if (gesture == ButtonGesture::Double)
    {
        if (slot == 0)
        {
            input.command = InputCommand::CloseTopLayer;
            input.lv_key = LV_KEY_ESC;
        }
        else
        {
            input.command = InputCommand::OpenAction;
        }
    }
    else if (gesture == ButtonGesture::Long)
    {
        if (slot == 0)
        {
            input.lv_key = LV_KEY_ENTER;
        }
        else
        {
            input.command = InputCommand::CycleKeyboardNext;
        }
    }
    else if (gesture == ButtonGesture::LongRepeat && slot == 1)
    {
        input.command = InputCommand::CycleKeyboardNext;
    }
    return input;
}
#endif

#if HELTEC_UI_BUTTON_COUNT == 1
AdaptedInput adaptSingleKeyButton(ButtonGesture gesture)
{
    AdaptedInput input;
    if (gesture == ButtonGesture::Click)
    {
        input.lv_key = LV_KEY_DOWN;
    }
    else if (gesture == ButtonGesture::Double)
    {
        input.command = InputCommand::CloseTopLayer;
        input.lv_key = LV_KEY_ESC;
    }
    else if (gesture == ButtonGesture::Triple)
    {
        input.command = InputCommand::OpenAction;
    }
    else if (gesture == ButtonGesture::Long)
    {
        input.lv_key = LV_KEY_ENTER;
    }
    return input;
}
#endif

AdaptedInput adaptButton(uint8_t slot, ButtonGesture gesture)
{
#if HELTEC_UI_BUTTON_COUNT == 1
    (void)slot;
    return adaptSingleKeyButton(gesture);
#else
    return adaptDualKeyButton(slot, gesture);
#endif
}

AdaptedInput adaptKey(uint32_t key)
{
    AdaptedInput input;
    input.lv_key = key;
    if (input.lv_key == LV_KEY_NEXT)
    {
        input.lv_key = LV_KEY_DOWN;
    }
    else if (input.lv_key == LV_KEY_PREV)
    {
        input.lv_key = LV_KEY_UP;
    }
    if (input.lv_key == LV_KEY_ESC)
    {
        input.command = InputCommand::CloseTopLayer;
    }
    return input;
}

bool dispatchCommand(IUiInputHost &host, InputCommand command)
{
    if (!host.isReady())
    {
        return true;
    }

    if (command == InputCommand::CycleKeyboardNext)
    {
        if (!host.acceptsKeyboardCycle())
        {
            return true;
        }
        return Heltec::MeshCore::Dal::KeyInput::post(
            LV_KEY_RIGHT,
            Heltec::MeshCore::Dal::KeyInput::KeyDelivery::IndependentPulse);
    }

    switch (command)
    {
    case InputCommand::GoHome:
        host.ensureTileKeypadFocus();
        return true;
    case InputCommand::OpenNavigation:
        if (lv_obj_t *frame = host.frameRoot())
        {
            ht_event_send(frame, EventType::NavOpen);
        }
        return true;
    case InputCommand::OpenAction:
        if (lv_obj_t *frame = host.frameRoot())
        {
            ht_event_send(frame, EventType::ActionOpen);
        }
        return true;
    case InputCommand::CloseTopLayer:
        return false;
    default:
        return false;
    }
}

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
        if (dispatchCommand(host, input.command))
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

    dispatchAdapted(host, ev, adaptButton(slot, gesture), gesture == ButtonGesture::Long);
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
    dispatchAdapted(host, ev, adaptKey(lv_key), pulse);
}

} // namespace Heltec::MeshCore::Ui
