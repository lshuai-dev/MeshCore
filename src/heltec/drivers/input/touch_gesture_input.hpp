#pragma once

#include <stdint.h>

namespace heltec::meshcore::dal::touch_gesture_input {

/** @return true to block horizontal swipe at (x,y). */
using BlockAtPointFn = bool (*)(int16_t x, int16_t y);
enum class SwipeAxis : uint8_t {
  Horizontal = 0,
  Vertical = 1,
};
/** Horizontal dir: +1 next tile, -1 previous tile. Vertical dir: +1 down, -1 up. */
using SwipeFn = void (*)(SwipeAxis axis, int8_t dir, int16_t start_x, int16_t start_y);

void init();
void setHorizontalSwipeBlocker(BlockAtPointFn fn);
void setVerticalSwipeBlocker(BlockAtPointFn fn);
void setLongEnterBlocker(BlockAtPointFn fn);
void setDoubleTapBlocker(BlockAtPointFn fn);
void setSwipeHandler(SwipeFn fn);

/** Feed a mapped touch sample; sets *suppress_pointer when LVGL pointer should stay released. */
void feed(bool pressed, int16_t x, int16_t y, uint32_t now_ms, bool* suppress_pointer);

}  // namespace heltec::meshcore::dal::touch_gesture_input
