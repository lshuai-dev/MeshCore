#include "ui/app/ui_app.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ui_task.hpp"

#include <Arduino.h>

#if defined(MESH_DEBUG) && MESH_DEBUG
#define UI_ALERT_LOG(fmt, ...)                              \
    do                                                      \
    {                                                       \
        Serial.printf("[alert] " fmt "\\n", ##__VA_ARGS__); \
        Serial.flush();                                     \
    } while (0)
#else
#define UI_ALERT_LOG(fmt, ...) ((void)0)
#endif

namespace heltec::meshcore::ui
{

namespace
{
void cm_send_advert(biz::IBizFacade &app, _lv_obj_t *)
{
    (void)app.sendAdvert();
}
void cm_open_send_message(biz::IBizFacade &, _lv_obj_t *target)
{
    (void)ui_event_send(target, UiEventType::SendMessageOpen);
}
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void cm_calibrate_compass(biz::IBizFacade &, _lv_obj_t *target)
{
    (void)ui_event_send(target, UiEventType::CalibrationOpen);
}
#endif
} // namespace

bool UiApp::initOverlay()
{
    if (!_layerOverlay)
        return false;
    if (!_previewOvl.init(_layerOverlay))
        return false;
    if (!_alertOvl.init(_layerOverlay))
        return false;
    if (!_confirmOvl.init(_layerOverlay))
        return false;
    if (!_radioParamSyncOvl.init(_layerOverlay))
        return false;
    if (!_repeatModeOvl.init(_layerOverlay))
        return false;
    if (!_keyboardOvl.init(_layerOverlay))
        return false;
    if (!_sendMessageOvl.init(_layerOverlay))
        return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    if (!_calibrationOvl.init(_layerOverlay))
        return false;
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    if (!_quickPingOverlay.init(_layerOverlay))
        return false;
#else
    if (!_contextMenu.init(_layerOverlay))
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
    if (_surfaces.isVisible(&_previewOvl))
        return;
    (void)_surfaces.show(&_previewOvl);
}

void UiApp::closePreviewOverlay()
{
    const bool was_present = _surfaces.isVisible(&_previewOvl);
    const bool dismissed = _surfaces.hide(&_previewOvl);
    if (!was_present || dismissed)
        ui_task().dismissPreview();
}

void UiApp::openAlertOverlay(const char *text)
{
    if (!_inited)
        return;
    _alertOvl.setText(text);
    UI_ALERT_LOG("open text=%s now=%lu active=%d visible=%d active_surface=%p",
                 text ? text : "", (unsigned long)millis(), _surfaces.isActive(&_alertOvl) ? 1 : 0,
                 _surfaces.isVisible(&_alertOvl) ? 1 : 0, _surfaces.active());
    if (_surfaces.isVisible(&_alertOvl))
    {
        const bool ok = _surfaces.show(&_alertOvl);
        UI_ALERT_LOG("raise ok=%d active=%p", ok ? 1 : 0, _surfaces.active());
        return;
    }
    const bool ok = _surfaces.show(&_alertOvl);
    UI_ALERT_LOG("present ok=%d active=%p", ok ? 1 : 0, _surfaces.active());
}

void UiApp::closeAlertOverlay()
{
    const bool was_present = _surfaces.isVisible(&_alertOvl);
    UI_ALERT_LOG("close begin now=%lu was_present=%d active=%d active_surface=%p",
                 (unsigned long)millis(), was_present ? 1 : 0,
                 _surfaces.isActive(&_alertOvl) ? 1 : 0, _surfaces.active());
    const bool dismissed = _surfaces.hide(&_alertOvl);
    UI_ALERT_LOG("close dismissed=%d visible=%d active_surface=%p", dismissed ? 1 : 0,
                 _surfaces.isVisible(&_alertOvl) ? 1 : 0, _surfaces.active());
    if (!was_present || dismissed)
        ui_task().dismissAlert();
}

bool UiApp::openSendMessageOverlay()
{
    if (!_inited)
        return false;
    if (_surfaces.isVisible(&_sendMessageOvl))
        return true;
    return _surfaces.show(&_sendMessageOvl);
}

void UiApp::closeSendMessageOverlay()
{
    if (_surfaces.isVisible(&_keyboardOvl))
    {
        (void)_surfaces.hide(&_keyboardOvl);
    }
    (void)_surfaces.hide(&_sendMessageOvl);
}

void UiApp::openRadioParamSyncOverlay()
{
    if (!_inited)
        return;
    if (_surfaces.isVisible(&_radioParamSyncOvl))
        return;
    notifyDisplayActivity(millis());
    (void)_surfaces.show(&_radioParamSyncOvl);
}

void UiApp::closeRadioParamSyncOverlay()
{
    (void)_surfaces.hide(&_radioParamSyncOvl);
}

void UiApp::openRepeatModeOverlay()
{
    if (!_inited || _surfaces.isVisible(&_repeatModeOvl))
        return;
    notifyDisplayActivity(millis());
    if (!_surfaces.show(&_repeatModeOvl))
    {
        _biz.showAlert("Unable to open frequency picker", 2000);
    }
}

void UiApp::closeRepeatModeOverlay()
{
    (void)_surfaces.hide(&_repeatModeOvl);
}

void UiApp::presentMessageKeyboard()
{
    if (!_inited)
        return;
    const bool send_active = _surfaces.isActive(&_sendMessageOvl);
    if (!send_active)
        return;
    if (_surfaces.isVisible(&_keyboardOvl))
        return;
    (void)_surfaces.show(&_keyboardOvl);
}

void UiApp::openWaypointKeyboard()
{
    if (!_inited)
        return;
    if (!_keyboardOvl.prepareWaypointInput())
        return;
    if (_surfaces.isVisible(&_keyboardOvl))
        return;
    (void)_surfaces.show(&_keyboardOvl);
}

void UiApp::closeKeyboardOverlay()
{
    if (!_surfaces.isVisible(&_keyboardOvl))
        return;
    const bool waypoint = _keyboardOvl.isWaypointCompose();
    (void)_surfaces.hide(&_keyboardOvl);
    if (waypoint)
    {
        _surfaces.dispatchToActive(UiEventType::WaypointKeyboardClosed);
    }
}

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

bool UiApp::registerRadioContextMenu()
{
    _ctxRadioMenu.setTarget(_frame_root);
    (void)_ctxRadioMenu.addCommandHandler("send message", cm_open_send_message);
    (void)_ctxRadioMenu.addCommandHandler("send advert", cm_send_advert);
    return _contextMenu.registerMenu("QuickPing", _scrRadio.icon(), _ctxRadioMenu);
}

bool UiApp::registerCompassContextMenu()
{
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    _ctxCompassMenu.setTarget(_frame_root);
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
    return ui_event_send(_frame_root, UiEventType::ContextOpen);
}

void UiApp::dismissTopContextMenu()
{
    if (!_surfaces.isVisible(&_contextMenu))
    {
        return;
    }
    _contextMenu.leaveMenuLeaf();
    (void)_surfaces.hide(&_contextMenu);
}

void UiApp::dismissContextMenuStack()
{
    if (!_surfaces.isVisible(&_contextMenu))
    {
        return;
    }
    _contextMenu.leaveMenuLeaf();
    (void)_surfaces.hide(&_contextMenu);
}

#endif

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

bool UiApp::openCalibrationOverlay()
{
    if (!_inited)
        return false;
    return _surfaces.show(&_calibrationOvl);
}

void UiApp::closeCalibrationOverlay()
{
    if (!_surfaces.isVisible(&_calibrationOvl))
        return;
    _scrCompass.skipAutoCalibrationOnce();
    if (!_surfaces.hide(&_calibrationOvl))
        return;
}

#endif

} // namespace heltec::meshcore::ui
