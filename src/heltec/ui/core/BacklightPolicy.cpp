#include "BacklightPolicy.hpp"

#include "ui/app/App.hpp"

namespace Heltec::MeshCore::Ui
{

BacklightMode BacklightPolicy::mode()
{
#if defined(HELTEC_INPUT_BACKLIGHT_MANUAL_TOGGLE) && HELTEC_INPUT_BACKLIGHT_MANUAL_TOGGLE
    return BacklightMode::ManualToggle;
#else
    return BacklightMode::AutoTimeout;
#endif
}

bool BacklightPolicy::handle(UiApp &app, uint32_t now_ms)
{
    const bool backlight_on = app.isDisplayOn();

    switch (mode())
    {
    case BacklightMode::ManualToggle:
        if (!backlight_on)
            return true;
        break;

    case BacklightMode::AutoTimeout:
    default:
        if (!backlight_on)
        {
            app.notifyDisplayActivity(now_ms);
            return true;
        }
        app.notifyDisplayActivity(now_ms);
        break;
    }

    return false;
}

} // namespace Heltec::MeshCore::Ui
