#pragma once

#include "GeoPoint.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Ui::Map
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

} // namespace Heltec::MeshCore::Ui::Map
