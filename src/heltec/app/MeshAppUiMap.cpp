#include "MeshAppUi.hpp"

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "HeltecMesh.h"
#include "MapGeo.hpp"

#include <Arduino.h>
#include <string.h>

namespace Heltec::MeshCore::Biz
{
namespace
{

void copyMarkerLabel(char output[9], const char *name, size_t name_length)
{
    output[0] = 'T';
    output[1] = '\0';
    if (!name || name_length == 0)
        return;

    size_t input_offset = 0;
    size_t output_offset = 0;
    uint8_t character_count = 0;
    while (input_offset < name_length && character_count < 2U)
    {
        const uint8_t lead = static_cast<uint8_t>(name[input_offset]);
        size_t character_bytes = 1;
        if ((lead & 0xF8U) == 0xF0U)
            character_bytes = 4;
        else if ((lead & 0xF0U) == 0xE0U)
            character_bytes = 3;
        else if ((lead & 0xE0U) == 0xC0U)
            character_bytes = 2;
        if (input_offset + character_bytes > name_length ||
            output_offset + character_bytes >= 9U)
            break;
        memcpy(output + output_offset, name + input_offset, character_bytes);
        input_offset += character_bytes;
        output_offset += character_bytes;
        ++character_count;
    }
    output[output_offset] = '\0';
}

bool teamLocationUsable(const Heltec::MeshCore::Team::TeamMember &member, uint32_t now_ms)
{
    if (!member.location_valid ||
        !Geo::has_valid_gps_micro(member.latitude_e6, member.longitude_e6))
    {
        return false;
    }

    constexpr uint32_t kTimeoutMs = HELTEC_TEAM_MEMBER_TIMEOUT_SEC * 1000UL;
    const uint32_t received_age_ms = now_ms - member.last_seen_ms;
    if (received_age_ms > kTimeoutMs)
    {
        return false;
    }
    const uint32_t remaining_ms = kTimeoutMs - received_age_ms;
    return static_cast<uint32_t>(member.fix_age_sec) <= remaining_ms / 1000UL;
}

void buildTeamMapPlotUi(const MeshAppUi &app, MapPlotUi &out)
{
    out.online_count = 0;
    out.marker_count = 0;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    const uint32_t now_ms = millis();
    const auto &team = the_mesh.teamService();
    for (size_t slot = 0; slot < HELTEC_TEAM_MAX_MEMBERS &&
                          out.marker_count < MapPlotUi::kMaxMarkers; ++slot)
    {
        const Heltec::MeshCore::Team::TeamMember *const member = team.memberSlot(slot);
        if (!member)
        {
            continue;
        }
        if (now_ms - member->last_seen_ms <= HELTEC_TEAM_MEMBER_TIMEOUT_SEC * 1000UL)
        {
            ++out.online_count;
        }
        if (!teamLocationUsable(*member, now_ms))
        {
            continue;
        }

        MapPlotMarker &marker = out.markers[out.marker_count++];
        marker.lat_micro = member->latitude_e6;
        marker.lon_micro = member->longitude_e6;
        copyMarkerLabel(marker.label, member->name, member->name_len);
        marker.is_self = false;
        memcpy(marker.pub_key_prefix, member->public_key, sizeof(marker.pub_key_prefix));
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
        marker.alarm_active = app.teamGeofenceMemberAlarm(marker.pub_key_prefix);
#endif
    }
#endif
}

} // namespace

const MapPlotUi &MeshAppUi::mapPlotUi() const
{
    buildTeamMapPlotUi(*this, _map_plot_cache);
    return _map_plot_cache;
}

} // namespace Heltec::MeshCore::Biz

#endif // ENV_INCLUDE_MAP
