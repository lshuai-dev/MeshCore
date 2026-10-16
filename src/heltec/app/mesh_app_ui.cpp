#include "mesh_app_ui.hpp"

#include "heltec_mesh.h"
#include "ui/app/app.hpp"
#include "ui/core/app_state_notifier.hpp"
#include "ui/core/ui_task.hpp"

namespace heltec::meshcore::biz
{

MeshAppUi::MeshAppUi(heltec::meshcore::power::PowerMgr &power) : _power(power)
{
    heltec::meshcore::ui::app_state_notifier().bind(handleAppStateEvent, this);
}

void MeshAppUi::handleAppStateEvent(void *user_data,
                                    const heltec::meshcore::ui::AppStateEvent &event)
{
    auto *const app = static_cast<MeshAppUi *>(user_data);
    if (!app || event.type != heltec::meshcore::ui::AppStateEventType::GpsChanged)
    {
        return;
    }
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    the_mesh.teamService().notifyLocationChanged(event.gps.fix_valid,
        static_cast<int32_t>(event.gps.lat_micro),
        static_cast<int32_t>(event.gps.lon_micro));
#endif
}

void MeshAppUi::pollRuntime()
{
    pollGpsTrack();
    pollRadioStatus();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    pollTeamGeofenceAlerts();
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    // Non-overlapping fields: simultaneous changes must not cancel each other.
    const uint32_t team_fingerprint =
        static_cast<uint32_t>(the_mesh.teamService().changeSequence()) |
        (static_cast<uint32_t>(the_mesh.teamJoinService().state()) << 16) |
        (static_cast<uint32_t>(the_mesh.teamJoinService().offerCount()) << 24) |
        (the_mesh.teamJoinService().joinWindowOpen() ? 0x40000000UL : 0UL) |
        (the_mesh.teamService().locationShareEnabled() ? 0x20000000UL : 0UL) |
        (the_mesh.teamService().enabled() ? 0x80000000UL : 0UL);
    if (!_team_runtime_fingerprint_valid || team_fingerprint != _team_runtime_fingerprint)
    {
        _team_runtime_fingerprint = team_fingerprint;
        _team_runtime_fingerprint_valid = true;
        notifyAppState(heltec::meshcore::ui::AppStateEventType::TeamChanged);
        notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
    }
#endif
}

void MeshAppUi::handlePowerChanged(heltec::meshcore::power::PowerChangeMask changes,
                                   const heltec::meshcore::power::PowerSnapshot &snapshot)
{
    using heltec::meshcore::power::hasChange;
    using heltec::meshcore::power::PowerChange;
    if (hasChange(changes, PowerChange::Battery) || hasChange(changes, PowerChange::Source))
    {
        heltec::meshcore::ui::AppStateEvent ev{};
        ev.type = heltec::meshcore::ui::AppStateEventType::BatteryChanged;
        ev.battery.millivolts = snapshot.battery_mv;
        ev.battery.percent = snapshot.battery_percent;
        ev.battery.charging = false;
        ev.battery.source_known = snapshot.source != mesh::PowerSource::Unknown;
        ev.battery.external_powered = snapshot.source == mesh::PowerSource::External;
        heltec::meshcore::ui::app_state_notifier().notify(ev);
    }
    if (hasChange(changes, PowerChange::Gps))
        notifyGpsChanged();
}

void MeshAppUi::notifyAppState(heltec::meshcore::ui::AppStateEventType type)
{
    heltec::meshcore::ui::AppStateEvent ev{};
    ev.type = type;
    heltec::meshcore::ui::app_state_notifier().notify(ev);
}

void MeshAppUi::notifyCompanionChanged()
{
    heltec::meshcore::ui::AppStateEvent ev{};
    ev.type = heltec::meshcore::ui::AppStateEventType::CompanionChanged;
    ev.companion.connected = heltec::meshcore::ui::ui_task().hasConnection();
    ev.companion.pairing_pin = the_mesh.getBLEPin();
    heltec::meshcore::ui::app_state_notifier().notify(ev);
}

void MeshAppUi::showAlert(const char *text, int duration_ms)
{
    heltec::meshcore::ui::ui_task().showAlert(text, duration_ms);
}

#if !defined(ENV_INCLUDE_COMPASS) || !(ENV_INCLUDE_COMPASS)
void MeshAppUi::syncCompassCache() const
{
}
bool MeshAppUi::compassHasHardware() const
{
    return false;
}
void MeshAppUi::beginCompassCalibration()
{
}
void MeshAppUi::endCompassCalibration()
{
}
bool MeshAppUi::compassHasStoredCalibration() const
{
    return false;
}
bool MeshAppUi::saveCompassCalibration()
{
    return false;
}
bool MeshAppUi::restoreCompassCalibration()
{
    return false;
}
#endif

} // namespace heltec::meshcore::biz
