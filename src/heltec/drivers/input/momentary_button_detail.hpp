#pragma once

/**
 * @file momentary_button_detail.hpp
 * @brief Internal bridge for physical button gesture delivery.
 */

#include <stdint.h>

namespace heltec::meshcore::dal::momentary_button {

enum class ButtonGesture : uint8_t {
  None = 0,
  Click,
  Double,
  Triple,
  Long,
};

using GestureEmitFn = void (*)(uint8_t slot, ButtonGesture gesture);

/** When set, completed gestures invoke this instead of KeyMap lookup. */
void set_gesture_emit(GestureEmitFn fn);

}  // namespace heltec::meshcore::dal::momentary_button
