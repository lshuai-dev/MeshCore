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

void MeshAppUi::handlePowerChanged(
    heltec::meshcore::power::PowerChangeMask changes,
    const heltec::meshcore::power::PowerSnapshot& snapshot) {
  using heltec::meshcore::power::PowerChange;
  using heltec::meshcore::power::hasChange;
  if (hasChange(changes, PowerChange::Battery) ||
      hasChange(changes, PowerChange::Source)) {
    heltec::meshcore::ui::AppStateEvent ev{};
    ev.type = heltec::meshcore::ui::AppStateEventType::BatteryChanged;
    ev.battery.millivolts = snapshot.battery_mv;
    ev.battery.percent = snapshot.battery_percent;
    ev.battery.charging = false;
    ev.battery.source_known = snapshot.source != mesh::PowerSource::Unknown;
    ev.battery.external_powered = snapshot.source == mesh::PowerSource::External;
    heltec::meshcore::ui::app_state_notifier().notify(ev);
  }
  if (hasChange(changes, PowerChange::Gps)) notifyGpsChanged();
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
