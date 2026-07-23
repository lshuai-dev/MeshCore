#pragma once

#include "geo_point.hpp"
#include "map_prefs.hpp"

#include <stddef.h>
#include <stdint.h>

namespace heltec::meshcore::ui::map {

/** Prepare CS pins (fast, safe to call repeatedly). */
void map_sd_prepare_pins();
/** Reset deferred probe state (call on tracker screen enter). */
void map_sd_on_screen_enter();
/**
 * Try one descending SPI clock (80MHz..400kHz).
 * Returns false while another probe tick is required, true when probing has
 * completed.  Use map_sd_ready() to distinguish success from exhaustion.
 */
bool map_sd_probe_once();
/** Compatibility alias for map_sd_probe_once(). */
bool map_sd_try_mount_once();
/** Compatibility alias; high-to-low probing no longer has a separate boost phase. */
bool map_sd_try_boost_speed_once();
/** Mount SD and register LVGL FS driver (letter S:). Returns true if tiles may load. */
bool map_sd_init();
bool map_sd_ready();
/** Active SD SPI clock after mount/boost (Hz); 0 if not mounted. */
uint32_t map_sd_active_hz();
/** "exFAT", "FAT32", etc.; nullptr if SD not mounted. */
const char* map_sd_fs_label();
void map_sd_register_lvgl_fs();
bool map_sd_tile_path(char* out, size_t out_len, const char* style, uint8_t zoom, uint32_t x_tile,
                      uint32_t y_tile);
/** Resolve maps/{style}/... or tiles/{z}/{x}/{y}.png and verify it exists. */
bool map_sd_resolve_tile_path(char* out, size_t out_len, const char* style, uint8_t zoom,
                              uint32_t x_tile, uint32_t y_tile);
/** Relative path without S: prefix (e.g. maps/osm/6/32/32.png). */
bool map_sd_exists(const char* rel_path);
/** Pick maps/{style} on SD; styleless tiles/ cards keep the requested style. */
void map_sd_resolve_style(char* style, size_t style_len);
/** If the preferred zoom has no valid PNG, pick nearest maps/ or tiles/ zoom. */
uint8_t map_sd_best_zoom(const char* style, uint8_t preferred);
/** Log maps/ styles and maps/tiles zoom folders for current style (MESH_DEBUG). */
void map_sd_log_catalog(const char* style);
/** Resolve style + zoom from SD contents; updates prefs when adjusted. */
void map_sd_apply_tile_prefs(MapUiPrefs& prefs);
/** First valid tile under maps/{style}/{z} or tiles/{z} on SD. */
bool map_sd_find_first_tile(const char* style, uint8_t zoom, uint32_t& x_tile, uint32_t& y_tile);
/** If center lat/lon has no tile on SD, move to the first tile found. Returns true if adjusted. */
bool map_sd_snap_center_to_tiles(const char* style, uint8_t zoom, float& lat, float& lon);

}  // namespace heltec::meshcore::ui::map
