#include "input_pipeline.hpp"

#include <Arduino.h>

#include "backlight_policy.hpp"
#include "focus_key_mapper.hpp"
#include "input_adapter.hpp"
#include "input_dispatcher.hpp"
#include "input_host.hpp"
#include "ui/app/ui_app.hpp"
#include "heltec/drivers/input/key_input.hpp"
#include "heltec/drivers/input/momentary_button_detail.hpp"

namespace heltec::meshcore::ui {

namespace {

void emitKey(uint32_t key, bool pulse) {
  if (key == 0) return;
  using heltec::meshcore::dal::key_input::KeyDelivery;
  heltec::meshcore::dal::key_input::post(
      key, pulse ? KeyDelivery::Pulse : KeyDelivery::Tracked);
}

void dispatchAdapted(InputHost& host, const InputEvent& event, const AdaptedInput& input,
                     bool pulse) {
  if (input.command == InputCommand::ToggleBacklight ||
      input.command == InputCommand::WakeBacklight) {
    if (BacklightPolicy::handleCommand(host, input.command, event.timestamp_ms)) return;
  }

  if (input.command != InputCommand::None) {
    if (InputDispatcher::dispatch(host, input.command, event.timestamp_ms)) return;
  }

  if (input.lv_key != 0) {
    uint32_t key = FocusKeyMapper::translate(input.lv_key);
    emitKey(key, pulse);
  }
}

}  // namespace

void InputPipeline::init() {
  heltec::meshcore::dal::momentary_button::set_gesture_emit(
      +[](uint8_t slot, heltec::meshcore::dal::momentary_button::ButtonGesture gesture) {
        InputPipeline::onButtonGesture(
            slot, static_cast<ButtonGesture>(static_cast<uint8_t>(gesture)), millis());
      });
}

void InputPipeline::onButtonGesture(uint8_t slot, ButtonGesture gesture, uint32_t now_ms) {
  InputEvent ev;
  ev.source = InputSource::Button;
  ev.timestamp_ms = now_ms;
  ev.button.slot = slot;
  ev.button.gesture = gesture;

  InputHost& host = UiApp::instance();
  if (BacklightPolicy::handle(host, ev)) return;

  dispatchAdapted(host, ev, InputAdapter::active().adapt(ev),
                  gesture == ButtonGesture::Long);
}

void InputPipeline::onSyntheticKey(uint32_t lv_key, bool pulse) {
  const uint32_t now_ms = millis();
  InputEvent ev;
  ev.source = InputSource::Keyboard;
  ev.timestamp_ms = now_ms;
  ev.keyboard.code = lv_key;
  ev.keyboard.pressed = true;

  InputHost& host = UiApp::instance();
  if (BacklightPolicy::handle(host, ev)) return;
  dispatchAdapted(host, ev, InputAdapter::active().adapt(ev), pulse);
}

}  // namespace heltec::meshcore::ui
