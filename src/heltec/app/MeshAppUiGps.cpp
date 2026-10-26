#include "HeltecMesh.h"
#include "config/NodePrefs.h"
#include "MeshAppUi.hpp"
#include "Target.h"
#include "app/PowerMgr.hpp"
#include "ui/core/AppStateNotifier.hpp"

#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>
#include <heltec/sensors/GpsRmcSpeedStream.h>

namespace Heltec::MeshCore::Biz
{
bool MeshAppUi::applyGpsPowerPolicy()
{
#if !defined(ENV_INCLUDE_GPS) || !(ENV_INCLUDE_GPS)
    _power.setGpsAllowed(false);
    _power.setGpsDemand(Power::GpsDemand::Track, false);
    _power.setGpsDemand(Power::GpsDemand::LocationContinuous, false);
    _power.setGpsDemand(Power::GpsDemand::LocationAcquire, false);
    _power.setGpsDemand(Power::GpsDemand::FindFriend, false);
    return true;
#else
    NodePrefs *p = the_mesh.getNodePrefs();
    LocationProvider *location = sensors.getLocationProvider();
    if (!p || !location)
        return false;

    const uint32_t now_ms = millis();
    const auto &team_service = the_mesh.teamService();
    const bool team_share = team_service.enabled();
    const bool gps_enabled = p->gps_enabled != 0;

    const bool track_active = gpsTrackRecording();
    const bool find_friend_enabled =
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
        findFriendEnabled();
#else
        false;
#endif
    // Active friend navigation needs a continuous fix. Keep GPS powered while
    // this screen is foreground even if the backlight times out.
    const bool find_friend_foreground = _find_friend.foreground_active && find_friend_enabled;

    _power.setGpsAllowed(gps_enabled);
    _power.setGpsDemand(Power::GpsDemand::Track, track_active);
    _power.setGpsDemand(Power::GpsDemand::LocationContinuous, team_share);
    _power.setGpsDemand(Power::GpsDemand::LocationAcquire, false);
    _power.setGpsDemand(Power::GpsDemand::FindFriend, find_friend_foreground);
    return _power.reconcileGpsPower(now_ms);
#endif
}

void MeshAppUi::reconcileGpsPower()
{
    (void)applyGpsPowerPolicy();
}

void MeshAppUi::setFindFriendForegroundActive(bool active)
{
    if (_find_friend.foreground_active != active)
        resetFindFriendNavState();
    _find_friend.foreground_active = active;
    setCompassDemand(kCompassDemandFindFriend, active);
    if (active)
    {
        reconcileGpsPower();
    }
}

void MeshAppUi::setGpsForegroundActive(bool active)
{
    _power.setGpsDemand(Power::GpsDemand::GpsScreen, active);
    if (active)
    {
        (void)_power.reconcileGpsPower(millis());
    }
}

void MeshAppUi::setMapForegroundActive(bool active)
{
    _power.setGpsDemand(Power::GpsDemand::MapScreen, active);
    if (active)
    {
        (void)_power.reconcileGpsPower(millis());
    }
}

void MeshAppUi::notifyGpsChanged()
{
    Heltec::MeshCore::Ui::AppStateEvent event{};
    event.type = Heltec::MeshCore::Ui::AppStateEventType::GpsChanged;
    Heltec::MeshCore::Ui::app_state_notifier().notify(event);
}

void MeshAppUi::setGpsEnabled(bool enabled)
{
    NodePrefs *p = the_mesh.getNodePrefs();
    if (!p)
        return;

    const bool pref_on = p->gps_enabled != 0;
    if (pref_on == enabled)
    {
        reconcileGpsPower();
        return;
    }

    p->gps_enabled = enabled ? 1 : 0;
    if (!applyGpsPowerPolicy())
    {
        p->gps_enabled = pref_on ? 1 : 0;
        applyGpsPowerPolicy();
        return;
    }

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (!enabled)
    {
        if (gpsTrackRecording())
        {
            setGpsTrackRecording(false);
        }
        else if (p->gps_track_armed)
        {
            p->gps_track_armed = 0;
            the_mesh.savePrefs();
        }
    }
#endif

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (!enabled && findFriendEnabled())
    {
        (void)setFindFriendEnabled(false);
    }
#endif

    the_mesh.savePrefs();
    notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::ConfigChanged);
    notifyGpsChanged();
}

IBizFacade::GpsStatus MeshAppUi::gpsStatus() const
{
    GpsStatus s;
    LocationProvider *nmea = sensors.getLocationProvider();
    s.available = (nmea != nullptr);

    NodePrefs *p = the_mesh.getNodePrefs();
    if (p)
    {
        s.enabled = p->gps_enabled != 0;
    }
    else if (nmea)
    {
        s.enabled = nmea->isEnabled();
    }
    s.powered = _power.snapshot().gps_powered;
    if (!s.enabled || !s.powered || !nmea)
    {
        HeltecMesh::StableGpsFixSnapshot discarded{};
        (void)the_mesh.getStableGpsFix(discarded);
        return s;
    }

    HeltecMesh::StableGpsFixSnapshot snapshot{};
    s.fix_valid = the_mesh.getStableGpsFix(snapshot);
    s.fix_valid_ms = snapshot.age_ms;
    if (!s.fix_valid)
    {
        return s;
    }

    s.satellites =
        snapshot.satellites < 0
            ? 0
            : (snapshot.satellites > 255 ? 255 : static_cast<uint8_t>(snapshot.satellites));
    s.lat_micro = snapshot.lat_micro;
    s.lon_micro = snapshot.lon_micro;
    s.alt_m = snapshot.alt_milli / 1000.0;
    s.lat_deg = s.lat_micro / 1000000.0;
    s.lon_deg = s.lon_micro / 1000000.0;

    float speed_kph = -1.0f;
    if (Heltec::Gps::rmcSpeedKph(speed_kph))
        s.speed_kph = speed_kph;

    return s;
}

} // namespace Heltec::MeshCore::Biz
