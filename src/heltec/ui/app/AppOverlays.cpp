#include "ui/app/App.hpp"
#include "ui/core/HtEvent.h"
#include "ui/core/UiTask.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/DisplayPort.hpp"
#endif

#include <Arduino.h>

namespace Heltec::MeshCore::Ui
{

bool UiApp::openActionPane()
{
    if (!_inited || !inputOnActiveScreen())
        return false;
#if HELTEC_UI_HAS_TOUCH
    if (_surfaces.isVisible(&_quickPingOverlay))
        return true;
    return _surfaces.show(&_quickPingOverlay, true);
#else
    return openContextMenu();
#endif
}

bool UiApp::closeActionPane()
{
#if HELTEC_UI_HAS_TOUCH
    if (!_surfaces.isVisible(&_quickPingOverlay))
        return false;
    if (_quickPingOverlay.requestCloseAnimation())
        return true;
    return _surfaces.show(&_quickPingOverlay, false);
#else
    const bool visible = _surfaces.isVisible(&_contextMenu);
    closeContextMenu();
    return visible;
#endif
}

bool UiApp::isActionPaneVisible() const
{
#if HELTEC_UI_HAS_TOUCH
    return _surfaces.isVisible(&_quickPingOverlay);
#else
    return _surfaces.isVisible(&_contextMenu);
#endif
}

#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
namespace
{
void cm_send_advert(Biz::IBizFacade &app, _lv_obj_t *)
{
    (void)app.sendAdvert();
}
void cm_open_send_message(Biz::IBizFacade &, _lv_obj_t *target)
{
    (void)ht_event_send(target, EventType::SendMessageOpen);
}
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void cm_calibrate_compass(Biz::IBizFacade &, _lv_obj_t *target)
{
    (void)ht_event_send(target, EventType::CalibrationOpen);
}
#endif
} // namespace
#endif

bool UiApp::initOverlay()
{
    if (!_layerOverlay)
        return false;
    if (!_previewOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_previewOvl))
        return false;
    if (!_alertOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_alertOvl))
        return false;
    if (!_confirmOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_confirmOvl))
        return false;
    if (!_radioParamSyncOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_radioParamSyncOvl))
        return false;
    if (!_repeatModeOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_repeatModeOvl))
        return false;
#if HELTEC_UI_ENABLE_KEYBOARD
    if (!_keyboardOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_keyboardOvl))
        return false;
#endif
    if (!_sendMessageOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_sendMessageOvl))
        return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    if (!_calibrationOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_calibrationOvl))
        return false;
#endif
#if HELTEC_UI_HAS_TOUCH
    if (!_quickPingOverlay.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_quickPingOverlay))
        return false;
#else
    if (!_contextMenu.init(_layerOverlay, _frame_root))
        return false;
    if (!_surfaces.registerSurface(&_contextMenu))
        return false;
    ensureContextMenusRegistered();
    if (!_context_menus_registered || !_contextMenu.canOpen())
        return false;
#endif
    return true;
}

void UiApp::openPreviewOverlay(uint8_t unread, uint32_t received_ms, const char *origin,
                               const char *text)
{
    if (!_inited)
        return;
    _previewOvl.applyContent(unread, received_ms, origin, text);
    (void)_surfaces.show(&_previewOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
#endif
}

void UiApp::openAlertOverlay(const char *text)
{
    if (!_inited)
        return;
    _alertOvl.setText(text);
    if (_surfaces.isVisible(&_alertOvl))
    {
        (void)_surfaces.show(&_alertOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
        Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
#endif
        return;
    }
    const bool shown = _surfaces.show(&_alertOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    if (shown)
    {
        Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
    }
#else
    (void)shown;
#endif
}

bool UiApp::openSendMessageOverlay()
{
    if (!_inited)
        return false;
    if (_surfaces.isVisible(&_sendMessageOvl))
        return true;
    const bool shown = _surfaces.show(&_sendMessageOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    if (shown)
    {
        // Arm the panel only after the deferred surface mutation has happened,
        // so the request waits for the Send Message frame rather than an older
        // ContextMenu flush cycle.
        Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
    }
#endif
    return shown;
}

void UiApp::closeSendMessageOverlay()
{
#if HELTEC_UI_ENABLE_KEYBOARD
    if (_surfaces.isVisible(&_keyboardOvl))
    {
        (void)_surfaces.show(&_keyboardOvl, false);
    }
#endif
    (void)_surfaces.show(&_sendMessageOvl, false);
}

void UiApp::openRadioParamSyncOverlay()
{
    if (!_inited)
        return;
    if (_surfaces.isVisible(&_radioParamSyncOvl))
        return;
    notifyDisplayActivity(millis());
    (void)_surfaces.show(&_radioParamSyncOvl, true);
}

void UiApp::closeRadioParamSyncOverlay()
{
    (void)_surfaces.show(&_radioParamSyncOvl, false);
}

void UiApp::openRepeatModeOverlay()
{
    if (!_inited || _surfaces.isVisible(&_repeatModeOvl))
        return;
    notifyDisplayActivity(millis());
    if (!_surfaces.show(&_repeatModeOvl, true))
    {
        ui_task().showAlert("Unable to open frequency picker", 2000);
    }
}

void UiApp::closeRepeatModeOverlay()
{
    (void)_surfaces.show(&_repeatModeOvl, false);
}

void UiApp::presentMessageKeyboard()
{
#if HELTEC_UI_ENABLE_KEYBOARD
    if (!_inited)
        return;
    const bool send_active = _surfaces.isActive(&_sendMessageOvl);
    const bool preset_active = activeScreen() == &_scrSystem;
    if (!send_active && !preset_active)
        return;
    if (_surfaces.isVisible(&_keyboardOvl))
        return;
    const bool shown = _surfaces.show(&_keyboardOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    if (shown)
    {
        Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
    }
#else
    (void)shown;
#endif
#endif
}

void UiApp::openWaypointKeyboard()
{
#if HELTEC_UI_ENABLE_KEYBOARD
    if (!_inited)
        return;
    if (!_keyboardOvl.prepareWaypointInput())
        return;
    if (_surfaces.isVisible(&_keyboardOvl))
        return;
    const bool shown = _surfaces.show(&_keyboardOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    if (shown)
    {
        Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
    }
#else
    (void)shown;
#endif
#endif
}

void UiApp::closeKeyboardOverlay()
{
#if HELTEC_UI_ENABLE_KEYBOARD
    if (!_surfaces.isVisible(&_keyboardOvl))
        return;
    const bool waypoint = _keyboardOvl.isWaypointCompose();
    (void)_surfaces.show(&_keyboardOvl, false);
    if (waypoint)
    {
        _surfaces.dispatchToActive(EventType::WaypointKeyboardClosed);
    }
#endif
}

#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH

bool UiApp::registerRadioContextMenu()
{
    static const ContextMenuItem items[] = {
        { "send message", cm_open_send_message, nullptr },
        { "send advert", cm_send_advert, nullptr },
    };
    static const ContextMenuPage page = {
        "QuickPing",
        items,
        static_cast<uint8_t>(sizeof(items) / sizeof(items[0])),
    };
    return _contextMenu.registerMenu(_scrRadio.icon(), page);
}

bool UiApp::registerCompassContextMenu()
{
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    static const ContextMenuItem items[] = {
        { "calibrate", cm_calibrate_compass, nullptr },
    };
    static const ContextMenuPage page = {
        "Compass",
        items,
        static_cast<uint8_t>(sizeof(items) / sizeof(items[0])),
    };
    return _contextMenu.registerMenu(_scrCompass.icon(), page);
#else
    return true;
#endif
}

void UiApp::ensureContextMenusRegistered()
{
    if (_context_menus_registered)
        return;

    const bool radio_ok = registerRadioContextMenu();
    const bool compass_ok = registerCompassContextMenu();
    _context_menus_registered = radio_ok && compass_ok;
}

bool UiApp::openContextMenu()
{
    if (!_inited || !_frame_root || _surfaces.isVisible(&_contextMenu))
        return false;
    return ht_event_send(_frame_root, EventType::ContextOpen);
}

void UiApp::closeContextMenu()
{
    if (!_surfaces.isVisible(&_contextMenu))
    {
        return;
    }
    (void)_surfaces.show(&_contextMenu, false);
}

#endif

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

bool UiApp::openCalibrationOverlay()
{
    if (!_inited)
        return false;
    return _surfaces.show(&_calibrationOvl, true);
}

void UiApp::closeCalibrationOverlay()
{
    if (!_surfaces.isVisible(&_calibrationOvl))
        return;
    _scrCompass.skipAutoCalibrationOnce();
    if (!_surfaces.show(&_calibrationOvl, false))
        return;
}

#endif

} // namespace Heltec::MeshCore::Ui
