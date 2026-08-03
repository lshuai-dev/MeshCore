#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT

#include "touch_gesture_input.hpp"

#include "touch_port.hpp"
#include "ui/core/input_pipeline.hpp"

#include <Arduino.h>
#include <lvgl.h>

#ifndef HELTEC_TOUCH_GESTURE_SWIPE_PX
#define HELTEC_TOUCH_GESTURE_SWIPE_PX 24
#endif
#ifndef HELTEC_TOUCH_GESTURE_DBL_MS
#define HELTEC_TOUCH_GESTURE_DBL_MS 350
#endif
#ifndef HELTEC_TOUCH_GESTURE_DBL_DIST_PX
#define HELTEC_TOUCH_GESTURE_DBL_DIST_PX 20
#endif
#ifndef HELTEC_TOUCH_GESTURE_TAP_MS
#define HELTEC_TOUCH_GESTURE_TAP_MS 300
#endif
#ifndef HELTEC_TOUCH_GESTURE_LONG_MS
#define HELTEC_TOUCH_GESTURE_LONG_MS 1000
#endif

namespace heltec::meshcore::dal::touch_gesture_input {
namespace {

BlockAtPointFn s_block_long_enter = nullptr;
BlockAtPointFn s_block_double_tap = nullptr;

bool s_down = false;
int16_t s_x0 = 0;
int16_t s_y0 = 0;
uint32_t s_down_ms = 0;
int16_t s_max_abs_dx = 0;
int16_t s_max_abs_dy = 0;
bool s_long_fired = false;
bool s_suppress_pointer = false;

uint32_t s_last_tap_ms = 0;
int16_t s_last_tap_x = 0;
int16_t s_last_tap_y = 0;

static int16_t i16_abs(int16_t v) { return (v < 0) ? (int16_t)(-v) : v; }

static bool near_point(int16_t x, int16_t y, int16_t ox, int16_t oy) {
  const int16_t dx = (int16_t)(x - ox);
  const int16_t dy = (int16_t)(y - oy);
  return i16_abs(dx) <= HELTEC_TOUCH_GESTURE_DBL_DIST_PX &&
         i16_abs(dy) <= HELTEC_TOUCH_GESTURE_DBL_DIST_PX;
}

static void injectPulse(uint32_t key) {
  heltec::meshcore::ui::InputPipeline::onSyntheticKey(key, true);
}

static void consumeTouch() {
  touch_port::requestReleaseBarrier();
  s_suppress_pointer = true;
  s_last_tap_ms = 0;
}

}  // namespace

void init() {
  s_block_long_enter = nullptr;
  s_block_double_tap = nullptr;
  s_down = false;
  s_suppress_pointer = false;
  s_last_tap_ms = 0;
}

void setLongEnterBlocker(BlockAtPointFn fn) { s_block_long_enter = fn; }

void setDoubleTapBlocker(BlockAtPointFn fn) { s_block_double_tap = fn; }

void feed(bool pressed, int16_t x, int16_t y, uint32_t now_ms,
          bool native_scroll_active, bool* suppress_pointer) {
  if (suppress_pointer) *suppress_pointer = false;
  if (native_scroll_active) {
    s_down = false;
    s_suppress_pointer = false;
    s_last_tap_ms = 0;
    return;
  }

  if (pressed) {
    if (!s_down) {
      s_down = true;
      s_x0 = x;
      s_y0 = y;
      s_down_ms = now_ms;
      s_max_abs_dx = 0;
      s_max_abs_dy = 0;
      s_long_fired = false;
      s_suppress_pointer = false;
    } else {
      const int16_t dx = (int16_t)(x - s_x0);
      const int16_t dy = (int16_t)(y - s_y0);
      if (i16_abs(dx) > s_max_abs_dx) s_max_abs_dx = i16_abs(dx);
      if (i16_abs(dy) > s_max_abs_dy) s_max_abs_dy = i16_abs(dy);

      if (!s_long_fired && (now_ms - s_down_ms) >= (uint32_t)HELTEC_TOUCH_GESTURE_LONG_MS &&
          s_max_abs_dx < HELTEC_TOUCH_GESTURE_SWIPE_PX &&
          s_max_abs_dy < HELTEC_TOUCH_GESTURE_SWIPE_PX) {
        const bool block = s_block_long_enter && s_block_long_enter(s_x0, s_y0);
        if (!block) {
          consumeTouch();
          injectPulse(LV_KEY_ENTER);
          s_long_fired = true;
        }
      }
    }
  } else if (s_down) {
    s_down = false;
    const uint32_t held_ms = now_ms - s_down_ms;

    if (!s_long_fired && held_ms < (uint32_t)HELTEC_TOUCH_GESTURE_TAP_MS &&
        s_max_abs_dx < HELTEC_TOUCH_GESTURE_SWIPE_PX &&
        s_max_abs_dy < HELTEC_TOUCH_GESTURE_SWIPE_PX) {
      const bool block_double_tap =
          s_block_double_tap && s_block_double_tap(s_x0, s_y0);
      if (block_double_tap) {
        // A blocked tap must not pair with a later tap outside the protected region.
        s_last_tap_ms = 0;
      } else if (0 != s_last_tap_ms &&
                 (now_ms - s_last_tap_ms) <= (uint32_t)HELTEC_TOUCH_GESTURE_DBL_MS &&
           near_point(x, y, s_last_tap_x, s_last_tap_y)) {
        consumeTouch();
        injectPulse(LV_KEY_ESC);
        s_last_tap_ms = 0;
      } else {
        s_last_tap_ms = now_ms;
        s_last_tap_x = x;
        s_last_tap_y = y;
      }
    }
  }

  if (suppress_pointer && s_suppress_pointer) *suppress_pointer = true;
}

}  // namespace heltec::meshcore::dal::touch_gesture_input

#endif  // HELTEC_TOUCH_GESTURE_INPUT
