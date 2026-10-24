#include "InputRouter.hpp"

#include "BacklightPolicy.hpp"
#include "heltec/drivers/input/KeyInput.hpp"
#include "ui/app/App.hpp"

#include <Arduino.h>
#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{
namespace
{

using ButtonGesture = Heltec::MeshCore::Dal::MomentaryButton::ButtonGesture;
using KeyDelivery = Heltec::MeshCore::Dal::KeyInput::KeyDelivery;

void postKey(uint32_t key, bool pulse)
{
    if (key == 0)
    {
        return;
    }
    if (pulse)
    {
        (void)Heltec::MeshCore::Dal::KeyInput::postKey(key);
        return;
    }
    (void)Heltec::MeshCore::Dal::KeyInput::postState(key, KeyDelivery::Tracked);
}

void cycleKeyboard(UiApp &app)
{
    if (!app.isReady() || !app.acceptsKeyboardCycle())
    {
        return;
    }
    (void)Heltec::MeshCore::Dal::KeyInput::postState(LV_KEY_RIGHT,
                                                      KeyDelivery::IndependentPulse);
}

#if HELTEC_UI_BUTTON_COUNT == 1
void routeButton(UiApp &app, uint8_t slot, ButtonGesture gesture)
{
    (void)slot;
    switch (gesture)
    {
    case ButtonGesture::Click:
        postKey(LV_KEY_DOWN, false);
        break;
    case ButtonGesture::Double:
        postKey(LV_KEY_ESC, false);
        break;
    case ButtonGesture::Triple:
        if (app.isReady())
        {
            (void)app.openActionPane();
        }
        break;
    case ButtonGesture::Long:
        postKey(LV_KEY_ENTER, true);
        break;
    default:
        break;
    }
}
#else
void routeButton(UiApp &app, uint8_t slot, ButtonGesture gesture)
{
    switch (gesture)
    {
    case ButtonGesture::Click:
        postKey(slot == 0 ? LV_KEY_UP : LV_KEY_DOWN, false);
        break;
    case ButtonGesture::Double:
        if (slot == 0)
        {
            postKey(LV_KEY_ESC, false);
        }
        else if (app.isReady())
        {
            (void)app.openActionPane();
        }
        break;
    case ButtonGesture::Long:
        if (slot == 0)
        {
            postKey(LV_KEY_ENTER, true);
        }
        else
        {
            cycleKeyboard(app);
        }
        break;
    case ButtonGesture::LongRepeat:
        if (slot == 1)
        {
            cycleKeyboard(app);
        }
        break;
    default:
        break;
    }
}
#endif

} // namespace

void InputRouter::init()
{
    Heltec::MeshCore::Dal::MomentaryButton::setGestureHandler(
        +[](uint8_t slot, ButtonGesture gesture)
        { InputRouter::onButtonGesture(slot, gesture, millis()); });
    Heltec::MeshCore::Dal::KeyInput::set_poll_hook(
        Heltec::MeshCore::Dal::MomentaryButton::poll);
    Heltec::MeshCore::Dal::KeyInput::set_down_state_provider(
        Heltec::MeshCore::Dal::MomentaryButton::anyPhysicalPressed);
    (void)Heltec::MeshCore::Dal::KeyInput::initialize();
}

void InputRouter::onButtonGesture(uint8_t slot, ButtonGesture gesture, uint32_t now_ms)
{
    UiApp &app = UiApp::instance();
    if (BacklightPolicy::handle(app, now_ms))
    {
        return;
    }
    routeButton(app, slot, gesture);
}

void InputRouter::onSyntheticKey(uint32_t lv_key, bool pulse)
{
    UiApp &app = UiApp::instance();
    if (BacklightPolicy::handle(app, millis()))
    {
        return;
    }

    if (lv_key == LV_KEY_NEXT)
    {
        lv_key = LV_KEY_DOWN;
    }
    else if (lv_key == LV_KEY_PREV)
    {
        lv_key = LV_KEY_UP;
    }
    postKey(lv_key, pulse);
}

} // namespace Heltec::MeshCore::Ui
