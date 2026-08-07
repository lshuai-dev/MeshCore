#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "map_panel.hpp"

#include "map_sd.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/theme/ui_widget_theme.hpp"

#include "ui/core/biz_facade.hpp"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#if defined(ESP_PLATFORM)
#include <esp_task_wdt.h>
#endif

namespace heltec::meshcore::ui {
}  // namespace heltec::meshcore::ui

namespace heltec::meshcore::ui::map {
namespace {

#ifndef MAP_UI_PNG_LOADS_PER_TICK
#define MAP_UI_PNG_LOADS_PER_TICK 1
#endif

#ifndef MAP_UI_SD_STAT_PER_TICK
#define MAP_UI_SD_STAT_PER_TICK 1
#endif

#ifndef MAP_UI_LAYOUT_BUDGET_US
#define MAP_UI_LAYOUT_BUDGET_US 5000
#endif

constexpr int kPngLoadsPerTick = MAP_UI_PNG_LOADS_PER_TICK;
constexpr int kSdStatPerTick = MAP_UI_SD_STAT_PER_TICK;

constexpr int kRangeRingCount = 4;
constexpr uint32_t kRangeDistancesM[kRangeRingCount] = {500, 1000, 2000, 5000};
constexpr const char* kRangeLabels[kRangeRingCount] = {"500m", "1km", "2km", "5km"};

int64_t floor_div(int64_t value, int64_t divisor) {
  if (divisor <= 0) return 0;
  int64_t quotient = value / divisor;
  const int64_t remainder = value % divisor;
  if (remainder < 0) --quotient;
  return quotient;
}

uint32_t wrap_tile(uint32_t tile, uint32_t world_tiles) {
  return world_tiles == 0 ? 0U : tile % world_tiles;
}

int32_t wrapped_tile_delta(uint32_t tile, uint32_t start, uint32_t world_tiles) {
  if (world_tiles == 0) return 0;
  // Tile grids are laid out in the forward X direction from _x_start.  Use
  // that same modulo distance for overlays, including viewports wider than
  // half of a low-zoom world; choosing the shortest signed path would fold a
  // valid right-hand tile back onto the left side in that case.
  const uint64_t forward = ((uint64_t)tile + world_tiles - (uint64_t)start) % world_tiles;
  return (int32_t)forward;
}

lv_obj_t* make_pin(lv_obj_t* parent, const char* text, lv_color_t color) {
  lv_obj_t* o = ht_obj_create(parent, meta_id::MapMarker);
  if (!o) return nullptr;
  lv_obj_set_size(o, 14, 14);
  // The marker layer owns map gestures.  Marker objects are visual only and
  // must not become LVGL touch targets over the map.
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
  ui_map_marker_apply_color(o, color);
  if (lv_obj_t* label = ht_label_create(o, meta_id::MapMarkerLabel, text)) {
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  }
  return o;
}

}  // namespace

void MapPanel::load_prefs(const float* gps_home_deg) {
#if defined(ESP_PLATFORM)
  esp_task_wdt_reset();
#endif
  _prefs_loaded = map_prefs_load(_prefs);
  _prefs_dirty = false;
  if (!_prefs_loaded) {
    _prefs.zoom = kZoomDefault;
    strncpy(_prefs.tile_style, "osm", sizeof(_prefs.tile_style));
    _prefs.tile_style[sizeof(_prefs.tile_style) - 1] = '\0';
  }
  _home = GeoPoint(_prefs.home_lat, _prefs.home_lon, _prefs.zoom);
  _scrolled = gps_home_deg ? GeoPoint(gps_home_deg[0], gps_home_deg[1], _prefs.zoom) : _home;
  _sd_tiles = map_sd_ready();
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
}

void MapPanel::applySdPrefs() {
  if (!map_sd_ready()) return;
  const uint8_t before_zoom = _prefs.zoom;
  char before_style[sizeof(_prefs.tile_style)];
  strncpy(before_style, _prefs.tile_style, sizeof(before_style) - 1);
  before_style[sizeof(before_style) - 1] = '\0';
  map_sd_apply_tile_prefs(_prefs);
  const bool changed = before_zoom != _prefs.zoom ||
                       strncmp(before_style, _prefs.tile_style, sizeof(before_style)) != 0;
  _sd_tiles = true;
  if (changed) {
    _home.set_zoom(_prefs.zoom);
    _scrolled.set_zoom(_prefs.zoom);
    if (_prefs_loaded) map_prefs_save(_prefs);
  }
  request_redraw();
}

void MapPanel::refreshSdTiles() {
  const bool was = _sd_tiles;
  _sd_tiles = map_sd_ready();
  if (was != _sd_tiles) {
    request_redraw();
  }
}

void MapPanel::syncViewportLayout() {
  if (!_viewport) return;
  lv_obj_update_layout(_viewport);
  request_layout();
}

void MapPanel::centerOnLocation(float lat, float lon, bool persist) {
  if (!isfinite(lat) || !isfinite(lon) || lat < -90.0f || lat > 90.0f ||
      lon < -180.0f || lon > 180.0f || (lat == 0.0f && lon == 0.0f)) {
    return;
  }
  _scrolled = GeoPoint(lat, lon, _prefs.zoom);
  _home = _scrolled;
  if (persist) {
    _prefs.home_lat = _scrolled.latitude;
    _prefs.home_lon = _scrolled.longitude;
    _prefs_dirty = true;
  }
  request_redraw();
  if (persist) save_prefs();
}

void MapPanel::save_prefs() {
  if (!_prefs_dirty) return;
  _prefs.home_lat = _scrolled.latitude;
  _prefs.home_lon = _scrolled.longitude;
  _prefs.zoom = _scrolled.zoom_level;
  map_prefs_save(_prefs);
  _home = _scrolled;
  _prefs_loaded = true;
  _prefs_dirty = false;
}

bool MapPanel::createTileSlot(int idx) {
  if (idx < 0 || idx >= kMaxTiles || !_tile_layer) return false;
  TileSlot& t = _tiles[idx];
  if (t.root) return true;

  t.root = ht_obj_create(_tile_layer, meta_id::MapTile);
  if (!t.root) return false;
  lv_obj_set_size(t.root, map::kTileSizePx, map::kTileSizePx);
  lv_obj_clear_flag(t.root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(t.root, LV_OBJ_FLAG_HIDDEN);

  t.img = ht_img_create(t.root, meta_id::MapTileImage);
  if (!t.img) {
    lv_obj_del(t.root);
    t.root = nullptr;
    return false;
  }
  lv_obj_set_size(t.img, map::kTileSizePx, map::kTileSizePx);
  lv_obj_set_pos(t.img, 0, 0);
  lv_obj_clear_flag(t.img, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(t.img, LV_OBJ_FLAG_HIDDEN);
  t.placeholder = ht_label_create(t.root, meta_id::MapTilePlaceholder);
  if (t.placeholder) {
    lv_obj_set_width(t.placeholder, map::kTileSizePx - 8);
    lv_obj_align(t.placeholder, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(t.placeholder, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(t.placeholder, t.placeholder_text);
  }
  return true;
}

bool MapPanel::buildPendingTiles(int max_tiles) {
  if (!_tile_layer || max_tiles <= 0) return tilesReady();
  _tile_build_failed = false;
  int built = 0;
  while (_tiles_built < kMaxTiles && built < max_tiles) {
#if defined(ESP_PLATFORM)
    esp_task_wdt_reset();
#endif
    if (!createTileSlot(_tiles_built)) {
      _tile_build_failed = true;
      _tile_load_pending = false;
      break;
    }
    ++_tiles_built;
    ++built;
  }
  if (tilesReady()) {
    markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
  }
  return tilesReady();
}

bool MapPanel::ensureMarkerSlot(int idx) {
  if (idx < 0 || idx >= kMaxMarkers || !_marker_layer) return false;
  MarkerSlot& m = _markers[idx];
  if (m.root) return true;

  m.root = ht_obj_create(_marker_layer, meta_id::MapMarker);
  if (!m.root) return false;
  lv_obj_set_size(m.root, 14, 14);
  lv_obj_clear_flag(m.root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(m.root, LV_OBJ_FLAG_HIDDEN);
  m.label = ht_label_create(m.root, meta_id::MapMarkerLabel, "?");
  if (!m.label) {
    lv_obj_del(m.root);
    m.root = nullptr;
    return false;
  }
  lv_label_set_text_static(m.label, m.text);
  lv_obj_align(m.label, LV_ALIGN_CENTER, 0, 0);
  return true;
}

bool MapPanel::prewarmPools(int max_tiles, int max_markers) {
  if (!_tile_layer || !_marker_layer || poolBuildFailed()) return false;

  if (!tilesReady()) {
    (void)buildPendingTiles(max_tiles);
    return poolsReady();
  }

  int built = 0;
  while (_markers_built < kMaxMarkers && built < max_markers) {
#if defined(ESP_PLATFORM)
    esp_task_wdt_reset();
#endif
    if (!ensureMarkerSlot(_markers_built)) {
      _marker_build_failed = true;
      break;
    }
    ++_markers_built;
    ++built;
  }
  return poolsReady();
}

void MapPanel::attach(_lv_obj_t* viewport) {
  if (_viewport) return;
  _viewport = viewport;
  if (!_viewport) return;

  _tiles_built = 0;
  _markers_built = 0;
  _tile_build_failed = false;
  _marker_build_failed = false;
  _tile_layer = ht_obj_create(_viewport, meta_id::MapTileLayer);
  _range_layer = ht_obj_create(_viewport, meta_id::MapRangeLayer);
  _marker_layer = ht_obj_create(_viewport, meta_id::MapMarkerLayer);
  if (!_tile_layer || !_range_layer || !_marker_layer) {
    if (_tile_layer) {
      lv_obj_del(_tile_layer);
      _tile_layer = nullptr;
    }
    if (_range_layer) {
      lv_obj_del(_range_layer);
      _range_layer = nullptr;
    }
    if (_marker_layer) {
      lv_obj_del(_marker_layer);
      _marker_layer = nullptr;
    }
    _viewport = nullptr;
    return;
  }
  _lv_obj_t* const passive_layers[] = {_tile_layer, _range_layer};
  for (_lv_obj_t* layer : passive_layers) {
    lv_obj_set_size(layer, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(layer, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_set_size(_marker_layer, lv_pct(100), lv_pct(100));
  lv_obj_clear_flag(_marker_layer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_marker_layer, LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_add_flag(_marker_layer, LV_OBJ_FLAG_CLICKABLE);

  for (int i = 0; i < kRangeRingCount; ++i) {
    _range_rings[i] = ht_obj_create(_range_layer, meta_id::MapRangeRing);
    if (_range_rings[i]) {
      lv_obj_clear_flag(_range_rings[i], LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(_range_rings[i], LV_OBJ_FLAG_HIDDEN);
      ui_map_range_ring_apply_opa(_range_rings[i],
                                      (lv_opa_t)(LV_OPA_COVER - (i * 18)));
    }
    _range_labels[i] = ht_label_create(_range_layer, meta_id::MapRangeLabel);
    if (_range_labels[i]) {
      lv_label_set_text_static(_range_labels[i], kRangeLabels[i]);
      lv_obj_clear_flag(_range_labels[i], LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(_range_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  _gps_pin = make_pin(_marker_layer, "G", ui_color_success());
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);

}

void MapPanel::center_view() {
  if (!_viewport) return;
  lv_obj_update_layout(_viewport);
  _width = lv_obj_get_width(_viewport);
  _height = lv_obj_get_height(_viewport);
  if (_width <= 0 || _height <= 0) return;

  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  const int64_t center_x = (int64_t)_scrolled.x_tile * kTileSizePx + _scrolled.x_pos;
  const int64_t center_y = (int64_t)_scrolled.y_tile * kTileSizePx + _scrolled.y_pos;
  const int64_t left_px = center_x - _width / 2;
  const int64_t top_px = center_y - _height / 2;
  const int64_t x_start_signed = floor_div(left_px, kTileSizePx);
  const int64_t y_start_signed = floor_div(top_px, kTileSizePx);
  const int64_t x_remainder = left_px - x_start_signed * kTileSizePx;
  const int64_t y_remainder = top_px - y_start_signed * kTileSizePx;
  _x_offset = (int16_t)-x_remainder;
  _x_start = wrap_tile((uint32_t)((x_start_signed % world_tiles + world_tiles) % world_tiles),
                       world_tiles);
  const int64_t clamped_y_start = y_start_signed < 0
                                      ? 0
                                      : (y_start_signed >= world_tiles ? world_tiles - 1
                                                                       : y_start_signed);
  _y_start = (uint32_t)clamped_y_start;
  _y_offset = y_start_signed < 0 ? (int16_t)-top_px : (int16_t)-y_remainder;
  _tiles_x = (uint8_t)((_width - _x_offset + kTileSizePx - 1) / kTileSizePx);
  _tiles_y = (uint8_t)((_height - _y_offset + kTileSizePx - 1) / kTileSizePx);
  if (_tiles_x == 0) _tiles_x = 1;
  if (_tiles_y == 0) _tiles_y = 1;
  // Never describe more cells than the fixed tile-slot pool can hold.  The
  // V4 R8 viewport is much smaller than this limit, but keeping the invariant
  // here prevents a future wider layout from silently overflowing the grid.
  if (_tiles_x > kMaxTiles) _tiles_x = kMaxTiles;
  const uint8_t max_rows = (uint8_t)(kMaxTiles / _tiles_x);
  if (_tiles_y > max_rows) _tiles_y = max_rows;
}

void MapPanel::request_layout() {
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
  _tile_load_pending = true;
  _tile_layout_prepared = false;
  _tile_load_cursor = 0;
}

void MapPanel::request_redraw() {
  request_layout();
  for (TileSlot& t : _tiles) {
    t.active = false;
    if (t.loaded_path[0] != '\0') {
      lv_img_cache_invalidate_src(t.loaded_path);
      if (t.img) lv_img_set_src(t.img, nullptr);
    }
    t.loaded_path[0] = '\0';
    t.missing_cached = false;
    if (t.root) lv_obj_add_flag(t.root, LV_OBJ_FLAG_HIDDEN);
  }
}

void MapPanel::prepare_tile_layout() {
  _tile_need_count = 0;
  _tile_load_cursor = 0;
  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  for (uint8_t y = 0; y < _tiles_y && _tile_need_count < kMaxTiles; ++y) {
    for (uint8_t x = 0; x < _tiles_x && _tile_need_count < kMaxTiles; ++x) {
      const uint32_t yt = _y_start + y;
      if (yt >= world_tiles) continue;
      TileNeed& need = _tile_needs[_tile_need_count++];
      need.x_tile = wrap_tile(_x_start + x, world_tiles);
      need.y_tile = yt;
      need.x = (int16_t)(x * kTileSizePx + _x_offset);
      need.y = (int16_t)(y * kTileSizePx + _y_offset);
      need.slot = -1;
    }
  }

  bool slot_used[kMaxTiles]{};
  for (uint8_t ni = 0; ni < _tile_need_count; ++ni) {
    TileNeed& need = _tile_needs[ni];
    for (int si = 0; si < kMaxTiles; ++si) {
      if (slot_used[si] || !_tiles[si].root) continue;
      const TileSlot& t = _tiles[si];
      if (t.x_tile != need.x_tile || t.y_tile != need.y_tile) continue;
      need.slot = (int8_t)si;
      slot_used[si] = true;
      break;
    }
  }

  for (uint8_t ni = 0; ni < _tile_need_count; ++ni) {
    TileNeed& need = _tile_needs[ni];
    if (need.slot >= 0) continue;
    for (int si = 0; si < kMaxTiles; ++si) {
      if (slot_used[si] || !_tiles[si].root) continue;
      need.slot = (int8_t)si;
      slot_used[si] = true;
      break;
    }
  }

  for (int si = 0; si < kMaxTiles; ++si) {
    TileSlot& t = _tiles[si];
    if (slot_used[si]) continue;
    t.active = false;
    if (t.loaded_path[0] != '\0') {
      lv_img_cache_invalidate_src(t.loaded_path);
      if (t.img) lv_img_set_src(t.img, nullptr);
      t.loaded_path[0] = '\0';
    }
    t.missing_cached = false;
    if (t.root) lv_obj_add_flag(t.root, LV_OBJ_FLAG_HIDDEN);
  }

  for (uint8_t ni = 0; ni < _tile_need_count; ++ni) {
    const TileNeed& need = _tile_needs[ni];
    if (need.slot < 0) continue;
    TileSlot& t = _tiles[need.slot];
    const bool identity_changed = t.x_tile != need.x_tile || t.y_tile != need.y_tile;
    if (identity_changed) {
      if (t.loaded_path[0] != '\0') {
        lv_img_cache_invalidate_src(t.loaded_path);
        if (t.img) lv_img_set_src(t.img, nullptr);
        t.loaded_path[0] = '\0';
      }
      t.missing_cached = false;
      if (t.img) lv_obj_add_flag(t.img, LV_OBJ_FLAG_HIDDEN);
    }
    t.x_tile = need.x_tile;
    t.y_tile = need.y_tile;
    t.active = true;
    lv_obj_set_pos(t.root, need.x, need.y);
    lv_obj_clear_flag(t.root, LV_OBJ_FLAG_HIDDEN);
  }

  _tile_layout_prepared = true;
  _tile_load_pending = _tile_need_count > 0;
}

void MapPanel::layout_tiles() {
  if (!_tile_layer || !tilesReady()) return;
  if (!_tile_layout_prepared) prepare_tile_layout();
  if (_pan_dragging) {
    _tile_load_pending = _tile_load_cursor < _tile_need_count;
    return;
  }

  const uint32_t started_us = micros();
  const uint8_t started_cursor = _tile_load_cursor;
  int png_loads = 0;
  int stat_budget = kSdStatPerTick;

  while (_tile_load_cursor < _tile_need_count) {
    if (_tile_load_cursor != started_cursor &&
        micros() - started_us >= (uint32_t)MAP_UI_LAYOUT_BUDGET_US) {
      break;
    }
    const TileNeed& need = _tile_needs[_tile_load_cursor];
    if (need.slot < 0) {
      ++_tile_load_cursor;
      continue;
    }
    TileSlot& t = _tiles[need.slot];
    if (!t.root) {
      ++_tile_load_cursor;
      continue;
    }
    char path[96] = {};
    const char* placeholder_hint = nullptr;
    if (!_sd_tiles || !t.img) {
      placeholder_hint = "(no SD)";
    } else if (t.loaded_path[0] != '\0') {
      lv_obj_clear_flag(t.img, LV_OBJ_FLAG_HIDDEN);
      if (t.placeholder) lv_obj_add_flag(t.placeholder, LV_OBJ_FLAG_HIDDEN);
      ++_tile_load_cursor;
      continue;
    } else if (t.missing_cached) {
      placeholder_hint = "(missing)";
    } else {
      if (stat_budget <= 0 || png_loads >= kPngLoadsPerTick) break;
#if defined(ESP_PLATFORM)
      esp_task_wdt_reset();
#endif
      const bool has_tile = map_sd_resolve_tile_path(path, sizeof(path), _prefs.tile_style,
                                                     _scrolled.zoom_level, need.x_tile,
                                                     need.y_tile);
      --stat_budget;
      if (!has_tile) {
        t.missing_cached = true;
        placeholder_hint = "(missing)";
      } else {
        lv_img_set_src(t.img, path);
        strncpy(t.loaded_path, path, sizeof(t.loaded_path) - 1);
        t.loaded_path[sizeof(t.loaded_path) - 1] = '\0';
#if defined(ESP_PLATFORM)
        esp_task_wdt_reset();
#endif
        yield();
        lv_obj_clear_flag(t.img, LV_OBJ_FLAG_HIDDEN);
        if (t.placeholder) lv_obj_add_flag(t.placeholder, LV_OBJ_FLAG_HIDDEN);
        ++png_loads;
        ++_tile_load_cursor;
        continue;
      }
    }

    if (t.img) {
      lv_obj_add_flag(t.img, LV_OBJ_FLAG_HIDDEN);
    }
    if (t.placeholder) {
      lv_obj_clear_flag(t.placeholder, LV_OBJ_FLAG_HIDDEN);
      snprintf(t.placeholder_text, sizeof(t.placeholder_text), "z%u\n%lu/%lu\n%s",
               (unsigned)_scrolled.zoom_level,
               (unsigned long)need.x_tile, (unsigned long)need.y_tile,
               placeholder_hint ? placeholder_hint : "(loading)");
      lv_label_set_text_static(t.placeholder, t.placeholder_text);
    }
    ++_tile_load_cursor;
  }
  _tile_load_pending = _tile_load_cursor < _tile_need_count;
}

void MapPanel::layout_markers() {
  _visible_marker_count = 0;
  for (MarkerSlot& m : _markers) {
    m.active = false;
    if (m.root) lv_obj_add_flag(m.root, LV_OBJ_FLAG_HIDDEN);
  }

  int slot = 0;
  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  for (int i = 0; i < _marker_count && slot < kMaxMarkers; ++i) {
    const auto& src = _marker_data[i];
    if (src.is_self) {
      continue;
    }
    GeoPoint gp((float)src.lat_micro / 1000000.0f, (float)src.lon_micro / 1000000.0f,
                _scrolled.zoom_level);
    const int32_t tile_dx = wrapped_tile_delta(gp.x_tile, _x_start, world_tiles);
    const int32_t tile_dy = (int32_t)gp.y_tile - (int32_t)_y_start;
    if (tile_dx < 0 || tile_dx >= _tiles_x || tile_dy < 0 || tile_dy >= _tiles_y) {
      continue;
    }
    const int16_t tx = (int16_t)(tile_dx * kTileSizePx + _x_offset + gp.x_pos);
    const int16_t ty = (int16_t)(tile_dy * kTileSizePx + _y_offset + gp.y_pos);
    if (tx < -8 || ty < -8 || tx > _width + 8 || ty > _height + 8) {
      continue;
    }

    MarkerSlot& m = _markers[slot];
    ensureMarkerSlot(slot);
    if (!m.root) continue;
    m.active = true;
    _visible_marker_count++;
    lv_obj_clear_flag(m.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(m.root, (lv_coord_t)(tx - 7), (lv_coord_t)(ty - 7));
    if (m.label && m.glyph != src.glyph) {
      m.glyph = src.glyph;
      m.text[0] = m.glyph;
      lv_label_set_text_static(m.label, m.text);
    }
    ++slot;
  }
}

void MapPanel::reposition_visible_tiles() {
  if (!_tile_layer) return;
  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  for (TileSlot& t : _tiles) {
    if (!t.active || !t.root) continue;
    const int32_t tile_dx = wrapped_tile_delta(t.x_tile, _x_start, world_tiles);
    const int32_t tile_dy = (int32_t)t.y_tile - (int32_t)_y_start;
    if (tile_dx < 0 || tile_dx >= _tiles_x || tile_dy < 0 || tile_dy >= _tiles_y) {
      lv_obj_add_flag(t.root, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    const int16_t px = (int16_t)(tile_dx * kTileSizePx + _x_offset);
    const int16_t py = (int16_t)(tile_dy * kTileSizePx + _y_offset);
    lv_obj_clear_flag(t.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(t.root, px, py);
  }
}

void MapPanel::draw_location_pins() {
  auto place = [&](lv_obj_t* pin, const GeoPoint& gp) {
    if (!pin) return;
    if (gp.zoom_level != _scrolled.zoom_level) return;
    const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
    const int32_t tile_dx = wrapped_tile_delta(gp.x_tile, _x_start, world_tiles);
    const int32_t tile_dy = (int32_t)gp.y_tile - (int32_t)_y_start;
    if (tile_dx < 0 || tile_dx >= _tiles_x || tile_dy < 0 || tile_dy >= _tiles_y) {
      lv_obj_add_flag(pin, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    const int32_t tx = tile_dx * kTileSizePx + _x_offset + gp.x_pos;
    const int32_t ty = tile_dy * kTileSizePx + _y_offset + gp.y_pos;
    if (tx < -8 || ty < -8 || tx > _width + 8 || ty > _height + 8) {
      lv_obj_add_flag(pin, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    lv_obj_clear_flag(pin, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(pin, (lv_coord_t)(tx - 7), (lv_coord_t)(ty - 7));
    lv_obj_move_foreground(pin);
  };

  if (_gps_valid) {
    GeoPoint gp = _gps;
    gp.set_zoom(_scrolled.zoom_level);
    place(_gps_pin, gp);
  } else if (_gps_pin) {
    lv_obj_add_flag(_gps_pin, LV_OBJ_FLAG_HIDDEN);
  }
}

void MapPanel::draw_range_rings() {
  if (!_range_layer || _width <= 0 || _height <= 0) return;

  if (!_gps_valid) {
    for (int i = 0; i < kRangeRingCount; ++i) {
      if (_range_rings[i]) lv_obj_add_flag(_range_rings[i], LV_OBJ_FLAG_HIDDEN);
      if (_range_labels[i]) lv_obj_add_flag(_range_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  GeoPoint gp = _gps;
  gp.set_zoom(_scrolled.zoom_level);
  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  const int32_t tile_dx = wrapped_tile_delta(gp.x_tile, _x_start, world_tiles);
  const int32_t tile_dy = (int32_t)gp.y_tile - (int32_t)_y_start;
  if (tile_dx < 0 || tile_dx >= _tiles_x || tile_dy < 0 || tile_dy >= _tiles_y) {
    for (int i = 0; i < kRangeRingCount; ++i) {
      if (_range_rings[i]) lv_obj_add_flag(_range_rings[i], LV_OBJ_FLAG_HIDDEN);
      if (_range_labels[i]) lv_obj_add_flag(_range_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }
  const float ref_lat = gp.latitude;
  const int32_t cx = tile_dx * kTileSizePx + _x_offset + gp.x_pos;
  const int32_t cy = tile_dy * kTileSizePx + _y_offset + gp.y_pos;
  if (cx < 0 || cy < 0 || cx >= _width || cy >= _height) {
    for (int i = 0; i < kRangeRingCount; ++i) {
      if (_range_rings[i]) lv_obj_add_flag(_range_rings[i], LV_OBJ_FLAG_HIDDEN);
      if (_range_labels[i]) lv_obj_add_flag(_range_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  const float mpp = metersPerPixel(ref_lat, _scrolled.zoom_level);
  if (mpp <= 0.f) {
    return;
  }
  const lv_coord_t max_r = (lv_coord_t)((_width > _height ? _width : _height));

  for (int i = 0; i < kRangeRingCount; ++i) {
    lv_obj_t* ring = _range_rings[i];
    lv_obj_t* lbl = _range_labels[i];
    if (!ring) continue;

    const uint32_t dist_m = kRangeDistancesM[i];
    const float diam_f = (float)dist_m * 2.f / mpp;
    if (!isfinite(diam_f) || diam_f < 12.f || diam_f > (float)LV_COORD_MAX) {
      lv_obj_add_flag(ring, LV_OBJ_FLAG_HIDDEN);
      if (lbl) lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    const lv_coord_t diam = (lv_coord_t)diam_f;
    const lv_coord_t r = diam / 2;
    if (r < 6 || r > max_r) {
      lv_obj_add_flag(ring, LV_OBJ_FLAG_HIDDEN);
      if (lbl) lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
      continue;
    }

    lv_obj_clear_flag(ring, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(ring, diam, diam);
    lv_obj_set_pos(ring, (lv_coord_t)(cx - r), (lv_coord_t)(cy - r));

    if (lbl) {
      lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
      lv_obj_update_layout(lbl);
      const lv_coord_t lw = lv_obj_get_width(lbl);
      const lv_coord_t lh = lv_obj_get_height(lbl);
      lv_coord_t lx = (lv_coord_t)(cx - lw / 2);
      lv_coord_t ly = (lv_coord_t)(cy - r - lh - 2);
      if (ly < 0) ly = 0;
      if (lx < 0) lx = 0;
      if (lx + lw > _width) lx = (lv_coord_t)(_width - lw);
      lv_obj_set_pos(lbl, lx, ly);
      lv_obj_move_foreground(lbl);
    }
    lv_obj_move_foreground(ring);
  }
}

void MapPanel::commit() {
  if (!_viewport || !tilesReady()) return;
  resetTransientPan();

  uint8_t dirty = _dirty;
  _dirty = DirtyNone;

  if (dirty & DirtyViewport) {
    center_view();
    if (_width <= 0 || _height <= 0) {
      markDirty(dirty);
      return;
    }
    dirty = static_cast<uint8_t>(dirty | DirtyMarkers | DirtyRings | DirtyGpsPin);
  }

  if ((dirty & DirtyTiles) || _tile_load_pending) {
    _tile_load_pending = false;
    layout_tiles();
    if (_tile_load_pending) markDirty(DirtyTiles);
  }
  if (dirty & DirtyMarkers) layout_markers();
  if (dirty & DirtyRings) draw_range_rings();
  if (dirty & DirtyGpsPin) draw_location_pins();
}

void MapPanel::refreshOverlays() {
  if (!_viewport || !tilesReady()) return;
  uint8_t dirty = _dirty;
  if ((dirty & (DirtyViewport | DirtyMarkers | DirtyRings | DirtyGpsPin)) == 0) return;
  resetTransientPan();
  uint8_t processed = DirtyNone;
  if (dirty & DirtyViewport) {
    center_view();
    if (_width <= 0 || _height <= 0) return;
    reposition_visible_tiles();
    dirty = static_cast<uint8_t>(dirty | DirtyMarkers | DirtyRings | DirtyGpsPin);
    processed = static_cast<uint8_t>(processed | DirtyViewport);
  }
  if (dirty & DirtyMarkers) {
    layout_markers();
    processed = static_cast<uint8_t>(processed | DirtyMarkers);
  }
  if (dirty & DirtyRings) {
    draw_range_rings();
    processed = static_cast<uint8_t>(processed | DirtyRings);
  }
  if (dirty & DirtyGpsPin) {
    draw_location_pins();
    processed = static_cast<uint8_t>(processed | DirtyGpsPin);
  }
  _dirty = static_cast<uint8_t>(_dirty & ~processed);
}

void MapPanel::set_gps(float lat, float lon, bool valid) {
  valid = valid && isfinite(lat) && isfinite(lon) && lat >= -90.0f && lat <= 90.0f &&
          lon >= -180.0f && lon <= 180.0f && (lat != 0.0f || lon != 0.0f);
  if (!valid) {
    if (!_gps_valid) return;
    _gps_valid = false;
    markDirty(DirtyRings | DirtyGpsPin);
    return;
  }
  const GeoPoint normalized(lat, lon, _scrolled.zoom_level);
  const bool gps_changed = !_gps_valid || _gps.latitude != normalized.latitude ||
                           _gps.longitude != normalized.longitude;
  if (!gps_changed) return;
  _gps_valid = true;
  _gps = normalized;
  markDirty(DirtyRings | DirtyGpsPin);
}

void MapPanel::set_markers(const heltec::meshcore::biz::MapPlotMarker* markers, int count) {
  int n = (markers && count > 0) ? count : 0;
  if (n > kMaxMarkers) n = kMaxMarkers;
  bool unchanged = n == _marker_count;
  for (int i = 0; unchanged && i < n; ++i) {
    const auto& src = markers[i];
    const auto& cached = _marker_data[i];
    unchanged = src.contact_index == cached.contact_index && src.glyph == cached.glyph &&
                src.is_self == cached.is_self &&
                (src.is_self || (src.lat_micro == cached.lat_micro &&
                                 src.lon_micro == cached.lon_micro));
  }
  if (unchanged) return;

  _marker_count = n;
  for (int i = 0; i < _marker_count; ++i) {
    _marker_data[i].lat_micro = markers[i].lat_micro;
    _marker_data[i].lon_micro = markers[i].lon_micro;
    _marker_data[i].contact_index = markers[i].contact_index;
    _marker_data[i].glyph = markers[i].glyph;
    _marker_data[i].is_self = markers[i].is_self;
  }
  markDirty(DirtyMarkers);
}

void MapPanel::zoom_in() {
  if (_prefs.zoom >= kZoomMax) return;
  _prefs.zoom++;
  _scrolled.set_zoom(_prefs.zoom);
  _home.set_zoom(_prefs.zoom);
  if (_gps_valid) _gps.set_zoom(_prefs.zoom);
  _prefs_dirty = true;
  request_redraw();
  save_prefs();
}

void MapPanel::zoom_out() {
  if (_prefs.zoom <= kZoomMin) return;
  _prefs.zoom--;
  _scrolled.set_zoom(_prefs.zoom);
  _home.set_zoom(_prefs.zoom);
  if (_gps_valid) _gps.set_zoom(_prefs.zoom);
  _prefs_dirty = true;
  request_redraw();
  save_prefs();
}

void MapPanel::setPanDragging(bool dragging) {
  if (_pan_dragging == dragging) return;
  _pan_dragging = dragging;
  if (_range_layer) {
    if (dragging) {
      lv_obj_add_flag(_range_layer, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(_range_layer, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (!dragging) {
    markDirty(DirtyMarkers | DirtyRings | DirtyGpsPin);
  }
}

void MapPanel::pan_pixels(int16_t dx, int16_t dy) {
  if (dx == 0 && dy == 0) return;
  _scrolled.move(dx, dy);
  _prefs_dirty = true;
  _transient_pan_x = static_cast<int16_t>(_transient_pan_x + dx);
  _transient_pan_y = static_cast<int16_t>(_transient_pan_y + dy);
  if (_tile_layer) lv_obj_set_pos(_tile_layer, _transient_pan_x, _transient_pan_y);
  if (_range_layer) lv_obj_set_pos(_range_layer, _transient_pan_x, _transient_pan_y);
  if (_marker_layer) lv_obj_set_pos(_marker_layer, _transient_pan_x, _transient_pan_y);
}

void MapPanel::resetTransientPan() {
  if (_transient_pan_x == 0 && _transient_pan_y == 0) return;
  if (_tile_layer) lv_obj_set_pos(_tile_layer, 0, 0);
  if (_range_layer) lv_obj_set_pos(_range_layer, 0, 0);
  if (_marker_layer) lv_obj_set_pos(_marker_layer, 0, 0);
  _transient_pan_x = 0;
  _transient_pan_y = 0;
}

void MapPanel::finish_pan() {
  resetTransientPan();
  request_layout();
}

void MapPanel::scroll_step(int16_t delta_x, int16_t delta_y) {
  const int16_t step = kTileSizePx / 3;
  pan_pixels((int16_t)(delta_x * step), (int16_t)(delta_y * step));
  finish_pan();
}

}  // namespace heltec::meshcore::ui::map

#endif
