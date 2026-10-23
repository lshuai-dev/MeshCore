#pragma once

/**
 * @file MomentaryButtonDetail.hpp
 * @brief Internal bridge for physical button gesture delivery.
 */

#include <stdint.h>

namespace Heltec::MeshCore::Dal::MomentaryButton
{

enum class ButtonGesture : uint8_t
{
    None = 0,
    Click,
    Double,
    Triple,
    Long,
    LongRepeat,
};

using GestureEmitFn = void (*)(uint8_t slot, ButtonGesture gesture);

/** When set, completed gestures invoke this instead of KeyMap lookup. */
void set_gesture_emit(GestureEmitFn fn);

} // namespace Heltec::MeshCore::Dal::MomentaryButton
