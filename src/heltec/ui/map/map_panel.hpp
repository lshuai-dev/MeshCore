#pragma once

#include "geo_point.hpp"
#include "map_panel_ids.hpp"
#include "map_prefs.hpp"
#include "ui/core/biz_facade.hpp"

#include <lvgl.h>
#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::ui::map {

enum MapDirty : uint8_t {
  DirtyNone = 0,
  DirtyTiles = 1 << 0,
  DirtyMarkers = 1 << 1,
  DirtyRings = 1 << 2,
  DirtyGpsPin = 1 << 3,
  DirtyViewport = 1 << 4,
};

/** Slippy-map panel (Meshtastic device-ui MapPanel, adapted for LVGL 8). */
class MapPanel {
 public:
  static constexpr int kMaxTiles = 12;
  static constexpr int kMaxMarkers = 40;

  void attach(_lv_obj_t* viewport);
  bool attached() const { return _viewport != nullptr && _tile_layer != nullptr; }
  /** Create tile slots in small batches (internal heap is tight on some boards). */
  bool buildPendingTiles(int max_tiles);
  /** Pre-create tile and marker object pools in bounded batches. */
  bool prewarmPools(int max_tiles, int max_markers);
  bool tilesReady() const { return _tiles_built >= kMaxTiles; }
  bool markerPoolReady() const { return _markers_built >= kMaxMarkers; }
  bool poolsReady() const { return tilesReady() && markerPoolReady(); }
  bool tileBuildFailed() const { return _tile_build_failed; }
  bool poolBuildFailed() const { return _tile_build_failed || _marker_build_failed; }
  void load_prefs(const float* gps_home_deg = nullptr);
  /** Apply SD tile style/zoom after the card becomes available. */
  void applySdPrefs();
  void save_prefs();

  void set_gps(float lat, float lon, bool valid);
  void set_markers(const heltec::meshcore::biz::MapPlotMarker* markers, int count);

  void zoom_in();
  void zoom_out();
  void pan_pixels(int16_t dx, int16_t dy);
  void finish_pan();
  void scroll_step(int16_t delta_x, int16_t delta_y);
  void setPanDragging(bool dragging);
  bool panDragging() const { return _pan_dragging; }

  void commit();
  uint8_t zoom() const { return _prefs.zoom; }
  float center_lat() const { return _scrolled.latitude; }
  float center_lon() const { return _scrolled.longitude; }
  bool gps_valid() const { return _gps_valid; }
  bool hasSavedCenter() const { return _prefs_loaded; }
  bool sd_tiles() const { return _sd_tiles; }
  int visibleMarkerCount() const { return _visible_marker_count; }
  void refreshSdTiles();
  /** Top map overlay for pan gestures (above tiles/range rings). */
  _lv_obj_t* touchLayer() const { return _marker_layer; }
  /** Re-measure viewport and queue tile redraw (call after flex layout). */
  void syncViewportLayout();
  /** Move the map center; persist it only when requested. */
  void centerOnLocation(float lat, float lon, bool persist = true);
  /** Refresh markers/rings/GPS pin without tile IO. */
  void refreshOverlays();
  bool tilesLoadPending() const {
    return !_tile_build_failed && (_tile_load_pending || (_dirty & DirtyTiles) != 0);
  }

 private:
  struct TileSlot {
    lv_obj_t* root = nullptr;
    lv_obj_t* img = nullptr;
    lv_obj_t* placeholder = nullptr;
    uint32_t x_tile = 0;
    uint32_t y_tile = 0;
    bool active = false;
    bool missing_cached = false;
    char loaded_path[96] = {};
    char placeholder_text[48] = {};
  };

  struct MarkerSlot {
    lv_obj_t* root = nullptr;
    lv_obj_t* label = nullptr;
    bool active = false;
    char glyph = '?';
    char text[2] = {'?', '\0'};
  };

  struct TileNeed {
    uint32_t x_tile = 0;
    uint32_t y_tile = 0;
    int16_t x = 0;
    int16_t y = 0;
    int8_t slot = -1;
  };

  struct CompactMarker {
    int32_t lat_micro = 0;
    int32_t lon_micro = 0;
    int16_t contact_index = -1;
    char glyph = '?';
    bool is_self = false;
  };

  void center_view();
  /** Full tile cache invalidate (zoom / style / SD mount). */
  void request_redraw();
  /** Re-layout visible tiles; keep loaded PNG paths when still valid. */
  void request_layout();
  void prepare_tile_layout();
  void layout_tiles();
  void layout_markers();
  void reposition_visible_tiles();
  void draw_location_pins();
  void draw_range_rings();
  bool createTileSlot(int idx);
  bool ensureMarkerSlot(int idx);
  void markDirty(uint8_t dirty) { _dirty = static_cast<uint8_t>(_dirty | dirty); }
  void resetTransientPan();

  _lv_obj_t* _viewport = nullptr;
  _lv_obj_t* _tile_layer = nullptr;
  _lv_obj_t* _range_layer = nullptr;
  _lv_obj_t* _marker_layer = nullptr;
  _lv_obj_t* _gps_pin = nullptr;

  static constexpr int kRangeRingCount = 4;
  _lv_obj_t* _range_rings[kRangeRingCount]{};
  _lv_obj_t* _range_labels[kRangeRingCount]{};

  MapUiPrefs _prefs{};
  GeoPoint _home{};
  GeoPoint _gps{};
  GeoPoint _scrolled{};
  bool _prefs_loaded = false;
  bool _prefs_dirty = false;
  bool _gps_valid = false;
  bool _sd_tiles = false;
  int _visible_marker_count = 0;
  uint8_t _dirty = DirtyTiles | DirtyMarkers | DirtyRings | DirtyGpsPin | DirtyViewport;
  bool _tile_load_pending = true;
  bool _tile_layout_prepared = false;
  bool _tile_build_failed = false;
  bool _marker_build_failed = false;
  bool _pan_dragging = false;
  int16_t _transient_pan_x = 0;
  int16_t _transient_pan_y = 0;
  int16_t _width = 0;
  int16_t _height = 0;
  int16_t _x_offset = 0;
  int16_t _y_offset = 0;
  uint32_t _x_start = 0;
  uint32_t _y_start = 0;
  uint8_t _tiles_x = 0;
  uint8_t _tiles_y = 0;
  int _tiles_built = 0;
  int _markers_built = 0;
  uint8_t _tile_need_count = 0;
  uint8_t _tile_load_cursor = 0;

  TileSlot _tiles[kMaxTiles]{};
  TileNeed _tile_needs[kMaxTiles]{};
  MarkerSlot _markers[kMaxMarkers]{};
  CompactMarker _marker_data[kMaxMarkers]{};
  int _marker_count = 0;
};

}  // namespace heltec::meshcore::ui::map
