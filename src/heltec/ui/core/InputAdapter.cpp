#include "InputAdapter.hpp"
#include "Capabilities.hpp"
#include <lvgl.h>

namespace Heltec::MeshCore::Ui {
namespace {

AdaptedInput dualKeyButton(uint8_t slot, ButtonGesture gesture) {
    AdaptedInput out;
    if (gesture == ButtonGesture::Click) {
        out.lv_key = (slot == 0) ? LV_KEY_UP : LV_KEY_DOWN;
    } else if (gesture == ButtonGesture::Double) {
        if (slot == 0) {
            out.command = InputCommand::CloseTopLayer;
            out.lv_key = LV_KEY_ESC;
        } else {
            out.command = InputCommand::OpenAction;
        }
    } else if (gesture == ButtonGesture::Long) {
        if (slot == 0) {
            out.lv_key = LV_KEY_ENTER;
        } else {
            out.command = InputCommand::CycleKeyboardNext;
        }
    } else if (gesture == ButtonGesture::LongRepeat && slot == 1) {
        out.command = InputCommand::CycleKeyboardNext;
    }
    return out;
}

AdaptedInput singleKeyButton(ButtonGesture gesture) {
    AdaptedInput out;
    if (gesture == ButtonGesture::Click) {
        out.lv_key = LV_KEY_DOWN;
    } else if (gesture == ButtonGesture::Double) {
        out.command = InputCommand::CloseTopLayer;
        out.lv_key = LV_KEY_ESC;
    } else if (gesture == ButtonGesture::Triple) {
        out.command = InputCommand::OpenAction;
    } else if (gesture == ButtonGesture::Long) {
        out.lv_key = LV_KEY_ENTER;
    }
    return out;
}

} // namespace

const InputAdapter &InputAdapter::active() {
    static InputAdapter adapter;
    return adapter;
}

AdaptedInput InputAdapter::adapt(const InputEvent &event) const {
    switch (event.source) {
    case InputSource::Button:
        return adaptButton(event);
    case InputSource::Keyboard:
        return adaptKeyboard(event);
    default:
        return {};
    }
}

AdaptedInput InputAdapter::adaptButton(const InputEvent &event) const {
    const uint8_t slot = event.button.slot;
    const ButtonGesture gesture = event.button.gesture;

#if HELTEC_UI_BUTTON_COUNT == 1
    // Keep one-button boards on the single-key navigation profile.
    (void)slot;
    return singleKeyButton(gesture);
#else
    return dualKeyButton(slot, gesture);
#endif
}

AdaptedInput InputAdapter::adaptKeyboard(const InputEvent &event) const {
    AdaptedInput out;
    if (!event.keyboard.pressed) return out;
    out.lv_key = event.keyboard.code;
    if (out.lv_key == LV_KEY_NEXT) {
        out.lv_key = LV_KEY_DOWN;
    } else if (out.lv_key == LV_KEY_PREV) {
        out.lv_key = LV_KEY_UP;
    }
    if (out.lv_key == LV_KEY_ESC) out.command = InputCommand::CloseTopLayer;
    return out;
}

} // namespace Heltec::MeshCore::Ui
