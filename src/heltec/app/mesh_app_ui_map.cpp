#include "mesh_app_ui.hpp"

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "HeltecMesh.h"
#include "map_geo.hpp"

#include <Arduino.h>
#include <string.h>

namespace heltec::meshcore::biz
{
namespace
{

bool teamLocationUsable(const heltec::meshcore::team::TeamMember &member, uint32_t now_ms)
{
    if (!member.location_valid ||
        !geo::has_valid_gps_micro(member.latitude_e6, member.longitude_e6))
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

void buildTeamMapPlotUi(MapPlotUi &out)
{
    out.online_count = 0;
    out.marker_count = 0;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    const uint32_t now_ms = millis();
    const auto &team = the_mesh.teamService();
    for (size_t slot = 0; slot < HELTEC_TEAM_MAX_MEMBERS &&
                          out.marker_count < MapPlotUi::kMaxMarkers; ++slot)
    {
        const heltec::meshcore::team::TeamMember *const member = team.memberSlot(slot);
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
        marker.glyph = member->name[0] ? member->name[0] : 'T';
        marker.is_self = false;
        memcpy(marker.pub_key_prefix, member->public_key, sizeof(marker.pub_key_prefix));
    }
#endif
}

} // namespace

const MapPlotUi &MeshAppUi::mapPlotUi() const
{
    buildTeamMapPlotUi(_map_plot_cache);
    return _map_plot_cache;
}

} // namespace heltec::meshcore::biz

#endif // ENV_INCLUDE_MAP
