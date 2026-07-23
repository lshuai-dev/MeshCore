#pragma once

#include "geo_point.hpp"

#include <stdint.h>

namespace heltec::meshcore::ui::map {

struct MapUiPrefs {
  float home_lat = 51.5074f;
  float home_lon = -0.1278f;
  uint8_t zoom = kZoomDefault;
  char tile_style[16] = "osm";
};

bool map_prefs_load(MapUiPrefs& out);
void map_prefs_save(const MapUiPrefs& prefs);

}  // namespace heltec::meshcore::ui::map
