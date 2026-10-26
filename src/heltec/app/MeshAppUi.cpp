#include "MeshAppUi.hpp"

#include "HeltecMesh.h"
#include "app/PowerMgr.hpp"
#include "ui/core/AppStateNotifier.hpp"

#include <Arduino.h>

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include <heltec/sensors/IcmCompassProvider.h>
extern ICMCompassProvider compassProvider;
#endif

namespace Heltec::MeshCore::Biz
{

MeshAppUi::MeshAppUi(Heltec::MeshCore::Power::PowerMgr &power) : _power(power) {}

void MeshAppUi::poll()
{
    reconcileCompassSampling(millis());
    pollGpsTrack();
    pollRadioStatus();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    pollTeamGeofenceAlerts();
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    constexpr uint32_t kTeamPollPeriodMs = 500U;
    const uint32_t now_ms = millis();
    if (_team_runtime_valid &&
        static_cast<uint32_t>(now_ms - _team_runtime_poll_ms) < kTeamPollPeriodMs)
    {
        return;
    }
    _team_runtime_poll_ms = now_ms;

    const GpsStatus gps = gpsStatus();
    the_mesh.teamService().notifyLocationChanged(
        gps.fix_valid, static_cast<int32_t>(gps.lat_micro),
        static_cast<int32_t>(gps.lon_micro));

    TeamRuntimeState current{};
    current.sequence = the_mesh.teamService().changeSequence();
    current.join_state = static_cast<uint8_t>(the_mesh.teamJoinService().state());
    current.offer_count = static_cast<uint8_t>(the_mesh.teamJoinService().offerCount());
    current.flags = static_cast<uint8_t>(
        (the_mesh.teamJoinService().joinWindowOpen() ? 0x01U : 0U) |
        (the_mesh.teamService().locationShareEnabled() ? 0x02U : 0U) |
        (the_mesh.teamService().enabled() ? 0x04U : 0U));
    if (!_team_runtime_valid || !(current == _team_runtime))
    {
        _team_runtime = current;
        _team_runtime_valid = true;
        notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::TeamChanged);
        notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::FindFriendChanged);
    }
#endif
}

void MeshAppUi::setCompassDemand(uint8_t demand, bool active)
{
    _power.setCompassDemand(static_cast<Power::CompassDemand>(demand), active);
    if (active)
    {
        _compass_demand_mask = static_cast<uint8_t>(_compass_demand_mask | demand);
        _compass_idle_since_ms = 0;
    }
    else
    {
        _compass_demand_mask = static_cast<uint8_t>(_compass_demand_mask & static_cast<uint8_t>(~demand));
        if (_compass_demand_mask == 0U && _compass_idle_since_ms == 0U)
        {
            _compass_idle_since_ms = millis();
        }
    }
}

void MeshAppUi::reconcileCompassSampling(uint32_t now_ms)
{
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    constexpr uint32_t kCompassIdleDelayMs = 1500U;
    if (_compass_demand_mask == 0U && _compass_idle_since_ms == 0U)
    {
        _compass_idle_since_ms = now_ms;
    }
    const bool should_sample = (_power.compassDemands() != 0U) ||
                               (static_cast<uint32_t>(now_ms - _compass_idle_since_ms) < kCompassIdleDelayMs);
    if (should_sample != _compass_sampling_enabled)
    {
        compassProvider.setSamplingEnabled(should_sample);
        _compass_sampling_enabled = should_sample;
    }
#else
    (void)now_ms;
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

void MeshAppUi::setCompassForegroundActive(bool active)
{
    setCompassDemand(kCompassDemandForeground, active);
}

#if !defined(ENV_INCLUDE_COMPASS) || !(ENV_INCLUDE_COMPASS)
CompassUi MeshAppUi::compassUi() const
{
    return CompassUi{};
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
