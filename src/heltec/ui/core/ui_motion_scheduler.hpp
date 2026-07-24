#pragma once

#include <stdint.h>

namespace heltec::meshcore::ui {

using UiMotionExec = void (*)(void* target, int32_t value);
using UiMotionReady = void (*)(void* user_data);

enum class UiMotionPath : uint8_t {
  Linear = 0,
  EaseIn,
  EaseOut,
  EaseInOut,
};

struct UiMotionSpec {
  void* target = nullptr;
  UiMotionExec exec = nullptr;
  UiMotionReady ready = nullptr;
  void* ready_data = nullptr;
  int32_t start_value = 0;
  int32_t end_value = 0;
  uint16_t duration_ms = 0;
  UiMotionPath path = UiMotionPath::Linear;
};

/** Create the fixed-slot scheduler timer once during UI startup. */
bool ui_motion_init();
bool ui_motion_start(const UiMotionSpec& spec);
uint8_t ui_motion_cancel(void* target, UiMotionExec exec = nullptr);
bool ui_motion_active(void* target, UiMotionExec exec = nullptr);

}  // namespace heltec::meshcore::ui
