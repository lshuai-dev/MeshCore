#pragma once

#include <stdint.h>

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#define HELTEC_TOUCH_INPUT 1
#else
#define HELTEC_TOUCH_INPUT 0
#endif

namespace heltec::meshcore::dal::touch_input {

using WakeFn = bool (*)();
using GestureBlockFn = bool (*)(int16_t x, int16_t y);

struct UiHooks {
  WakeFn wake = nullptr;
#if defined(HELTEC_V4_R8_TFT)
  /** Return true to route the complete touch sequence directly to LVGL. */
  GestureBlockFn raw_pointer_passthrough = nullptr;
#endif
  GestureBlockFn block_long_enter = nullptr;
  GestureBlockFn block_double_tap = nullptr;
};

#if HELTEC_TOUCH_INPUT
/** Bind UI callbacks (wake display, map-screen gesture blocks). Call once before init. */
void bindUi(const UiHooks& hooks);

/** Schedule touch init after display/LVGL is up (deferred by poll()). */
void armInit(uint16_t hor_res, uint16_t ver_res, uint32_t delay_ms = 400);

/** Run deferred init when due; call from ui_task::loop(). */
void poll();

bool isReady();
#else
inline void bindUi(const UiHooks&) {}
inline void armInit(uint16_t, uint16_t, uint32_t = 400) {}
inline void poll() {}
inline bool isReady() { return false; }
#endif

}  // namespace heltec::meshcore::dal::touch_input
