#pragma once

#include <stdint.h>

namespace heltec::meshcore::ui {

enum class InputSource : uint8_t {
  None = 0,
  Button,
  Touch,
  Keyboard,
  kCount,
};

enum class ButtonGesture : uint8_t {
  None = 0,
  Click,
  Double,
  Triple,
  Long,
  LongLong,
  kCount,
};

enum class InputCommand : uint8_t {
  None = 0,
  CloseTopLayer,
  OpenNavigation,
  GoHome,
  ToggleBacklight,
  WakeBacklight,
  kCount,
};

enum class BacklightMode : uint8_t {
  AutoTimeout = 0,
  ManualToggle,
  kCount,
};

struct ButtonInput {
  uint8_t slot = 0;
  ButtonGesture gesture = ButtonGesture::None;
};

struct TouchInput {
  int16_t x = 0;
  int16_t y = 0;
  bool pressed = false;
};

struct KeyboardInput {
  uint32_t code = 0;
  bool pressed = false;
  uint8_t modifiers = 0;
};

struct InputEvent {
  InputSource source = InputSource::None;
  uint32_t timestamp_ms = 0;

  union {
    ButtonInput button;
    TouchInput touch;
    KeyboardInput keyboard;
  };

  InputEvent() : button{} {}
};

struct AdaptedInput {
  InputCommand command = InputCommand::None;
  uint32_t lv_key = 0;
};

}  // namespace heltec::meshcore::ui
