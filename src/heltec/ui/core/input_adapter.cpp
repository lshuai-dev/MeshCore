#include "input_adapter.hpp"

#include <lvgl.h>

#ifndef MOMENTARY_BUTTON_MAX
#define MOMENTARY_BUTTON_MAX 2
#endif

namespace heltec::meshcore::ui {
namespace {

AdaptedInput dualKeyButton(uint8_t slot, ButtonGesture gesture) {
  AdaptedInput out;
  if (gesture == ButtonGesture::Click) {
    out.lv_key = (slot == 0) ? LV_KEY_PREV : LV_KEY_NEXT;
  } else if (gesture == ButtonGesture::Double) {
    if (slot == 0) {
      out.command = InputCommand::CloseTopLayer;
      out.lv_key = LV_KEY_ESC;
    } else {
      out.command = InputCommand::OpenAction;
    }
  } else if (gesture == ButtonGesture::Long) {
    out.lv_key = LV_KEY_ENTER;
  }
  return out;
}

AdaptedInput singleKeyButton(ButtonGesture gesture) {
  AdaptedInput out;
  if (gesture == ButtonGesture::Click) {
    out.lv_key = LV_KEY_NEXT;
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

}  // namespace

const InputAdapter& InputAdapter::active() {
  static InputAdapter adapter;
  return adapter;
}

AdaptedInput InputAdapter::adapt(const InputEvent& event) const {
  switch (event.source) {
    case InputSource::Button:
      return adaptButton(event);
    case InputSource::Keyboard:
      return adaptKeyboard(event);
    default:
      return {};
  }
}

AdaptedInput InputAdapter::adaptButton(const InputEvent& event) const {
  const uint8_t slot = event.button.slot;
  const ButtonGesture gesture = event.button.gesture;

#if defined(HELTEC_INPUT_PROFILE_SINGLE_KEY) && defined(HELTEC_INPUT_PROFILE) && \
    HELTEC_INPUT_PROFILE == HELTEC_INPUT_PROFILE_SINGLE_KEY
  (void)slot;
  return singleKeyButton(gesture);
#elif MOMENTARY_BUTTON_MAX == 1 || defined(HELTEC_LORA_V4)
  // Keep boards configured with one physical button on the single-key
  // navigation profile even if a legacy/base environment leaves
  // MOMENTARY_BUTTON_MAX at its dual-button default.
  (void)slot;
  return singleKeyButton(gesture);
#else
  return dualKeyButton(slot, gesture);
#endif
}

AdaptedInput InputAdapter::adaptKeyboard(const InputEvent& event) const {
  AdaptedInput out;
  if (!event.keyboard.pressed) return out;
  out.lv_key = event.keyboard.code;
  if (out.lv_key == LV_KEY_ESC) out.command = InputCommand::CloseTopLayer;
  return out;
}

}  // namespace heltec::meshcore::ui
