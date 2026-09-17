#include "HeltecMesh.h"
#include "config/DataStore.h"
#include "mesh_app_ui.hpp"
#include "target.h"

#include <helpers/BaseChatMesh.h>
#include <lvgl.h>
#include <math.h>
#include <string.h>

#ifndef COMPASS_HEADING_OFFSET_DEG
#define COMPASS_HEADING_OFFSET_DEG 0
#endif

namespace heltec::meshcore::biz
{
namespace
{

#if defined(COMPASS_DECLINATION_DEG)
constexpr bool kCompassDeclinationConfigured = true;
#else
constexpr bool kCompassDeclinationConfigured = false;
#endif

inline double microdeg_to_deg(int32_t micro)
{
    return (double)micro * 1.0e-6;
}

int32_t deg_to_e6(double deg)
{
    return (int32_t)(deg * 1000000.0 + (deg >= 0.0 ? 0.5 : -0.5));
}

double e6_to_deg(int32_t e6)
{
    return ((double)e6) / 1000000.0;
}

void format_e6_coord(char *buf, size_t len, int32_t e6)
{
    if (!buf || len == 0)
        return;
    const int32_t a = e6 < 0 ? -e6 : e6;
    lv_snprintf(buf, len, "%s%d.%06ld", e6 < 0 ? "-" : "", a / 1000000, (long)(a % 1000000));
}

void format_e6_pair(char *buf, size_t len, int32_t lat_e6, int32_t lon_e6)
{
    if (!buf || len == 0)
        return;
    char la[16];
    char lo[16];
    format_e6_coord(la, sizeof(la), lat_e6);
    format_e6_coord(lo, sizeof(lo), lon_e6);
    lv_snprintf(buf, len, "%s,%s", la, lo);
}

bool load_ff_settings(int &mode, int &wp_valid, double &wp_lat, double &wp_lon, uint16_t &track_cm,
                      uint16_t *track_interval_min_out, bool *enabled_out)
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    DataStore *ds = the_mesh.getDataStore();
    if (!ds)
        return false;
    return ds->loadFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, track_cm,
                                             nullptr, track_interval_min_out, enabled_out,
                                             nullptr, nullptr);
#else
    (void)mode;
    (void)wp_valid;
    (void)wp_lat;
    (void)wp_lon;
    (void)track_cm;
    (void)track_interval_min_out;
    (void)enabled_out;
    return false;
#endif
}

FindFriendTargetFreshness to_ui_freshness(nav::TargetFreshness freshness)
{
    switch (freshness)
    {
    case nav::TargetFreshness::Unknown:
        return FindFriendTargetFreshness::Unknown;
    case nav::TargetFreshness::Fresh:
        return FindFriendTargetFreshness::Fresh;
    case nav::TargetFreshness::Aging:
        return FindFriendTargetFreshness::Aging;
    case nav::TargetFreshness::Stale:
        return FindFriendTargetFreshness::Stale;
    case nav::TargetFreshness::NotApplicable:
    default:
        return FindFriendTargetFreshness::NotApplicable;
    }
}

FindFriendConfidence to_ui_confidence(nav::Confidence confidence)
{
    switch (confidence)
    {
    case nav::Confidence::Low:
        return FindFriendConfidence::Low;
    case nav::Confidence::Medium:
        return FindFriendConfidence::Medium;
    case nav::Confidence::High:
        return FindFriendConfidence::High;
    case nav::Confidence::Unavailable:
    default:
        return FindFriendConfidence::Unavailable;
    }
}

} // namespace

void MeshAppUi::persistFfPrefs(int mode, int wp_valid, double wp_lat, double wp_lon) const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    DataStore *ds = the_mesh.getDataStore();
    if (!ds)
        return;
    ds->saveFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, HELTEC_GPS_TRACK_MIN_DIST_CM,
                                      -1, _ff_gps_track_interval_min,
                                      _ff_enabled,
                                      nullptr);
#else
    (void)mode;
    (void)wp_valid;
    (void)wp_lat;
    (void)wp_lon;
#endif
}

void MeshAppUi::syncFindFriendTeamPin() const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    const uint8_t *key =
        _ff_enabled && _ff_mode == 0 && _ff_target_pub_key_valid ? _ff_target_pub_key : nullptr;
    the_mesh.teamService().pinMember(key);
#endif
}

void MeshAppUi::resetFindFriendNavState() const
{
    _ff_nav_estimator.reset();
}

void MeshAppUi::setFfWaypointCache(double lat_deg, double lon_deg)
{
    _ff_mode = 1;
    _ff_wp_valid = 1;
    _ff_wp_lat_e6 = deg_to_e6(lat_deg);
    _ff_wp_lon_e6 = deg_to_e6(lon_deg);
    syncFindFriendTeamPin();
    resetFindFriendNavState();
}

void MeshAppUi::ensureFindFriendPrefsLoaded() const
{
    if (_ff_prefs_loaded)
        return;
    _ff_prefs_loaded = true;

    int mode = 0;
    int wp_valid = 0;
    double la = 0;
    double lo = 0;
    uint16_t track_cm = 100;
    uint16_t track_interval_min = 1;
    bool enabled = false;
    const bool loaded =
        load_ff_settings(mode, wp_valid, la, lo, track_cm, &track_interval_min,
                         &enabled);
    if (!loaded)
        return;

    _ff_mode = mode ? 1 : 0;
    _ff_wp_valid = wp_valid && nav::FindFriendNavEstimator::coordinateValid(la, lo);
    _ff_wp_lat_e6 = deg_to_e6(la);
    _ff_wp_lon_e6 = deg_to_e6(lo);
    _ff_gps_track_interval_min = track_interval_min;
    _ff_enabled = enabled;

    // Team selection is volatile; never restore an ordinary contact or old Team key.
    _ff_target_contact_idx = -1;
    _ff_target_pub_key_valid = false;
    memset(_ff_target_pub_key, 0, sizeof(_ff_target_pub_key));
    syncFindFriendTeamPin();
}

bool MeshAppUi::locationShareEnabled() const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    return the_mesh.teamService().hasTeam() && the_mesh.teamService().locationShareEnabled();
#else
    return false;
#endif
}

bool MeshAppUi::findFriendEnabled() const
{
    ensureFindFriendPrefsLoaded();
    return _ff_enabled;
}

bool MeshAppUi::setFindFriendEnabled(bool enabled)
{
    ensureFindFriendPrefsLoaded();
    if (enabled && !gpsStatus().enabled)
    {
        setGpsEnabled(true);
        if (!gpsStatus().enabled)
            return false;
    }

    if (_ff_enabled == enabled)
    {
        syncFindFriendTeamPin();
        reconcileGpsPower();
        return true;
    }

    _ff_enabled = enabled;
    syncFindFriendTeamPin();
    resetFindFriendNavState();
    persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6), e6_to_deg(_ff_wp_lon_e6));
    reconcileGpsPower();
    notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
    return true;
}

int MeshAppUi::findFriendMode() const
{
    ensureFindFriendPrefsLoaded();
    return _ff_mode;
}

bool MeshAppUi::setFindFriendMode(int mode)
{
    ensureFindFriendPrefsLoaded();
    _ff_mode = mode ? 1 : 0;
    syncFindFriendTeamPin();
    resetFindFriendNavState();
    persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6), e6_to_deg(_ff_wp_lon_e6));
    notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
    return true;
}

bool MeshAppUi::setFindFriendWaypoint(double lat_deg, double lon_deg)
{
    if (!nav::FindFriendNavEstimator::coordinateValid(lat_deg, lon_deg))
        return false;
    ensureFindFriendPrefsLoaded();

    setFfWaypointCache(lat_deg, lon_deg);
    persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6), e6_to_deg(_ff_wp_lon_e6));
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
    return true;
}

void MeshAppUi::formatFindFriendWaypointInput(char *buf, size_t buf_len) const
{
    if (!buf || buf_len == 0)
        return;
    buf[0] = '\0';
    ensureFindFriendPrefsLoaded();
    if (!_ff_wp_valid)
        return;
    format_e6_pair(buf, buf_len, _ff_wp_lat_e6, _ff_wp_lon_e6);
}

int MeshAppUi::findFriendContactCount() const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().enabled())
        return static_cast<int>(the_mesh.teamService().memberCount());
#endif
    return 0;
}

bool MeshAppUi::findFriendMemberLabel(int index, char *buf, size_t buf_len) const
{
    if (!buf || buf_len == 0 || index < 0)
        return false;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().enabled())
    {
        heltec::meshcore::team::TeamMember member{};
        if (!the_mesh.teamService().memberAt(static_cast<size_t>(index), member))
            return false;
        const bool online = millis() - member.last_seen_ms <= 20UL * 60UL * 1000UL;
        const char *status = !online ? " [offline]" :
                             (!member.location_valid ? " [no loc]" : "");
        lv_snprintf(buf, buf_len, "%.*s%s", member.name_len, member.name, status);
        return true;
    }
#endif
    return false;
}

int MeshAppUi::fillFindFriendContacts(int offset, int selected_contact_index,
                                      FindFriendContactItem *items, int max_items, int *total_items,
                                      int *selected_rank) const
{
    if (total_items)
        *total_items = 0;
    if (selected_rank)
        *selected_rank = -1;
    // Team ordinals are contiguous; metadata queries require no page allocation.
    const int n = findFriendContactCount();
    if (total_items)
        *total_items = n;
    if (selected_rank && selected_contact_index >= 0 && selected_contact_index < n)
        *selected_rank = selected_contact_index;
    if (!items || max_items <= 0)
        return 0;
    if (offset < 0)
        offset = 0;
    int filled = 0;
    for (int index = offset; index < n && filled < max_items; ++index)
    {
        FindFriendContactItem &item = items[filled];
        item = FindFriendContactItem{};
        if (!findFriendMemberLabel(index, item.label, sizeof(item.label)))
            break;
        item.contact_index = static_cast<int16_t>(index);
        ++filled;
    }
    return filled;
}

bool MeshAppUi::findFriendMemberHasGps(int index) const
{
    if (index < 0)
        return false;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().enabled())
    {
        heltec::meshcore::team::TeamMember member{};
        return the_mesh.teamService().memberAt(static_cast<size_t>(index), member) &&
               member.location_valid;
    }
#endif
    return false;
}

int MeshAppUi::resolveFindFriendTargetContactIndex() const
{
    _ff_target_contact_idx = -1;
    if (!teamStatus().has_team)
    {
        _ff_target_pub_key_valid = false;
        memset(_ff_target_pub_key, 0, sizeof(_ff_target_pub_key));
        return -1;
    }
    if (!_ff_target_pub_key_valid)
        return -1;
    const int n = findFriendContactCount();
    for (int i = 0; i < n; ++i)
    {
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
        if (the_mesh.teamService().enabled())
        {
            heltec::meshcore::team::TeamMember member{};
            if (the_mesh.teamService().memberAt(static_cast<size_t>(i), member) &&
                memcmp(member.public_key, _ff_target_pub_key, PUB_KEY_SIZE) == 0)
            {
                _ff_target_contact_idx = i;
                break;
            }
            continue;
        }
#endif

    }
    return _ff_target_contact_idx;
}

bool MeshAppUi::setFindFriendTargetKeyFromIndex(int index) const
{
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (the_mesh.teamService().enabled())
    {
        heltec::meshcore::team::TeamMember member{};
        if (index < 0 || !the_mesh.teamService().memberAt(static_cast<size_t>(index), member))
            return false;
        memcpy(_ff_target_pub_key, member.public_key, PUB_KEY_SIZE);
        _ff_target_pub_key_valid = true;
        _ff_target_contact_idx = index;
        return true;
    }
#endif
    (void)index;
    return false;
}

int MeshAppUi::findFriendTargetContactIndex() const
{
    ensureFindFriendPrefsLoaded();
    return resolveFindFriendTargetContactIndex();
}

void MeshAppUi::setFindFriendTargetContactIndex(int index)
{
    ensureFindFriendPrefsLoaded();
    const int n = findFriendContactCount();
    if (index < 0 || index >= n)
    {
        _ff_target_contact_idx = -1;
        _ff_target_pub_key_valid = false;
        memset(_ff_target_pub_key, 0, sizeof(_ff_target_pub_key));
    }
    else
    {
        (void)setFindFriendTargetKeyFromIndex(index);
    }
    syncFindFriendTeamPin();

    resetFindFriendNavState();
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

bool MeshAppUi::tryAutoPickFindFriendTarget()
{
    ensureFindFriendPrefsLoaded();
    const int n = findFriendContactCount();
    if (n <= 0)
        return false;
    int idx = resolveFindFriendTargetContactIndex();
    if (idx < 0)
        idx = 0;
    for (int step = 0; step < n; ++step)
    {
        if (findFriendMemberHasGps(idx))
        {
            (void)setFindFriendTargetKeyFromIndex(idx);
            syncFindFriendTeamPin();
            resetFindFriendNavState();
            return true;
        }
        idx = (idx + 1 + n) % n;
    }
    return false;
}

FindFriendUi MeshAppUi::findFriendUi() const
{
    ensureFindFriendPrefsLoaded();
    syncCompassCache();
    FindFriendUi u{};
    const CompassUi &cm = _compass_ui;
    u.compass_hw = cm.has_hardware;
    u.compass_quality = cm.quality;
    u.heading_valid = cm.heading_valid;
    u.heading_deg = nav::FindFriendNavEstimator::wrapHeading360(
        cm.heading_deg + static_cast<float>(COMPASS_HEADING_OFFSET_DEG));
    u.declination_configured = kCompassDeclinationConfigured;
    if (!_ff_enabled)
    {
        resetFindFriendNavState();
        return u;
    }
    const GpsStatus gps = gpsStatus();
    u.mode = _ff_mode;
    u.gps_age_ms = gps.fix_valid_ms;
    u.gps_satellites = gps.satellites;
    u.waypoint_valid = (_ff_wp_valid != 0) &&
                       nav::FindFriendNavEstimator::coordinateValid(microdeg_to_deg(_ff_wp_lat_e6),
                                                                    microdeg_to_deg(_ff_wp_lon_e6));
    u.waypoint_lat = microdeg_to_deg(_ff_wp_lat_e6);
    u.waypoint_lon = microdeg_to_deg(_ff_wp_lon_e6);

    double tgt_lat = 0;
    double tgt_lon = 0;
    bool have_tgt = false;

    if (_ff_mode == 0)
    {
        const int target_index = resolveFindFriendTargetContactIndex();
        u.target_selected = target_index >= 0;
        if (target_index >= 0)
        {
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
            if (the_mesh.teamService().enabled())
            {
                heltec::meshcore::team::TeamMember member{};
                if (the_mesh.teamService().memberAt(static_cast<size_t>(target_index), member))
                {
                    lv_snprintf(u.target_label, sizeof(u.target_label), "%.*s",
                                member.name_len, member.name);
                    if (member.location_valid)
                    {
                        tgt_lat = microdeg_to_deg(member.latitude_e6);
                        tgt_lon = microdeg_to_deg(member.longitude_e6);
                        have_tgt = true;
                    }
                    u.target_age_known = member.last_seen_ms != 0;
                    u.target_age_ms = millis() - member.last_seen_ms +
                                      static_cast<uint32_t>(member.fix_age_sec) * 1000UL;
                }
            }
#endif
        }
        else
        {
            lv_snprintf(u.target_label, sizeof(u.target_label), "--");
        }
    }
    else
    {
        u.target_selected = u.waypoint_valid;
        have_tgt = u.waypoint_valid;
        if (have_tgt)
        {
            tgt_lat = microdeg_to_deg(_ff_wp_lat_e6);
            tgt_lon = microdeg_to_deg(_ff_wp_lon_e6);
        }
        lv_snprintf(u.target_label, sizeof(u.target_label), "WP");
    }

    u.target_valid = have_tgt;

    nav::FindFriendNavInput input{};
    input.now_ms = millis();
    input.gps_valid = gps.fix_valid;
    input.gps_lat_deg = gps.lat_deg;
    input.gps_lon_deg = gps.lon_deg;
    input.gps_age_ms = gps.fix_valid_ms;
    input.gps_satellites = gps.satellites;
    input.target_valid = have_tgt;
    input.target_lat_deg = tgt_lat;
    input.target_lon_deg = tgt_lon;
    input.target_is_contact = _ff_mode == 0;
    input.target_age_known = u.target_age_known;
    input.target_age_ms = u.target_age_ms;
    input.heading_valid = cm.heading_valid;
    // ICMCompassProvider has already converted magnetic north using
    // COMPASS_DECLINATION_DEG before this heading reaches the facade.
    input.heading_deg = nav::FindFriendNavEstimator::wrapHeading360(
        cm.heading_deg + static_cast<float>(COMPASS_HEADING_OFFSET_DEG));
    input.compass_quality = static_cast<uint8_t>(cm.quality < 0 ? 0 : cm.quality);
    input.declination_configured = kCompassDeclinationConfigured;

    const nav::FindFriendNavOutput estimated = _ff_nav_estimator.update(input);
    u.gps_fix = estimated.gps_valid;
    u.here_lat = estimated.filtered_lat_deg;
    u.here_lon = estimated.filtered_lon_deg;
    u.estimated_accuracy_m = estimated.estimated_accuracy_m;
    u.gps_low_accuracy = estimated.gps_low_accuracy;
    u.target_usable = estimated.target_usable;
    u.target_freshness = to_ui_freshness(estimated.target_freshness);
    u.heading_valid = estimated.heading_valid;
    u.heading_deg = estimated.filtered_heading_deg;
    u.bearing_valid = estimated.bearing_valid;
    u.bearing_to_waypoint_deg = estimated.bearing_deg;
    u.relative_valid = estimated.relative_valid;
    u.turn_deg = estimated.turn_deg;
    u.distance_m = estimated.distance_m;
    u.near_target = estimated.near_target;
    u.nearby_enter_m = estimated.nearby_enter_m;
    u.confidence = to_ui_confidence(estimated.confidence);

    return u;
}

} // namespace heltec::meshcore::biz
