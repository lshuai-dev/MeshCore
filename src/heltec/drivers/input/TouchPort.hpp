#pragma once

#include <stdint.h>

namespace Heltec::MeshCore::Dal::TouchPort
{

bool init(uint16_t hor_res, uint16_t ver_res);
bool isReady();
/** True while a touch is held or a consumed gesture is awaiting release. */
bool isPressed();
/** True after LVGL has selected a scroll object for the current touch. */
bool isNativeScrolling();
/** Return the mapped display coordinate where the current touch began. */
bool getPressStart(int16_t &x, int16_t &y);
void requestReleaseBarrier();

/** Backlight wake while display auto-off is on; return true to swallow the touch event. */
using WakeFn = bool (*)();
void set_wake_handler(WakeFn fn);

/** Optional hook after coordinate mapping (gestures, etc.); set *suppress to hide LVGL pointer. */
using SampleHookFn = void (*)(bool pressed, int16_t x, int16_t y, bool *suppress);
void set_sample_hook(SampleHookFn fn);

} // namespace Heltec::MeshCore::Dal::TouchPort

#if !(defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH)

namespace Heltec::MeshCore::Dal::TouchPort
{

inline bool init(uint16_t, uint16_t)
{
    return false;
}
inline bool isReady()
{
    return false;
}
inline bool isPressed()
{
    return false;
}
inline bool isNativeScrolling()
{
    return false;
}
inline bool getPressStart(int16_t &, int16_t &)
{
    return false;
}
inline void requestReleaseBarrier()
{
}
inline void set_wake_handler(WakeFn)
{
}
inline void set_sample_hook(SampleHookFn)
{
}

} // namespace Heltec::MeshCore::Dal::TouchPort

#endif
