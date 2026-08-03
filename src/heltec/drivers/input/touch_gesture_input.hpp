#pragma once

#include <stdint.h>

namespace heltec::meshcore::dal::touch_gesture_input {

/** Return true to protect a point from a custom gesture action. */
using BlockAtPointFn = bool (*)(int16_t x, int16_t y);

void init();
void setLongEnterBlocker(BlockAtPointFn fn);
void setDoubleTapBlocker(BlockAtPointFn fn);

/** Feed a mapped touch sample for non-native long-press and double-tap actions. */
void feed(bool pressed, int16_t x, int16_t y, uint32_t now_ms,
          bool native_scroll_active, bool* suppress_pointer);

}  // namespace heltec::meshcore::dal::touch_gesture_input
