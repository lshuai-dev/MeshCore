#include "ui/app/ui_app.hpp"

#include "ui/core/ui_task.hpp"
#include "ui/core/ui_events.h"
#include <Arduino.h>

#if defined(MESH_DEBUG) && MESH_DEBUG
#define UI_ALERT_LOG(fmt, ...) \
  do { \
    Serial.printf("[alert] " fmt "\\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)
#else
#define UI_ALERT_LOG(fmt, ...) ((void)0)
#endif

namespace heltec::meshcore::ui {

namespace {
void cm_send_advert(biz::IBizFacade& app) { app.sendAdvertWithFeedback(); }
void cm_open_send_message(biz::IBizFacade& app) { app.requestSendMessageOverlay(); }
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void cm_calibrate_compass(biz::IBizFacade& app) { app.requestCompassCalibration(); }
#endif
}  // namespace

bool UiApp::initOverlay() {
  if (!_layerOverlay) return false;
  if (!_previewOvl.create(_layerOverlay)) return false;
  if (!_alertOvl.create(_layerOverlay)) return false;
  if (!_radioParamSyncOvl.create(_layerOverlay)) return false;
  if (!_keyboardOvl.create(_layerOverlay)) return false;
  if (!_sendMessageOvl.create(_layerOverlay)) return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (!_calibrationOvl.create(_layerOverlay)) return false;
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (!_quickPingOverlay.create(_layerOverlay)) return false;
#else
  if (!_contextMenu.create(_layerOverlay)) return false;
  ensureContextMenusRegistered();
  if (!_context_menus_registered || !_contextMenu.canOpen()) return false;
#endif
  return true;
}

void UiApp::openPreviewOverlay(uint8_t unread, uint32_t age_sec, const char* origin, const char* text) {
  if (!_inited) return;
  _previewOvl.applyContent(unread, age_sec, origin, text);
  if (_surfaces.isActive(&_previewOvl)) return;
  (void)_surfaces.present(&_previewOvl, nullptr);
}

void UiApp::closePreviewOverlay() {
  const bool was_present = _surfaces.contains(&_previewOvl);
  const bool dismissed = _surfaces.dismissBranch(&_previewOvl);
  if (!was_present || dismissed) ui_task().dismissPreview();
}

void UiApp::openAlertOverlay(const char* text) {
  if (!_inited) return;
  _alertOvl.setText(text);
  UI_ALERT_LOG("open text=%s now=%lu active=%d contains=%d depth=%u active_surface=%p",
               text ? text : "",
               (unsigned long)millis(),
               _surfaces.isActive(&_alertOvl) ? 1 : 0,
               _surfaces.contains(&_alertOvl) ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.active());
  if (_surfaces.contains(&_alertOvl)) {
    const bool ok = _surfaces.raise(&_alertOvl);
    UI_ALERT_LOG("raise ok=%d depth=%u active=%p",
                 ok ? 1 : 0,
                 (unsigned)_surfaces.modalDepth(),
                 _surfaces.active());
    return;
  }
  const bool ok = _surfaces.present(&_alertOvl, nullptr);
  UI_ALERT_LOG("present ok=%d depth=%u active=%p",
               ok ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.active());
}

void UiApp::closeAlertOverlay() {
  const bool was_present = _surfaces.contains(&_alertOvl);
  UI_ALERT_LOG("close begin now=%lu was_present=%d active=%d depth=%u active_surface=%p",
               (unsigned long)millis(),
               was_present ? 1 : 0,
               _surfaces.isActive(&_alertOvl) ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.active());
  const bool dismissed = _surfaces.dismissBranch(&_alertOvl);
  UI_ALERT_LOG("close dismissed=%d depth=%u contains=%d active_surface=%p",
               dismissed ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.contains(&_alertOvl) ? 1 : 0,
               _surfaces.active());
  if (!was_present || dismissed) ui_task().dismissAlert();
}

bool UiApp::openSendMessageOverlay() {
  if (!_inited) return false;
  if (_surfaces.isActive(&_sendMessageOvl)) return true;
  return _surfaces.present(&_sendMessageOvl);
}

void UiApp::closeSendMessageOverlay() {
  (void)_surfaces.dismissBranch(&_sendMessageOvl);
}

void UiApp::openRadioParamSyncOverlay() {
  if (!_inited) return;
  if (_surfaces.isActive(&_radioParamSyncOvl)) return;
  notifyDisplayActivity(millis());
  (void)_surfaces.present(&_radioParamSyncOvl);
}

void UiApp::closeRadioParamSyncOverlay() {
  (void)_surfaces.dismissBranch(&_radioParamSyncOvl);
}

void UiApp::presentMessageKeyboard() {
  if (!_inited) return;
  const bool send_active = _surfaces.isActive(&_sendMessageOvl);
  if (!send_active) return;
  if (_surfaces.isActive(&_keyboardOvl)) return;
  (void)_surfaces.present(&_keyboardOvl, &_sendMessageOvl);
}

void UiApp::openWaypointKeyboard() {
  if (!_inited) return;
  UiSurface* const owner = _surfaces.active();
  if (!_keyboardOvl.prepareWaypointInput()) return;
  if (_surfaces.isActive(&_keyboardOvl)) return;
  (void)_surfaces.present(&_keyboardOvl, owner);
}

void UiApp::closeKeyboardOverlay() {
  if (!_surfaces.isActive(&_keyboardOvl)) return;
  const bool waypoint = _keyboardOvl.isWaypointCompose();
  (void)_surfaces.dismiss(&_keyboardOvl);
  if (waypoint) {
    if (AbstractScreen* scr = activeScreen()) scr->onWaypointKeyboardClosed();
  }
}

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

bool UiApp::registerRadioContextMenu() {
  (void)_ctxRadioMenu.addCommandHandler("send message", cm_open_send_message);
  (void)_ctxRadioMenu.addCommandHandler("send advert", cm_send_advert);
  return _contextMenu.registerMenu("QuickPing", _scrRadio.icon(), _ctxRadioMenu);
}

bool UiApp::registerCompassContextMenu() {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  (void)_ctxCompassMenu.addCommandHandler("calibrate", cm_calibrate_compass);
  return _contextMenu.registerMenu(
      _scrCompass.title(), _scrCompass.icon(), _ctxCompassMenu);
#else
  return true;
#endif
}

void UiApp::ensureContextMenusRegistered() {
  if (_context_menus_registered) return;

  _contextMenu.beginRegister();

  const bool radio_ok = registerRadioContextMenu();
  const bool compass_ok = registerCompassContextMenu();

  _contextMenu.endRegister();
  _context_menus_registered = radio_ok && compass_ok;
}

bool UiApp::openContextMenu() {
  if (!_inited || !_frame_root || _surfaces.contains(&_contextMenu)) return false;
  return ui_event_send(_frame_root, UiEventType::ContextOpen);
}

void UiApp::dismissTopContextMenu() {
  if (!_surfaces.contains(&_contextMenu)) {
    return;
  }
  _contextMenu.leaveMenuLeaf();
  (void)_surfaces.dismiss(&_contextMenu);
}

void UiApp::dismissContextMenuStack() {
  if (!_surfaces.contains(&_contextMenu)) {
    return;
  }
  _contextMenu.leaveMenuLeaf();
  (void)_surfaces.dismissBranch(&_contextMenu);
}

#endif

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

bool UiApp::openCalibrationOverlay() {
  if (!_inited) return false;
  return _surfaces.present(&_calibrationOvl);
}

void UiApp::closeCalibrationOverlay() {
  if (!_surfaces.contains(&_calibrationOvl)) return;
  _scrCompass.skipAutoCalibrationOnce();
  if (!_surfaces.dismissBranch(&_calibrationOvl)) return;
}

#endif

}  // namespace heltec::meshcore::ui
