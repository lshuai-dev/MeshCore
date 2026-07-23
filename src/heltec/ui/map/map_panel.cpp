#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "map_panel.hpp"

#include "map_sd.hpp"
#include "map_debug.hpp"
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
namespace {

static lv_style_t s_map_status_label_style;
static lv_style_t s_map_viewport_style;
static lv_style_t s_map_toolbar_style;
static lv_style_t s_map_toolbar_button_style;
static lv_style_t s_map_toolbar_button_checked_style;
static lv_style_t s_map_toolbar_button_pressed_style;
static lv_style_t s_map_toolbar_button_disabled_style;
static lv_style_t s_map_toolbar_label_style;
static lv_style_t s_map_toolbar_label_checked_style;
static lv_style_t s_map_toolbar_label_pressed_style;
static lv_style_t s_map_toolbar_label_disabled_style;
static lv_style_t s_map_full_layer_style;
static lv_style_t s_map_marker_layer_style;
static lv_style_t s_map_tile_style;
static lv_style_t s_map_marker_style;
static lv_style_t s_map_range_ring_style;
static lv_style_t s_map_tile_placeholder_style;
static lv_style_t s_map_marker_label_style;
static lv_style_t s_map_range_label_style;
static bool s_map_styles_ready = false;

struct MapColorStyleSlot {
  bool ready = false;
  uint32_t color = 0;
  lv_style_t style;
};

struct MapOpaStyleSlot {
  bool ready = false;
  lv_opa_t opa = LV_OPA_TRANSP;
  lv_style_t style;
};

static MapColorStyleSlot s_map_marker_color_styles[8];
static MapOpaStyleSlot s_map_range_opa_styles[8];

static void init_map_styles() {
  if (s_map_styles_ready) return;

  lv_style_init(&s_map_status_label_style);
  lv_style_set_text_color(&s_map_status_label_style, ui_color_fg());
  lv_style_set_text_align(&s_map_status_label_style, LV_TEXT_ALIGN_LEFT);

  lv_style_init(&s_map_viewport_style);
  lv_style_set_bg_opa(&s_map_viewport_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_map_viewport_style, 0);

  lv_style_init(&s_map_toolbar_style);
  lv_style_set_radius(&s_map_toolbar_style, 4);
  lv_style_set_bg_opa(&s_map_toolbar_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_toolbar_style, 0);
  lv_style_set_shadow_width(&s_map_toolbar_style, 0);

  lv_style_init(&s_map_toolbar_button_style);
  lv_style_set_radius(&s_map_toolbar_button_style, 3);
  lv_style_set_border_width(&s_map_toolbar_button_style, 1);
  lv_style_set_border_color(&s_map_toolbar_button_style, ui_color_accent());
  lv_style_set_border_opa(&s_map_toolbar_button_style, LV_OPA_COVER);
  lv_style_set_shadow_width(&s_map_toolbar_button_style, 0);
  lv_style_set_bg_opa(&s_map_toolbar_button_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_button_checked_style);
  lv_style_set_border_color(&s_map_toolbar_button_checked_style, ui_color_success());
  lv_style_set_bg_opa(&s_map_toolbar_button_checked_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_button_pressed_style);
  lv_style_set_border_color(&s_map_toolbar_button_pressed_style, ui_color_error());
  lv_style_set_bg_opa(&s_map_toolbar_button_pressed_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_button_disabled_style);
  lv_style_set_border_opa(&s_map_toolbar_button_disabled_style, ui_effective_opa(LV_OPA_50));
  lv_style_set_bg_opa(&s_map_toolbar_button_disabled_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_label_style);
  lv_style_set_text_color(&s_map_toolbar_label_style, ui_color_accent());
  lv_style_set_text_font(&s_map_toolbar_label_style, LV_FONT_DEFAULT);

  lv_style_init(&s_map_toolbar_label_checked_style);
  lv_style_set_text_color(&s_map_toolbar_label_checked_style, ui_color_success());

  lv_style_init(&s_map_toolbar_label_pressed_style);
  lv_style_set_text_color(&s_map_toolbar_label_pressed_style, ui_color_error());

  lv_style_init(&s_map_toolbar_label_disabled_style);
  lv_style_set_text_color(&s_map_toolbar_label_disabled_style, lv_color_hex(0x607080));

  lv_style_init(&s_map_full_layer_style);
  lv_style_set_bg_opa(&s_map_full_layer_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_full_layer_style, 0);

  lv_style_init(&s_map_marker_layer_style);
  lv_style_set_bg_opa(&s_map_marker_layer_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_marker_layer_style, 0);

  lv_style_init(&s_map_tile_style);
  lv_style_set_border_width(&s_map_tile_style, 0);
  lv_style_set_bg_opa(&s_map_tile_style, LV_OPA_COVER);

  lv_style_init(&s_map_marker_style);
  lv_style_set_radius(&s_map_marker_style, LV_RADIUS_CIRCLE);
  lv_style_set_bg_color(&s_map_marker_style, ui_color_success());
  lv_style_set_bg_opa(&s_map_marker_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_map_marker_style, 1);
  lv_style_set_border_color(&s_map_marker_style, ui_color_fg());

  lv_style_init(&s_map_range_ring_style);
  lv_style_set_radius(&s_map_range_ring_style, LV_RADIUS_CIRCLE);
  lv_style_set_bg_opa(&s_map_range_ring_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_range_ring_style, 1);
  lv_style_set_border_color(&s_map_range_ring_style, ui_color_fg());

  lv_style_init(&s_map_tile_placeholder_style);
  lv_style_set_text_color(&s_map_tile_placeholder_style, lv_color_hex(0x606060));
  lv_style_set_text_align(&s_map_tile_placeholder_style, LV_TEXT_ALIGN_CENTER);

  lv_style_init(&s_map_marker_label_style);
  lv_style_set_text_color(&s_map_marker_label_style, ui_color_bg());

  lv_style_init(&s_map_range_label_style);
  lv_style_set_text_color(&s_map_range_label_style, ui_color_fg());
  lv_style_set_bg_opa(&s_map_range_label_style, LV_OPA_TRANSP);

  s_map_styles_ready = true;
}

static lv_style_t* marker_color_style(lv_color_t color) {
  const uint32_t color32 = lv_color_to32(color);
  for (MapColorStyleSlot& slot : s_map_marker_color_styles) {
    if (slot.ready && slot.color == color32) return &slot.style;
  }
  for (MapColorStyleSlot& slot : s_map_marker_color_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.color = color32;
    lv_style_init(&slot.style);
    lv_style_set_bg_color(&slot.style, color);
    return &slot.style;
  }
  return nullptr;
}

static lv_style_t* range_opa_style(lv_opa_t opa) {
  for (MapOpaStyleSlot& slot : s_map_range_opa_styles) {
    if (slot.ready && slot.opa == opa) return &slot.style;
  }
  for (MapOpaStyleSlot& slot : s_map_range_opa_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.opa = opa;
    lv_style_init(&slot.style);
    lv_style_set_border_opa(&slot.style, ui_effective_opa(opa));
    return &slot.style;
  }
  return nullptr;
}

}  // namespace

bool ui_map_widget_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  init_map_styles();

  switch (ht_id(obj)) {
    case meta_id::MapStatusLabel:
      lv_obj_add_style(obj, &s_map_status_label_style, LV_PART_MAIN);
      return true;

    case meta_id::MapViewport:
      lv_obj_add_style(obj, &s_map_viewport_style, LV_PART_MAIN);
      return true;

    case meta_id::MapToolbar:
      lv_obj_add_style(obj, &s_map_toolbar_style, LV_PART_MAIN);
      return true;

    case meta_id::MapToolbarButton:
      lv_obj_add_style(obj, &s_map_toolbar_button_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_map_toolbar_button_checked_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_add_style(obj, &s_map_toolbar_button_pressed_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_map_toolbar_button_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      return true;

    case meta_id::MapToolbarButtonLabel:
      lv_obj_add_style(obj, &s_map_toolbar_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_map_toolbar_label_checked_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_add_style(obj, &s_map_toolbar_label_pressed_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_map_toolbar_label_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      return true;

    case meta_id::MapTileLayer:
    case meta_id::MapRangeLayer:
      lv_obj_add_style(obj, &s_map_full_layer_style, LV_PART_MAIN);
      return true;

    case meta_id::MapMarkerLayer:
      lv_obj_add_style(obj, &s_map_marker_layer_style, LV_PART_MAIN);
      return true;

    case meta_id::MapTile:
      lv_obj_add_style(obj, &s_map_tile_style, LV_PART_MAIN);
      return true;

    case meta_id::MapTileImage:
      return true;

    case meta_id::MapMarker:
      lv_obj_add_style(obj, &s_map_marker_style, LV_PART_MAIN);
      return true;

    case meta_id::MapRangeRing:
      lv_obj_add_style(obj, &s_map_range_ring_style, LV_PART_MAIN);
      return true;

    case meta_id::MapTilePlaceholder:
      lv_obj_add_style(obj, &s_map_tile_placeholder_style, LV_PART_MAIN);
      return true;

    case meta_id::MapMarkerLabel:
      lv_obj_add_style(obj, &s_map_marker_label_style, LV_PART_MAIN);
      return true;

    case meta_id::MapRangeLabel:
      lv_obj_add_style(obj, &s_map_range_label_style, LV_PART_MAIN);
      return true;

    default:
      return false;
  }
}

void ui_map_marker_apply_color(_lv_obj_t* obj, lv_color_t color) {
  if (!obj) return;
  if (lv_style_t* style = marker_color_style(color)) {
    lv_obj_add_style(obj, style, LV_PART_MAIN);
  }
}

void ui_map_range_ring_apply_opa(_lv_obj_t* obj, lv_opa_t opa) {
  if (!obj) return;
  if (lv_style_t* style = range_opa_style(opa)) {
    lv_obj_add_style(obj, style, LV_PART_MAIN);
  }
}

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
#define MAP_UI_LAYOUT_BUDGET_US 20000
#endif

constexpr int kPngLoadsPerTick = MAP_UI_PNG_LOADS_PER_TICK;
constexpr int kSdStatPerTick = MAP_UI_SD_STAT_PER_TICK;

#if defined(MESH_DEBUG) && MESH_DEBUG
// Keep successful header traces bounded: a normal redraw touches several
// tiles, while failures remain visible long enough to diagnose a bad path or
// unreadable PNG without flooding the serial port indefinitely.
constexpr uint16_t kPngInfoSuccessLogLimit = 12;
constexpr uint16_t kPngInfoFailureLogLimit = 24;
static uint16_t s_png_info_success_logs = 0;
static uint16_t s_png_info_failure_logs = 0;
#endif

constexpr int kRangeRingCount = 4;
constexpr uint32_t kRangeDistancesM[kRangeRingCount] = {500, 1000, 2000, 5000};

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

void format_range_label(char* buf, size_t n, uint32_t meters) {
  if (!buf || 0 == n) return;
  if (meters >= 1000U && 0U == (meters % 1000U)) {
    snprintf(buf, n, "%lukm", (unsigned long)(meters / 1000U));
  } else if (meters >= 1000U) {
    snprintf(buf, n, "%.1fkm", (double)meters / 1000.0);
  } else {
    snprintf(buf, n, "%lum", (unsigned long)meters);
  }
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
#if defined(MESH_DEBUG) && MESH_DEBUG
  s_png_info_success_logs = 0;
  s_png_info_failure_logs = 0;
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
  MAP_UI_LOG("load_prefs loaded=%d style=%s z=%u center=%.4f,%.4f sd=%d",
             _prefs_loaded ? 1 : 0, _prefs.tile_style, (unsigned)_prefs.zoom,
             (double)_scrolled.latitude, (double)_scrolled.longitude, _sd_tiles ? 1 : 0);
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
  MAP_UI_LOG("applySdPrefs changed=%d style=%s z=%u", changed ? 1 : 0, _prefs.tile_style,
             (unsigned)_prefs.zoom);
}

void MapPanel::refreshSdTiles() {
  const bool was = _sd_tiles;
  _sd_tiles = map_sd_ready();
  if (was != _sd_tiles) {
    MAP_UI_LOG("refreshSdTiles sd=%d", _sd_tiles ? 1 : 0);
    request_redraw();
  }
}

void MapPanel::syncViewportLayout() {
  if (!_viewport) return;
  lv_obj_update_layout(_viewport);
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
  MAP_UI_LOG("syncViewportLayout vp=%dx%d", (int)_width, (int)_height);
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
  if (!t.root) {
    MAP_UI_LOG("createTileSlot %d root fail", idx);
    return false;
  }
  lv_obj_set_size(t.root, map::kTileSizePx, map::kTileSizePx);
  lv_obj_set_style_pad_all(t.root, 0, LV_PART_MAIN);
  lv_obj_clear_flag(t.root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(t.root, LV_OBJ_FLAG_HIDDEN);

  t.img = ht_img_create(t.root, meta_id::MapTileImage);
  if (!t.img) {
    MAP_UI_LOG("createTileSlot %d img fail", idx);
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
  MAP_UI_LOG("buildPendingTiles %d/%d", _tiles_built, kMaxTiles);
  if (tilesReady()) {
    markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
  }
  return tilesReady();
}

void MapPanel::ensureMarkerSlot(int idx) {
  if (idx < 0 || idx >= kMaxMarkers || !_marker_layer) return;
  MarkerSlot& m = _markers[idx];
  if (m.root) return;

  m.root = ht_obj_create(_marker_layer, meta_id::MapMarker);
  if (!m.root) return;
  lv_obj_set_size(m.root, 14, 14);
  lv_obj_clear_flag(m.root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(m.root, LV_OBJ_FLAG_HIDDEN);
  m.label = ht_label_create(m.root, meta_id::MapMarkerLabel, "?");
  if (m.label) lv_obj_align(m.label, LV_ALIGN_CENTER, 0, 0);
}

void MapPanel::attach(_lv_obj_t* viewport) {
  if (_viewport) return;
  _viewport = viewport;
  if (!_viewport) return;

  _tiles_built = 0;
  _tile_build_failed = false;
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
    MAP_UI_LOG("attach layer create failed");
    return;
  }
  _lv_obj_t* const passive_layers[] = {_tile_layer, _range_layer};
  for (_lv_obj_t* layer : passive_layers) {
    lv_obj_set_size(layer, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(layer, 0, LV_PART_MAIN);
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
      lv_obj_set_style_pad_all(_range_labels[i], 0, LV_PART_MAIN);
      lv_obj_clear_flag(_range_labels[i], LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(_range_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  _gps_pin = make_pin(_marker_layer, "G", ui_color_success());
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);

  MAP_UI_LOG("attach vp=%dx%d tile_layer=%p marker_layer=%p", (int)lv_obj_get_width(_viewport),
             (int)lv_obj_get_height(_viewport), (void*)_tile_layer, (void*)_marker_layer);
}

void MapPanel::center_view() {
  if (!_viewport) return;
  lv_obj_update_layout(_viewport);
  _width = lv_obj_get_width(_viewport);
  _height = lv_obj_get_height(_viewport);
  if (_width <= 0 || _height <= 0) {
    MAP_UI_LOG("center_view skip zero size %dx%d", (int)_width, (int)_height);
    return;
  }

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
  MAP_UI_LOG("center_view %dx%d z=%u tile=%lu,%lu grid=%u+%u off=%d,%d start=%lu,%lu", (int)_width,
             (int)_height, (unsigned)_scrolled.zoom_level, (unsigned long)_scrolled.x_tile,
             (unsigned long)_scrolled.y_tile, (unsigned)_tiles_x, (unsigned)_tiles_y, (int)_x_offset,
             (int)_y_offset, (unsigned long)_x_start, (unsigned long)_y_start);
}

void MapPanel::request_layout() {
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
  _tile_load_pending = true;
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
    t.miss_path[0] = '\0';
    if (t.root) lv_obj_add_flag(t.root, LV_OBJ_FLAG_HIDDEN);
  }
}

void MapPanel::layout_tiles() {
  if (!_tile_layer || !tilesReady()) return;

  MAP_UI_LOG("[tiles] layout begin sd=%d style=%s z=%u grid=%ux%u offset=%d,%d "
             "start=%lu,%lu pending=%d",
             _sd_tiles ? 1 : 0, _prefs.tile_style, (unsigned)_scrolled.zoom_level,
             (unsigned)_tiles_x, (unsigned)_tiles_y, (int)_x_offset, (int)_y_offset,
             (unsigned long)_x_start, (unsigned long)_y_start,
             _tile_load_pending ? 1 : 0);

  const uint32_t t_total = micros();
  uint32_t stat_us = 0;
  int stat_calls = 0;
  const int png_cap = _pan_dragging ? 0 : kPngLoadsPerTick;
  int stat_budget = _pan_dragging ? 0 : kSdStatPerTick;

  struct CellNeed {
    uint32_t xt = 0;
    uint32_t yt = 0;
    int16_t px = 0;
    int16_t py = 0;
    int slot = -1;
  };

  CellNeed cells[kMaxTiles]{};
  int n_cells = 0;
  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  for (uint8_t y = 0; y < _tiles_y && n_cells < kMaxTiles; ++y) {
    for (uint8_t x = 0; x < _tiles_x && n_cells < kMaxTiles; ++x) {
      const uint32_t yt = _y_start + y;
      if (yt >= world_tiles) continue;
      CellNeed& c = cells[n_cells++];
      c.xt = wrap_tile(_x_start + x, world_tiles);
      c.yt = yt;
      c.px = (int16_t)(x * kTileSizePx + _x_offset);
      c.py = (int16_t)(y * kTileSizePx + _y_offset);
    }
  }

  bool slot_used[kMaxTiles]{};
  int tiles_reused = 0;

  for (int ci = 0; ci < n_cells; ++ci) {
    const uint32_t xt = cells[ci].xt;
    const uint32_t yt = cells[ci].yt;
    for (int si = 0; si < kMaxTiles; ++si) {
      if (slot_used[si] || !_tiles[si].root) continue;
      const TileSlot& t = _tiles[si];
      if (t.x_tile != xt || t.y_tile != yt) continue;
      cells[ci].slot = si;
      slot_used[si] = true;
      ++tiles_reused;
      break;
    }
  }

  for (int ci = 0; ci < n_cells; ++ci) {
    if (cells[ci].slot >= 0) continue;
    for (int si = 0; si < kMaxTiles; ++si) {
      if (slot_used[si] || !_tiles[si].root) continue;
      cells[ci].slot = si;
      slot_used[si] = true;
      break;
    }
  }

  for (int si = 0; si < kMaxTiles; ++si) {
    _tiles[si].active = false;
    if (!_tiles[si].root) continue;
    // Hide every slot before the budgeted loop.  If the loop yields midway,
    // cells not processed in this tick must not keep displaying a stale tile
    // at its previous position.
    lv_obj_add_flag(_tiles[si].root, LV_OBJ_FLAG_HIDDEN);
    if (!slot_used[si]) {
      if (_tiles[si].loaded_path[0] != '\0') {
        lv_img_cache_invalidate_src(_tiles[si].loaded_path);
        if (_tiles[si].img) lv_img_set_src(_tiles[si].img, nullptr);
        _tiles[si].loaded_path[0] = '\0';
      }
      _tiles[si].miss_path[0] = '\0';
    }
  }

  int png_loads = 0;
  int png_info_failures = 0;
  int tiles_found = 0;
  int tiles_shown = 0;
  uint32_t bind_us = 0;
  char sample_path[96] = {};
  for (int ci = 0; ci < n_cells; ++ci) {
    if ((micros() - t_total) >= (uint32_t)MAP_UI_LAYOUT_BUDGET_US) {
      _tile_load_pending = true;
      break;
    }

    const int si = cells[ci].slot;
    if (si < 0) continue;

    TileSlot& t = _tiles[si];
    if (!t.root) continue;
    const uint32_t xt = cells[ci].xt;
    const uint32_t yt = cells[ci].yt;
    const bool tile_identity_changed = t.x_tile != xt || t.y_tile != yt;
    if (tile_identity_changed) {
      if (t.loaded_path[0] != '\0') {
        lv_img_cache_invalidate_src(t.loaded_path);
        if (t.img) lv_img_set_src(t.img, nullptr);
        t.loaded_path[0] = '\0';
      }
      t.miss_path[0] = '\0';
    }
    t.x_tile = xt;
    t.y_tile = yt;
    t.active = true;
    lv_obj_clear_flag(t.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(t.root, cells[ci].px, cells[ci].py);

    char path[96] = {};
    bool shown_img = false;
    bool has_tile = false;
    bool lookup_deferred = false;
    if (_sd_tiles && t.img) {
      if (!tile_identity_changed && t.loaded_path[0] != '\0') {
        has_tile = true;
        strncpy(path, t.loaded_path, sizeof(path) - 1);
      } else if (!tile_identity_changed && t.miss_path[0] != '\0') {
        has_tile = false;
      } else if (stat_budget <= 0) {
        _tile_load_pending = true;
        lookup_deferred = true;
      } else {
        const uint32_t t_stat = mapUiPerfNowUs();
#if defined(ESP_PLATFORM)
        esp_task_wdt_reset();
#endif
        has_tile = map_sd_resolve_tile_path(path, sizeof(path), _prefs.tile_style,
                                            _scrolled.zoom_level, xt, yt);
        stat_us += micros() - t_stat;
        ++stat_calls;
        --stat_budget;
        if (!has_tile) {
          map_sd_tile_path(path, sizeof(path), _prefs.tile_style, _scrolled.zoom_level, xt, yt);
          strncpy(t.miss_path, path, sizeof(t.miss_path) - 1);
          t.miss_path[sizeof(t.miss_path) - 1] = '\0';
          MAP_UI_LOG("[tiles] miss style=%s z=%u x=%lu y=%lu fallback=%s",
                     _prefs.tile_style, (unsigned)_scrolled.zoom_level,
                     (unsigned long)xt, (unsigned long)yt, path[0] ? path : "-");
        } else {
          t.miss_path[0] = '\0';
          MAP_UI_LOG("[tiles] hit style=%s z=%u x=%lu y=%lu path=%s",
                     _prefs.tile_style, (unsigned)_scrolled.zoom_level,
                     (unsigned long)xt, (unsigned long)yt, path);
        }
      }
    }
    if (has_tile) {
      ++tiles_found;
      if (0 == sample_path[0]) strncpy(sample_path, path, sizeof(sample_path) - 1);
    }
    if (has_tile) {
      if (t.loaded_path[0] != '\0' && 0 == strcmp(t.loaded_path, path)) {
        lv_obj_clear_flag(t.img, LV_OBJ_FLAG_HIDDEN);
        if (t.placeholder) lv_obj_add_flag(t.placeholder, LV_OBJ_FLAG_HIDDEN);
        shown_img = true;
        ++tiles_shown;
      } else if (png_loads < png_cap) {
        bool info_ok = true;
#if defined(MESH_DEBUG) && MESH_DEBUG
        lv_img_header_t header{};
        const lv_res_t info_res = lv_img_decoder_get_info(path, &header);
        info_ok = info_res == LV_RES_OK && header.w > 0 && header.h > 0;
        if (info_ok) {
          if (s_png_info_success_logs < kPngInfoSuccessLogLimit) {
            MAP_UI_LOG("[tiles] png info OK slot=%d path=%s w=%d h=%d cf=%u", si, path,
                       (int)header.w, (int)header.h, (unsigned)header.cf);
            ++s_png_info_success_logs;
          }
        } else if (s_png_info_failure_logs < kPngInfoFailureLogLimit) {
          MAP_UI_LOG("[tiles] png info FAIL slot=%d path=%s res=%d w=%d h=%d cf=%u", si,
                     path, (int)info_res, (int)header.w, (int)header.h,
                     (unsigned)header.cf);
          ++s_png_info_failure_logs;
        }
#endif
        if (!info_ok) {
          ++png_info_failures;
        }
        const uint32_t t_bind = mapUiPerfNowUs();
        lv_img_set_src(t.img, path);
        bind_us += micros() - t_bind;
        MAP_UI_LOG("[tiles] png bind queued slot=%d info_ok=%d path=%s", si,
                   info_ok ? 1 : 0, path);
        strncpy(t.loaded_path, path, sizeof(t.loaded_path) - 1);
        t.loaded_path[sizeof(t.loaded_path) - 1] = '\0';
#if defined(ESP_PLATFORM)
        esp_task_wdt_reset();
#endif
        yield();
        lv_obj_clear_flag(t.img, LV_OBJ_FLAG_HIDDEN);
        if (t.placeholder) lv_obj_add_flag(t.placeholder, LV_OBJ_FLAG_HIDDEN);
        shown_img = true;
        ++png_loads;
        ++tiles_shown;
      }
    } else if (t.img) {
      lv_obj_add_flag(t.img, LV_OBJ_FLAG_HIDDEN);
    }
    if (has_tile && !shown_img) _tile_load_pending = true;
    if (!shown_img && t.placeholder) {
      lv_obj_clear_flag(t.placeholder, LV_OBJ_FLAG_HIDDEN);
      char txt[48];
      const char* hint = !_sd_tiles ? "(no SD)" : ((has_tile || lookup_deferred) ? "(loading)" : "(missing)");
      snprintf(txt, sizeof(txt), "z%u\n%lu/%lu\n%s", (unsigned)_scrolled.zoom_level,
               (unsigned long)xt, (unsigned long)yt, hint);
      lv_label_set_text(t.placeholder, txt);
    }
  }
  if (png_loads > 0 || _tile_load_pending || stat_calls > 0 || tiles_reused > 0 ||
      _pan_dragging) {
    MAP_UI_LOG("layout_tiles cells=%d reused=%d resolved=%d visible=%d bind=%d info_fail=%d pending=%d dragging=%d sample=%s",
               n_cells, tiles_reused, tiles_found, tiles_shown, png_loads, png_info_failures,
               _tile_load_pending ? 1 : 0, _pan_dragging ? 1 : 0,
               sample_path[0] ? sample_path : "-");
  }
  mapUiLogPerfDetail("layout_tiles", t_total,
                     "grid=%ux%u reused=%d stat=%d/%luus bind=%d/%luus pending=%d",
                     (unsigned)_tiles_x, (unsigned)_tiles_y, tiles_reused, stat_calls,
                     (unsigned long)stat_us, png_loads, (unsigned long)bind_us,
                     _tile_load_pending ? 1 : 0);
}

void MapPanel::layout_markers() {
  const uint32_t t0 = mapUiPerfNowUs();
  _visible_marker_count = 0;
  for (MarkerSlot& m : _markers) {
    m.active = false;
    if (m.root) lv_obj_add_flag(m.root, LV_OBJ_FLAG_HIDDEN);
  }

  int skipped_self = 0;
  int skipped_tile = 0;
  int skipped_bounds = 0;
  int slot = 0;
  const uint32_t world_tiles = 1U << clampMapZoom(_scrolled.zoom_level);
  for (int i = 0; i < _marker_count && slot < kMaxMarkers; ++i) {
    const auto& src = _marker_data[i];
    if (src.is_self) {
      ++skipped_self;
      continue;
    }
    GeoPoint gp((float)src.lat_deg, (float)src.lon_deg, _scrolled.zoom_level);
    const int32_t tile_dx = wrapped_tile_delta(gp.x_tile, _x_start, world_tiles);
    const int32_t tile_dy = (int32_t)gp.y_tile - (int32_t)_y_start;
    if (tile_dx < 0 || tile_dx >= _tiles_x || tile_dy < 0 || tile_dy >= _tiles_y) {
      ++skipped_tile;
      continue;
    }
    const int16_t tx = (int16_t)(tile_dx * kTileSizePx + _x_offset + gp.x_pos);
    const int16_t ty = (int16_t)(tile_dy * kTileSizePx + _y_offset + gp.y_pos);
    if (tx < -8 || ty < -8 || tx > _width + 8 || ty > _height + 8) {
      ++skipped_bounds;
      continue;
    }

    MarkerSlot& m = _markers[slot];
    ensureMarkerSlot(slot);
    if (!m.root) continue;
    m.active = true;
    _visible_marker_count++;
    lv_obj_clear_flag(m.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(m.root, (lv_coord_t)(tx - 7), (lv_coord_t)(ty - 7));
    if (m.label) {
      char ch[2] = {src.label[0] ? src.label[0] : '?', '\0'};
      lv_label_set_text(m.label, ch);
    }
    ++slot;
  }

  static int s_last_in = -1;
  static int s_last_vis = -1;
  if (_marker_count != s_last_in || _visible_marker_count != s_last_vis) {
    MAP_UI_LOG("markers layout in=%d vis=%d skip self=%d tile=%d bounds=%d grid=%u+%u start=%lu,%lu",
               _marker_count, _visible_marker_count, skipped_self, skipped_tile, skipped_bounds,
               (unsigned)_tiles_x, (unsigned)_tiles_y, (unsigned long)_x_start,
               (unsigned long)_y_start);
    for (int i = 0; i < _marker_count; ++i) {
      const auto& src = _marker_data[i];
      if (src.is_self) continue;
      GeoPoint gp((float)src.lat_deg, (float)src.lon_deg, _scrolled.zoom_level);
      const int32_t tile_dx = wrapped_tile_delta(gp.x_tile, _x_start, world_tiles);
      const int32_t tile_dy = (int32_t)gp.y_tile - (int32_t)_y_start;
      if (tile_dx < 0 || tile_dx >= _tiles_x || tile_dy < 0 || tile_dy >= _tiles_y) {
        continue;
      }
      const int16_t tx = (int16_t)(tile_dx * kTileSizePx + _x_offset + gp.x_pos);
      const int16_t ty = (int16_t)(tile_dy * kTileSizePx + _y_offset + gp.y_pos);
      if (tx < -8 || ty < -8 || tx > _width + 8 || ty > _height + 8) continue;
      MAP_UI_LOG("marker draw %c (%.4f,%.4f) px=%d,%d idx=%d", src.label[0] ? src.label[0] : '?',
                 (double)src.lat_deg, (double)src.lon_deg, (int)tx, (int)ty, i);
    }
    s_last_in = _marker_count;
    s_last_vis = _visible_marker_count;
  }
  MAP_UI_PERF("layout_markers", t0);
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
  const uint32_t t0 = mapUiPerfNowUs();
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
  MAP_UI_PERF("draw_location_pins", t0);
}

void MapPanel::draw_range_rings() {
  const uint32_t t0 = mapUiPerfNowUs();
  if (!_range_layer || _width <= 0 || _height <= 0) return;

  if (!_gps_valid) {
    for (int i = 0; i < kRangeRingCount; ++i) {
      if (_range_rings[i]) lv_obj_add_flag(_range_rings[i], LV_OBJ_FLAG_HIDDEN);
      if (_range_labels[i]) lv_obj_add_flag(_range_labels[i], LV_OBJ_FLAG_HIDDEN);
    }
    MAP_UI_PERF("draw_range_rings", t0);
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
    MAP_UI_PERF("draw_range_rings", t0);
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
    MAP_UI_PERF("draw_range_rings", t0);
    return;
  }

  const float mpp = metersPerPixel(ref_lat, _scrolled.zoom_level);
  if (mpp <= 0.f) {
    MAP_UI_PERF("draw_range_rings", t0);
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
      char txt[12];
      format_range_label(txt, sizeof(txt), dist_m);
      lv_label_set_text(lbl, txt);
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
  MAP_UI_PERF("draw_range_rings", t0);
}

void MapPanel::commit() {
  if (!_viewport) {
    MAP_UI_LOG("[tiles] commit skipped: no viewport");
    return;
  }
  if (!tilesReady()) {
    MAP_UI_LOG("[tiles] commit skipped: slots=%d/%d build_failed=%d",
               _tiles_built, kMaxTiles, _tile_build_failed ? 1 : 0);
    return;
  }
  resetTransientPan();

  const uint32_t t0 = mapUiPerfNowUs();
  uint8_t dirty = _dirty;
  _dirty = DirtyNone;

  if (dirty & DirtyViewport) {
    center_view();
    if (_width <= 0 || _height <= 0) {
      markDirty(dirty);
      MAP_UI_LOG("commit defer viewport zero");
      return;
    }
  }

  uint32_t tiles_us = 0;
  if ((dirty & DirtyTiles) || _tile_load_pending) {
    _tile_load_pending = false;
    const uint32_t t_tiles = mapUiPerfNowUs();
    layout_tiles();
    tiles_us = micros() - t_tiles;
    if (_tile_load_pending) markDirty(DirtyTiles);
  }
  if (dirty & DirtyMarkers) layout_markers();
  if (dirty & DirtyRings) draw_range_rings();
  if (dirty & DirtyGpsPin) draw_location_pins();

  mapUiLogPerfDetail("commit", t0, "dirty=0x%02x tiles_us=%lu pending=%d z=%u",
                     (unsigned)dirty, (unsigned long)tiles_us,
                     tilesLoadPending() ? 1 : 0, (unsigned)_prefs.zoom);
}

void MapPanel::refreshOverlays() {
  if (!_viewport || !tilesReady()) return;
  resetTransientPan();
  center_view();
  if (_width <= 0 || _height <= 0) {
    markDirty(DirtyViewport | DirtyMarkers | DirtyRings | DirtyGpsPin);
    return;
  }
  reposition_visible_tiles();
  layout_markers();
  draw_range_rings();
  draw_location_pins();
  _dirty = static_cast<uint8_t>(_dirty & DirtyTiles);
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

namespace {

bool markersEqual(const heltec::meshcore::biz::MapPlotMarker* a,
                  const heltec::meshcore::biz::MapPlotMarker* b, int count) {
  if (count <= 0) return true;
  if (!a || !b) return false;
  for (int i = 0; i < count; ++i) {
    if (a[i].lat_deg != b[i].lat_deg || a[i].lon_deg != b[i].lon_deg) return false;
    if (a[i].is_self != b[i].is_self) return false;
    if (a[i].contact_index != b[i].contact_index) return false;
    if (strncmp(a[i].label, b[i].label, sizeof(a[i].label)) != 0) return false;
  }
  return true;
}

}  // namespace

void MapPanel::set_markers(const heltec::meshcore::biz::MapPlotMarker* markers, int count) {
  int n = (markers && count > 0) ? count : 0;
  if (n > kMaxMarkers) n = kMaxMarkers;
  if (n == _marker_count && markersEqual(markers, _marker_data, n)) {
    return;
  }

  const int prev = _marker_count;
  _marker_count = n;
  if (markers && _marker_count > 0) {
    memcpy(_marker_data, markers, sizeof(heltec::meshcore::biz::MapPlotMarker) * (size_t)_marker_count);
  }
  if (_marker_count != prev) {
    MAP_UI_LOG("markers set count=%d", _marker_count);
    for (int i = 0; i < _marker_count; ++i) {
      const auto& m = _marker_data[i];
      if (m.is_self) continue;
      MAP_UI_LOG("markers src[%d] %c (%.4f,%.4f) contact=%d", i, m.label[0] ? m.label[0] : '?',
                 (double)m.lat_deg, (double)m.lon_deg, m.contact_index);
    }
  }
  markDirty(DirtyMarkers);
}

void MapPanel::zoom_in() {
  const uint32_t t0 = mapUiPerfNowUs();
  if (_prefs.zoom >= kZoomMax) return;
  _prefs.zoom++;
  _scrolled.set_zoom(_prefs.zoom);
  _home.set_zoom(_prefs.zoom);
  if (_gps_valid) _gps.set_zoom(_prefs.zoom);
  _prefs_dirty = true;
  request_redraw();
  save_prefs();
  MAP_UI_PERF("zoom_in", t0);
}

void MapPanel::zoom_out() {
  const uint32_t t0 = mapUiPerfNowUs();
  if (_prefs.zoom <= kZoomMin) return;
  _prefs.zoom--;
  _scrolled.set_zoom(_prefs.zoom);
  _home.set_zoom(_prefs.zoom);
  if (_gps_valid) _gps.set_zoom(_prefs.zoom);
  _prefs_dirty = true;
  request_redraw();
  save_prefs();
  MAP_UI_PERF("zoom_out", t0);
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
  MAP_UI_LOG("pan dragging=%d pending=%d", dragging ? 1 : 0, tilesLoadPending() ? 1 : 0);
}

void MapPanel::pan_pixels(int16_t dx, int16_t dy) {
  const uint32_t t0 = mapUiPerfNowUs();
  if (dx == 0 && dy == 0) return;
  _scrolled.move(dx, dy);
  _prefs_dirty = true;
  _transient_pan_x = static_cast<int16_t>(_transient_pan_x + dx);
  _transient_pan_y = static_cast<int16_t>(_transient_pan_y + dy);
  if (_tile_layer) lv_obj_set_pos(_tile_layer, _transient_pan_x, _transient_pan_y);
  if (_range_layer) lv_obj_set_pos(_range_layer, _transient_pan_x, _transient_pan_y);
  if (_marker_layer) lv_obj_set_pos(_marker_layer, _transient_pan_x, _transient_pan_y);
  MAP_UI_PERF("pan_pixels", t0);
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
  markDirty(DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport);
}

void MapPanel::scroll_step(int16_t delta_x, int16_t delta_y) {
  const int16_t step = kTileSizePx / 3;
  pan_pixels((int16_t)(delta_x * step), (int16_t)(delta_y * step));
  finish_pan();
}

}  // namespace heltec::meshcore::ui::map

#endif
