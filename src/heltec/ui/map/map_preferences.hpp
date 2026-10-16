#pragma once

#include "geo_point.hpp"

#include <stdint.h>

namespace heltec::meshcore::ui::map
{

struct MapPreferences
{
    float home_lat = 51.5074f;
    float home_lon = -0.1278f;
    uint8_t zoom = kZoomDefault;
    char tile_style[16] = "osm";
};

bool map_preferences_load(MapPreferences &out);
void map_preferences_save(const MapPreferences &prefs);

} // namespace heltec::meshcore::ui::map
