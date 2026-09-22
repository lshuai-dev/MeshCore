#include "ui/app/app.hpp"
#include "ui/core/ht_event.h"
#include "ui/core/task.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/e213_port.hpp"
#endif

#include <Arduino.h>

namespace heltec::meshcore::ui
{

bool UiApp::openActionPane()
{
    if (!_inited || !inputOnActiveScreen())
        return false;
#if HELTEC_HAS_TOUCH
    if (_surfaces.isVisible(&_quickPingOverlay))
        return true;
    return _surfaces.show(&_quickPingOverlay, true);
#else
    return openContextMenu();
#endif
}

bool UiApp::closeActionPane()
{
#if HELTEC_HAS_TOUCH
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
#if HELTEC_HAS_TOUCH
    return _surfaces.isVisible(&_quickPingOverlay);
#else
    return _surfaces.isVisible(&_contextMenu);
#endif
}

#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
namespace
{
void cm_send_advert(biz::IBizFacade &app, _lv_obj_t *)
{
    (void)app.sendAdvert();
}
void cm_open_send_message(biz::IBizFacade &, _lv_obj_t *target)
{
    (void)ht_event_send(target, EventType::SendMessageOpen);
}
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void cm_calibrate_compass(biz::IBizFacade &, _lv_obj_t *target)
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
    if (!_alertOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_confirmOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_radioParamSyncOvl.init(_layerOverlay, _frame_root))
        return false;
    if (!_repeatModeOvl.init(_layerOverlay, _frame_root))
        return false;
#if UI_INCLUDE_KEYBOARD_OVERLAY
    if (!_keyboardOvl.init(_layerOverlay, _frame_root))
        return false;
#endif
    if (!_sendMessageOvl.init(_layerOverlay, _frame_root))
        return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    if (!_calibrationOvl.init(_layerOverlay, _frame_root))
        return false;
#endif
#if HELTEC_HAS_TOUCH
    if (!_quickPingOverlay.init(_layerOverlay, _frame_root))
        return false;
#else
    if (!_contextMenu.init(_layerOverlay, _frame_root))
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
    heltec::meshcore::dal::e213::requestRefresh();
#endif
}

void UiApp::closePreviewOverlay()
{
    const bool was_present = _surfaces.isVisible(&_previewOvl);
    const bool dismissed = _surfaces.show(&_previewOvl, false);
    if (!was_present || dismissed)
        ui_task().dismissPreview();
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
        heltec::meshcore::dal::e213::requestRefresh();
#endif
        return;
    }
    const bool shown = _surfaces.show(&_alertOvl, true);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    if (shown)
    {
        heltec::meshcore::dal::e213::requestRefresh();
    }
#else
    (void)shown;
#endif
}

void UiApp::closeAlertOverlay()
{
    const bool was_present = _surfaces.isVisible(&_alertOvl);
    const bool dismissed = _surfaces.show(&_alertOvl, false);
    if (!was_present || dismissed)
        ui_task().dismissAlert();
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
        heltec::meshcore::dal::e213::requestRefresh();
    }
#endif
    return shown;
}

void UiApp::closeSendMessageOverlay()
{
#if UI_INCLUDE_KEYBOARD_OVERLAY
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
        _biz.showAlert("Unable to open frequency picker", 2000);
    }
}

void UiApp::closeRepeatModeOverlay()
{
    (void)_surfaces.show(&_repeatModeOvl, false);
}

void UiApp::presentMessageKeyboard()
{
#if UI_INCLUDE_KEYBOARD_OVERLAY
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
        heltec::meshcore::dal::e213::requestRefresh();
    }
#else
    (void)shown;
#endif
#endif
}

void UiApp::openWaypointKeyboard()
{
#if UI_INCLUDE_KEYBOARD_OVERLAY
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
        heltec::meshcore::dal::e213::requestRefresh();
    }
#else
    (void)shown;
#endif
#endif
}

void UiApp::closeKeyboardOverlay()
{
#if UI_INCLUDE_KEYBOARD_OVERLAY
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

#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

bool UiApp::registerRadioContextMenu()
{
    (void)_ctxRadioMenu.addCommandHandler("send message", cm_open_send_message);
    (void)_ctxRadioMenu.addCommandHandler("send advert", cm_send_advert);
    return _contextMenu.registerMenu("QuickPing", _scrRadio.icon(), _ctxRadioMenu);
}

bool UiApp::registerCompassContextMenu()
{
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    (void)_ctxCompassMenu.addCommandHandler("calibrate", cm_calibrate_compass);
    return _contextMenu.registerMenu(_scrCompass.title(), _scrCompass.icon(), _ctxCompassMenu);
#else
    return true;
#endif
}

void UiApp::ensureContextMenusRegistered()
{
    if (_context_menus_registered)
        return;

    _contextMenu.beginRegister();
    const bool radio_ok = registerRadioContextMenu();
    const bool compass_ok = registerCompassContextMenu();

    _contextMenu.endRegister();
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
    _contextMenu.leaveMenuLeaf();
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

} // namespace heltec::meshcore::ui
