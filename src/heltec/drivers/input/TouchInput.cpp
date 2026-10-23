#include "TouchInput.hpp"

#if HELTEC_TOUCH_INPUT

#include "TouchPort.hpp"

#if defined(HELTEC_TOUCH_GESTURE) && HELTEC_TOUCH_GESTURE
#include "TouchGestureInput.hpp"
#endif

#include <Arduino.h>
#include <lvgl.h>

namespace Heltec::MeshCore::Dal::TouchInput
{
namespace
{

bool s_armed = false;
bool s_init_done = false;
uint16_t s_hor = 0;
uint16_t s_ver = 0;
uint32_t s_init_after_ms = 0;

#if defined(HELTEC_TOUCH_GESTURE) && HELTEC_TOUCH_GESTURE
#if defined(HELTEC_V4_R8_TFT)
GestureBlockFn s_raw_pointer_passthrough_fn = nullptr;
bool s_raw_pointer_passthrough_active = false;
#endif

static void gestureHook(bool pressed, int16_t x, int16_t y, bool *suppress)
{
#if defined(HELTEC_V4_R8_TFT)
    if (pressed)
    {
        if (!s_raw_pointer_passthrough_active && s_raw_pointer_passthrough_fn &&
            s_raw_pointer_passthrough_fn(x, y))
        {
            s_raw_pointer_passthrough_active = true;
        }
        if (s_raw_pointer_passthrough_active)
        {
            if (suppress)
                *suppress = false;
            return;
        }
    }
    else if (s_raw_pointer_passthrough_active)
    {
        s_raw_pointer_passthrough_active = false;
        if (suppress)
            *suppress = false;
        return;
    }
#endif
    TouchGestureInput::feed(pressed, x, y, millis(), TouchPort::isNativeScrolling(), suppress);
}
#endif

} // namespace

void bindUi(const UiHooks &hooks)
{
    TouchPort::set_wake_handler(hooks.wake);
#if defined(HELTEC_TOUCH_GESTURE) && HELTEC_TOUCH_GESTURE
    TouchGestureInput::init();
#if defined(HELTEC_V4_R8_TFT)
    s_raw_pointer_passthrough_fn = hooks.raw_pointer_passthrough;
    s_raw_pointer_passthrough_active = false;
#endif
    TouchGestureInput::setLongEnterBlocker(hooks.block_long_enter);
    TouchGestureInput::setDoubleTapBlocker(hooks.block_double_tap);
    TouchPort::set_sample_hook(gestureHook);
#else
    TouchPort::set_sample_hook(nullptr);
    (void)hooks;
#endif
}

void armInit(uint16_t hor_res, uint16_t ver_res, uint32_t delay_ms)
{
    if (TouchPort::isReady())
    {
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

void poll()
{
    if (s_init_done || !s_armed)
        return;
    if ((int32_t)(millis() - s_init_after_ms) < 0)
        return;
    s_armed = false;
    s_init_done = TouchPort::init(s_hor, s_ver);
}

bool isReady()
{
    return TouchPort::isReady();
}

} // namespace Heltec::MeshCore::Dal::TouchInput

#endif // HELTEC_TOUCH_INPUT
