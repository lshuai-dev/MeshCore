#include "touch_input.hpp"

#if HELTEC_TOUCH_INPUT

#include "touch_port.hpp"

#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT
#include "touch_gesture_input.hpp"
#endif

#include <Arduino.h>
#include <lvgl.h>

namespace heltec::meshcore::dal::touch_input {
namespace {

bool s_armed = false;
bool s_init_done = false;
uint16_t s_hor = 0;
uint16_t s_ver = 0;
uint32_t s_init_after_ms = 0;

#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT
#if defined(HELTEC_V4_R8_TFT)
GestureBlockFn s_raw_pointer_passthrough_fn = nullptr;
bool s_raw_pointer_passthrough_active = false;
#endif

static void gestureHook(bool pressed, int16_t x, int16_t y, bool* suppress) {
#if defined(HELTEC_V4_R8_TFT)
  if (pressed) {
    if (!s_raw_pointer_passthrough_active && s_raw_pointer_passthrough_fn &&
        s_raw_pointer_passthrough_fn(x, y)) {
      s_raw_pointer_passthrough_active = true;
    }
    if (s_raw_pointer_passthrough_active) {
      if (suppress) *suppress = false;
      return;
    }
  } else if (s_raw_pointer_passthrough_active) {
    s_raw_pointer_passthrough_active = false;
    if (suppress) *suppress = false;
    return;
  }
#endif
  touch_gesture_input::feed(pressed, x, y, millis(),
                            touch_port::isNativeScrolling(), suppress);
}
#endif

}  // namespace

void bindUi(const UiHooks& hooks) {
  touch_port::set_wake_handler(hooks.wake);
#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT
  touch_gesture_input::init();
#if defined(HELTEC_V4_R8_TFT)
  s_raw_pointer_passthrough_fn = hooks.raw_pointer_passthrough;
  s_raw_pointer_passthrough_active = false;
#endif
  touch_gesture_input::setLongEnterBlocker(hooks.block_long_enter);
  touch_gesture_input::setDoubleTapBlocker(hooks.block_double_tap);
  touch_port::set_sample_hook(gestureHook);
#else
  touch_port::set_sample_hook(nullptr);
  (void)hooks;
#endif
}

void armInit(uint16_t hor_res, uint16_t ver_res, uint32_t delay_ms) {
  if (touch_port::isReady()) {
    s_init_done = true;
    s_armed = false;
    return;
  }
  s_hor = hor_res;
  s_ver = ver_res;
  s_init_after_ms = millis() + delay_ms;
  s_armed = true;
  s_init_done = false;
}

void poll() {
  if (s_init_done || !s_armed) return;
  if ((int32_t)(millis() - s_init_after_ms) < 0) return;
  s_armed = false;
  s_init_done = touch_port::init(s_hor, s_ver);
}

bool isReady() { return touch_port::isReady(); }

}  // namespace heltec::meshcore::dal::touch_input

#endif  // HELTEC_TOUCH_INPUT
