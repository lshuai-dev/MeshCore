#include "MeshAppUi.hpp"

#include "HeltecMesh.h"
#include "ui/core/AppStateNotifier.hpp"
#include "ui/core/UiTask.hpp"

namespace Heltec::MeshCore::Biz
{

MeshAppUi::MeshAppUi(Heltec::MeshCore::Power::PowerMgr &power) : _power(power)
{
    Heltec::MeshCore::Ui::app_state_notifier().bind(handleAppStateEvent, this);
}

void MeshAppUi::handleAppStateEvent(void *user_data,
                                    const Heltec::MeshCore::Ui::AppStateEvent &event)
{
    auto *const app = static_cast<MeshAppUi *>(user_data);
    if (!app || event.type != Heltec::MeshCore::Ui::AppStateEventType::GpsChanged)
    {
        return;
    }
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    const GpsStatus gps = app->gpsStatus();
    the_mesh.teamService().notifyLocationChanged(
        gps.fix_valid, static_cast<int32_t>(gps.lat_micro),
        static_cast<int32_t>(gps.lon_micro));
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
        notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::TeamChanged);
        notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::FindFriendChanged);
    }
#endif
}

void MeshAppUi::handlePowerChanged(Heltec::MeshCore::Power::PowerChangeMask changes,
                                   const Heltec::MeshCore::Power::PowerSnapshot &)
{
    using Heltec::MeshCore::Power::hasChange;
    using Heltec::MeshCore::Power::PowerChange;
    if (hasChange(changes, PowerChange::Gps))
        notifyGpsChanged();
}

void MeshAppUi::notifyAppState(Heltec::MeshCore::Ui::AppStateEventType type)
{
    Heltec::MeshCore::Ui::AppStateEvent ev{};
    ev.type = type;
    Heltec::MeshCore::Ui::app_state_notifier().notify(ev);
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

} // namespace Heltec::MeshCore::Biz
