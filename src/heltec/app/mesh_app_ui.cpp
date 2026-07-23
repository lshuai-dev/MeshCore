#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "ui/app/ui_app.hpp"
#include "ui/core/app_state_notifier.hpp"
#include "ui/core/ui_task.hpp"

namespace heltec::meshcore::biz {

void MeshAppUi::pollRuntime() {
  pollGpsTrack();
  pollRadioStatus();
}

void MeshAppUi::notifyAppState(heltec::meshcore::ui::AppStateEventType type) {
  heltec::meshcore::ui::AppStateEvent ev{};
  ev.type = type;
  heltec::meshcore::ui::app_state_notifier().notify(ev);
}

void MeshAppUi::notifyCompanionChanged() {
  heltec::meshcore::ui::AppStateEvent ev{};
  ev.type = heltec::meshcore::ui::AppStateEventType::CompanionChanged;
  ev.companion.connected = heltec::meshcore::ui::ui_task().hasConnection();
  ev.companion.pairing_pin = the_mesh.getBLEPin();
  heltec::meshcore::ui::app_state_notifier().notify(ev);
}

void MeshAppUi::showAlert(const char* text, int duration_ms) {
  heltec::meshcore::ui::ui_task().showAlert(text, duration_ms);
}

void MeshAppUi::dismissMessagePreview() {
  heltec::meshcore::ui::UiApp::instance().closePreviewOverlay();
}

void MeshAppUi::requestWaypointManualInput() {
  heltec::meshcore::ui::UiApp::instance().openWaypointKeyboard();
}

void MeshAppUi::notifyWaypointKeyboardClosed() {}

void MeshAppUi::requestCloseSendMessageOverlay() {
  heltec::meshcore::ui::UiApp::instance().closeSendMessageOverlay();
}

#if !defined(ENV_INCLUDE_COMPASS) || !(ENV_INCLUDE_COMPASS)
void MeshAppUi::syncCompassCache() {}
bool MeshAppUi::compassHasHardware() const { return false; }
void MeshAppUi::beginCompassCalibration() {}
void MeshAppUi::endCompassCalibration() {}
bool MeshAppUi::compassHasStoredCalibration() const { return false; }
bool MeshAppUi::saveCompassCalibration() { return false; }
bool MeshAppUi::restoreCompassCalibration() { return false; }
#endif

}  // namespace heltec::meshcore::biz
