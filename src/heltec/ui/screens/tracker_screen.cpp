#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "tracker_screen.hpp"

#include "heltec/ui/map/map_sd.hpp"
#include "heltec/ui/map/map_fixed_test.hpp"
#include "heltec/ui/map/geo_point.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include "ui/app/ui_theme.hpp"

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#if defined(ESP_PLATFORM)
#include <esp_task_wdt.h>
#endif

#ifndef MAP_UI_PAN_START_PX
#define MAP_UI_PAN_START_PX 4
#endif

#ifndef MAP_UI_TILE_LOAD_AFTER_PAN_MS
#define MAP_UI_TILE_LOAD_AFTER_PAN_MS 350
#endif

#ifndef MAP_UI_TILE_LOAD_INTERVAL_MS
#define MAP_UI_TILE_LOAD_INTERVAL_MS 40
#endif

namespace {
enum MapWork : uint8_t {
  kMapWorkNone = 0,
  kMapWorkStartup = 1 << 0,
  kMapWorkRefresh = 1 << 1,
  kMapWorkTiles = 1 << 2,
};

constexpr uint8_t kMapWorkNonTile =
    static_cast<uint8_t>(kMapWorkStartup | kMapWorkRefresh);

constexpr uint8_t kStartupNone = 0;
constexpr uint8_t kStartupSkipFrame = 1;
constexpr uint8_t kStartupBuildTiles = 2;
constexpr uint8_t kStartupLoadPrefs = 3;
constexpr uint8_t kStartupProbeSd = 4;
constexpr uint8_t kStartupApplySd = 5;
constexpr int kTilesPerPoll = 3;
constexpr int kMarkersPerPoll = 4;
constexpr uint32_t kPoolPrewarmIntervalMs = 10;

constexpr lv_coord_t kMapToolBtnW = 38;
constexpr lv_coord_t kMapToolBtnH = 30;
constexpr lv_coord_t kMapToolbarPad = 3;
constexpr lv_coord_t kMapToolbarGap = 3;

lv_coord_t map_toolbar_width(int visible_buttons) {
  if (visible_buttons < 1) visible_buttons = 1;
  return static_cast<lv_coord_t>(kMapToolbarPad * 2 + kMapToolBtnW * visible_buttons +
                                 kMapToolbarGap * (visible_buttons - 1));
}

uint32_t remaining_wait_ms(uint32_t now_ms, uint32_t start_ms, uint32_t wait_ms) {
  if (start_ms == 0 || wait_ms == 0) return 0;
  const uint32_t elapsed = now_ms - start_ms;
  return elapsed >= wait_ms ? 0 : wait_ms - elapsed;
}

uint32_t ms_until(uint32_t now_ms, uint32_t due_ms) {
  const int32_t delta = (int32_t)(due_ms - now_ms);
  return delta <= 0 ? 0U : (uint32_t)delta;
}

bool usable_gps(const heltec::meshcore::biz::IBizFacade::GpsStatus& gps) {
  return gps.fix_valid && isfinite(gps.lat_deg) && isfinite(gps.lon_deg) &&
         gps.lat_deg >= -90.0 && gps.lat_deg <= 90.0 && gps.lon_deg >= -180.0 &&
         gps.lon_deg <= 180.0 && (gps.lat_deg != 0.0 || gps.lon_deg != 0.0);
}
}  // namespace

namespace heltec::meshcore::ui {

_lv_obj_t* TrackerScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::TrackerScreenRoot);
}

namespace {

_lv_obj_t* add_tool_btn_label(_lv_obj_t* btn, const char* text) {
  if (!btn) return nullptr;
  _lv_obj_t* lb = ht_label_create(btn, meta_id::MapToolbarButtonLabel, text);
  if (!lb) return nullptr;
  lv_obj_align(lb, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(lb, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(lb, LV_OBJ_FLAG_EVENT_BUBBLE);
  return lb;
}

}  // namespace

bool TrackerScreen::hitMapViewport(lv_coord_t x, lv_coord_t y) const {
  if (!_map_viewport) return false;
  lv_area_t a;
  lv_obj_get_coords(_map_viewport, &a);
  return x >= a.x1 && x <= a.x2 && y >= a.y1 && y <= a.y2;
}

bool TrackerScreen::hitMapToolbar(lv_coord_t x, lv_coord_t y) const {
  if (!_toolbar) return false;
  const _lv_obj_t* buttons[] = {_btn_zoom_out, _btn_zoom_in, _btn_gps};
  for (const _lv_obj_t* button : buttons) {
    if (!button || lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN)) continue;
    lv_area_t a;
    lv_obj_get_coords(button, &a);
    if (x >= a.x1 && x <= a.x2 && y >= a.y1 && y <= a.y2) return true;
  }
  return false;
}

void TrackerScreen::on_map_gesture(lv_event_t* e) {
  auto* self = static_cast<TrackerScreen*>(lv_event_get_user_data(e));
  if (!self) return;
  const lv_event_code_t code = lv_event_get_code(e);
  if (LV_EVENT_PRESSED != code && LV_EVENT_PRESSING != code && LV_EVENT_RELEASED != code &&
      LV_EVENT_PRESS_LOST != code) {
    return;
  }
  if (self->_map_unavailable || !self->_panel_attached || self->_startup_phase != kStartupNone ||
      !self->_panel.tilesReady() || self->_panel.tileBuildFailed()) {
    return;
  }
  lv_indev_t* indev = lv_event_get_indev(e);
  if (!indev) indev = lv_indev_get_act();
  if (!indev) return;
  lv_point_t p;
  lv_indev_get_point(indev, &p);
  const bool on_toolbar = self->hitMapToolbar(p.x, p.y);
  if (on_toolbar && code == LV_EVENT_PRESSED) return;

  if (code == LV_EVENT_PRESSED) {
    self->_pan_origin = p;
    self->_pan_applied_x = 0;
    self->_pan_applied_y = 0;
    self->_pan_active = true;
    self->_pan_moved = false;
  } else if (code == LV_EVENT_PRESSING && self->_pan_active) {
    const int32_t total_x = (int32_t)p.x - (int32_t)self->_pan_origin.x;
    const int32_t total_y = (int32_t)p.y - (int32_t)self->_pan_origin.y;
    if (!self->_pan_moved) {
      const int32_t abs_x = total_x < 0 ? -total_x : total_x;
      const int32_t abs_y = total_y < 0 ? -total_y : total_y;
      if (abs_x < MAP_UI_PAN_START_PX && abs_y < MAP_UI_PAN_START_PX) return;
      self->_pan_moved = true;
      self->_user_panned = true;
      self->_auto_center_on_first_fix = false;
      self->_panel.setPanDragging(true);
    }
    const int32_t dx = total_x - self->_pan_applied_x;
    const int32_t dy = total_y - self->_pan_applied_y;
    if (dx != 0 || dy != 0) {
      self->_last_pan_ms = millis();
      self->_pan_applied_x = total_x;
      self->_pan_applied_y = total_y;
      self->_panel.pan_pixels((int16_t)dx, (int16_t)dy);
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    const bool completed_pan = self->_pan_active && self->_pan_moved;
    if (completed_pan) {
      const int32_t total_x = (int32_t)p.x - (int32_t)self->_pan_origin.x;
      const int32_t total_y = (int32_t)p.y - (int32_t)self->_pan_origin.y;
      const int32_t dx = total_x - self->_pan_applied_x;
      const int32_t dy = total_y - self->_pan_applied_y;
      if (dx != 0 || dy != 0) {
        self->_panel.pan_pixels((int16_t)dx, (int16_t)dy);
      }
    }
    if (completed_pan) {
      self->_last_pan_ms = millis();
      self->_panel.finish_pan();
      self->_panel.setPanDragging(false);
      self->_panel.refreshOverlays();
      self->_panel.save_prefs();
      self->syncMapToolbarVisibility();
      self->update_status_line();
    } else if (self->_pan_active) {
      self->_panel.setPanDragging(false);
      self->_panel.refreshOverlays();
    }
    self->_pan_active = false;
    self->_pan_moved = false;
    if (completed_pan) {
      // End the gesture first, then schedule overlays immediately when they
      // were queued during the drag; tile work keeps the post-pan delay.
      self->_map_work = static_cast<uint8_t>(self->_map_work | kMapWorkTiles);
      const uint32_t delay_ms = self->nextMapWorkDelayMs();
      self->scheduleMapWork(delay_ms);
    } else if (self->hasMapWork()) {
      // A short press can pause the work timer without ever becoming a pan.
      // Resume any GPS/contact refresh queued while the pointer was down.
      self->scheduleMapWork();
    }
  }
}

void TrackerScreen::update_status_line() {
  if (!_lbl_status || !_panel_attached) return;
  char sd_buf[20];
  const char* fs = heltec::meshcore::ui::map::map_sd_fs_label();
  const char* sd_tag = "noSD";
  if (_panel.sd_tiles()) {
    if (fs) {
      lv_snprintf(sd_buf, sizeof(sd_buf), "SD %s", fs);
      sd_tag = sd_buf;
    } else {
      sd_tag = "SD";
    }
  }
  const int32_t lat_centi = (int32_t)(_panel.center_lat() * 100.0f +
                                      (_panel.center_lat() >= 0.f ? 0.5f : -0.5f));
  const int32_t lon_centi = (int32_t)(_panel.center_lon() * 100.0f +
                                      (_panel.center_lon() >= 0.f ? 0.5f : -0.5f));
  const int32_t lat_abs = lat_centi < 0 ? -lat_centi : lat_centi;
  const int32_t lon_abs = lon_centi < 0 ? -lon_centi : lon_centi;
  lv_snprintf(_status_text, sizeof(_status_text),
              "z%u %s | %s%ld.%02ld,%s%ld.%02ld gps:%d vis:%d/%d",
              (unsigned)_panel.zoom(), sd_tag, lat_centi < 0 ? "-" : "",
              (long)(lat_abs / 100), (long)(lat_abs % 100), lon_centi < 0 ? "-" : "",
              (long)(lon_abs / 100), (long)(lon_abs % 100), _contact_gps_count,
              _panel.visibleMarkerCount(), _contact_gps_count);
  lv_label_set_text_static(_lbl_status, _status_text);
}

void TrackerScreen::update_status_line(const biz::MapPlotUi& plot) {
  _contact_gps_count = plot.contact_gps_count;
  update_status_line();
}

void TrackerScreen::bindPanGestures(_lv_obj_t* obj) {
  if (!obj) return;
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_PRESSED, this);
  lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_PRESSING, this);
  lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_RELEASED, this);
  lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_PRESS_LOST, this);
}

void TrackerScreen::ensure_panel_attached() {
  if (_panel_attached) return;
  if (!_map_viewport) return;

  _panel.attach(_map_viewport);
  _panel_attached = _panel.attached();
  if (_panel_attached && _toolbar) {
    raiseMapToolbar();
  }
#if defined(HELTEC_MAP_TOUCH_PAN) && HELTEC_MAP_TOUCH_PAN
  if (_panel_attached) {
    bindPanGestures(_panel.touchLayer());
  }
#else
  if (_panel_attached && _panel.touchLayer()) {
    lv_obj_clear_flag(_panel.touchLayer(), LV_OBJ_FLAG_CLICKABLE);
  }
#endif
}

void TrackerScreen::refresh_from_biz() {
  if (!_panel_attached) ensure_panel_attached();
  if (!_panel_attached) return;
  _panel.refreshSdTiles();
  const biz::MapPlotUi& plot = _biz.mapPlotUi();
  biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
  if (map::mapFixedTestEnabled()) {
    map::mapFixedTestOverrideGps(gps);
  }
  const bool gps_usable = usable_gps(gps);
  _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, gps_usable);
  if (gps_usable && _auto_center_on_first_fix && !_user_panned) {
    _panel.centerOnLocation((float)gps.lat_deg, (float)gps.lon_deg);
    _auto_center_on_first_fix = false;
  }
  _panel.set_markers(plot.markers, plot.marker_count);
  _panel.refreshOverlays();
  if (_panel.tilesLoadPending()) {
    requestMapWork(kMapWorkTiles, nextMapWorkDelayMs());
  }
  syncMapToolbarVisibility();
  update_status_line(plot);
}

void TrackerScreen::refresh_markers_only() {
  if (!_panel_attached) return;
  biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
  if (map::mapFixedTestEnabled()) {
    map::mapFixedTestOverrideGps(gps);
  }
  const bool gps_usable = usable_gps(gps);
  const biz::MapPlotUi& plot = _biz.mapPlotUi();
  _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, gps_usable);
  if (gps_usable && _auto_center_on_first_fix && !_user_panned) {
    _panel.centerOnLocation((float)gps.lat_deg, (float)gps.lon_deg);
    _auto_center_on_first_fix = false;
  }
  _panel.set_markers(plot.markers, plot.marker_count);
  _panel.refreshOverlays();
  if (_panel.tilesLoadPending()) {
    requestMapWork(kMapWorkTiles, nextMapWorkDelayMs());
  }
  syncMapToolbarVisibility();
  update_status_line(plot);
}

void TrackerScreen::center_map_on_current_gps() {
  if (!_panel_attached) return;
  biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
  if (map::mapFixedTestEnabled()) {
    map::mapFixedTestOverrideGps(gps);
  }
  if (!usable_gps(gps)) {
    syncMapToolbarVisibility();
    update_status_line();
    return;
  }
  _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, true);
  _panel.centerOnLocation((float)gps.lat_deg, (float)gps.lon_deg);
  _user_panned = false;
  _auto_center_on_first_fix = false;
  refresh_from_biz();
  raiseMapToolbar();
}

void TrackerScreen::syncMapToolbarVisibility() {
  if (!_btn_gps) return;
  biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
  if (map::mapFixedTestEnabled()) {
    map::mapFixedTestOverrideGps(gps);
  }
  const bool gps_usable = usable_gps(gps);
  const int8_t gps_state = gps_usable ? 1 : 0;
  if (_toolbar_gps_usable == gps_state) return;
  _toolbar_gps_usable = gps_state;
  if (gps_usable) {
    lv_obj_clear_flag(_btn_gps, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(_btn_gps, LV_OBJ_FLAG_HIDDEN);
  }
  if (_toolbar) {
    const int visible_buttons = gps_usable ? 3 : 2;
    lv_obj_set_width(_toolbar, map_toolbar_width(visible_buttons));
  }
  raiseMapToolbar();
}

void TrackerScreen::raiseMapToolbar() {
  if (!_toolbar || !_map_viewport) return;
  if (_map_unavailable || !_panel_attached || _panel.tileBuildFailed()) {
    lv_obj_add_flag(_toolbar, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_align(_toolbar, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
  lv_obj_move_foreground(_toolbar);
  lv_obj_clear_flag(_toolbar, LV_OBJ_FLAG_HIDDEN);
}

void TrackerScreen::setMapToolbarBusy(bool busy) {
  _toolbar_busy = busy;
  _lv_obj_t* btns[] = {_btn_zoom_out, _btn_zoom_in, _btn_gps};
  for (_lv_obj_t* btn : btns) {
    if (!btn) continue;
    if (busy) {
      lv_obj_add_state(btn, LV_STATE_DISABLED);
    } else {
      lv_obj_clear_state(btn, LV_STATE_DISABLED);
    }
    if (_lv_obj_t* lb = lv_obj_get_child(btn, 0)) {
      if (busy) {
        lv_obj_add_state(lb, LV_STATE_DISABLED);
      } else {
        lv_obj_clear_state(lb, LV_STATE_DISABLED);
      }
    }
  }
}

void TrackerScreen::syncMapToolbarBusy() {
  if (!_toolbar_busy || !_panel_attached) return;
  if (!_panel.tilesLoadPending()) {
    setMapToolbarBusy(false);
  }
}

void TrackerScreen::setMapUnavailable() {
  _map_unavailable = true;
  _pool_prewarm_pending = false;
  _map_work = kMapWorkNone;
  _startup_phase = kStartupNone;
  if (_map_work_timer) lv_timer_pause(_map_work_timer);
  _map_work_timer_active = false;
  setMapToolbarBusy(false);
  if (_toolbar) lv_obj_add_flag(_toolbar, LV_OBJ_FLAG_HIDDEN);
  if (_lbl_status) {
    lv_snprintf(_status_text, sizeof(_status_text), "Map unavailable");
    lv_label_set_text_static(_lbl_status, _status_text);
  }
}

void TrackerScreen::runMapToolAction(void (*action)(TrackerScreen& self)) {
  if (_map_unavailable || _toolbar_busy || !_panel_attached || !_panel.tilesReady() ||
      _panel.tileBuildFailed() || !action) {
    return;
  }
  setMapToolbarBusy(true);
  action(*this);
  _last_tile_work_ms = millis();
  raiseMapToolbar();
#if defined(ESP_PLATFORM)
  esp_task_wdt_reset();
#endif
  syncMapToolbarBusy();
  if (_panel.tilesLoadPending()) requestMapWork(kMapWorkTiles);
}

void TrackerScreen::onEnter() {
  AbstractScreen::onEnter();
  _biz.setMapForegroundActive(true);
  using namespace heltec::meshcore::ui::map;
  map_sd_on_screen_enter();
  if (!_map_work_timer || _panel.poolBuildFailed()) {
    setMapUnavailable();
    return;
  }
  _map_unavailable = false;
  _user_panned = false;
  _auto_center_on_first_fix = false;
  _pan_active = false;
  _pan_moved = false;
  _pan_applied_x = 0;
  _pan_applied_y = 0;
  _last_pan_ms = 0;
  _last_tile_work_ms = 0;
  ensure_panel_attached();
  setMapToolbarBusy(true);
  raiseMapToolbar();
  if (_panel_attached) {
    _panel.refreshSdTiles();
  }
  _map_work = kMapWorkStartup;
  if (_panel_attached && _panel.tilesReady()) {
    _startup_phase = kStartupLoadPrefs;
  } else {
    _startup_phase = _panel_attached ? kStartupSkipFrame : kStartupNone;
  }
  if (_startup_phase == kStartupNone) _map_work = kMapWorkRefresh;
  scheduleMapWork();
}

void TrackerScreen::onExit() {
  _biz.setMapForegroundActive(false);
  if (_panel_attached) {
    if (_pan_active || _panel.panDragging()) {
      _panel.finish_pan();
      _panel.setPanDragging(false);
    }
    _panel.save_prefs();
  }
  _pan_active = false;
  _pan_moved = false;
  _pan_applied_x = 0;
  _pan_applied_y = 0;
  _auto_center_on_first_fix = false;
  setMapToolbarBusy(false);
  if (_toolbar) lv_obj_add_flag(_toolbar, LV_OBJ_FLAG_HIDDEN);
  if (_map_work_timer && !_pool_prewarm_pending) lv_timer_pause(_map_work_timer);
  if (!_pool_prewarm_pending) _map_work_timer_active = false;
  _map_work = kMapWorkNone;
  _startup_phase = kStartupNone;
  AbstractScreen::onExit();
  if (_pool_prewarm_pending && !_map_work_timer_active) {
    scheduleMapWork(kPoolPrewarmIntervalMs);
  }
}

void TrackerScreen::mapWorkTimerCb(lv_timer_t* timer) {
  auto* self = timer ? static_cast<TrackerScreen*>(timer->user_data) : nullptr;
  if (!self) return;
  if (timer) lv_timer_pause(timer);
  self->_map_work_timer_active = false;
  if (!self->_root) return;
  if (self->_pool_prewarm_pending) {
    (void)self->processPoolPrewarm();
    if (self->_pool_prewarm_pending) {
      self->scheduleMapWork(kPoolPrewarmIntervalMs);
      return;
    }
  }
  if (lv_obj_has_flag(self->_root, LV_OBJ_FLAG_HIDDEN)) return;
  self->processMapWork();
  if (!self->_pan_active && !self->_panel.panDragging() && self->hasMapWork()) {
    self->scheduleMapWork(self->nextMapWorkDelayMs());
  }
}

void TrackerScreen::requestMapWork(uint8_t work, uint32_t delay_ms) {
  if (_map_unavailable) return;
  _map_work = static_cast<uint8_t>(_map_work | work);
  if (_pan_active || (_panel_attached && _panel.panDragging())) return;
  scheduleMapWork(delay_ms);
}

void TrackerScreen::scheduleMapWork(uint32_t delay_ms) {
  if (_map_unavailable) return;
  if (!_root || (lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN) && !_pool_prewarm_pending)) return;
  if (!hasMapWork()) return;

  if (delay_ms == 0) delay_ms = 1;
  const uint32_t now_ms = millis();
  if (_map_work_timer_active && ms_until(now_ms, _map_work_due_ms) <= delay_ms) return;

  if (!_map_work_timer) {
    setMapUnavailable();
    return;
  }
  lv_timer_set_period(_map_work_timer, delay_ms);
  _map_work_due_ms = now_ms + delay_ms;
  _map_work_timer_active = true;
  lv_timer_reset(_map_work_timer);
  lv_timer_resume(_map_work_timer);
}

bool TrackerScreen::processPoolPrewarm() {
  if (!_pool_prewarm_pending) return true;
  if (!_panel_attached) ensure_panel_attached();
  if (!_panel_attached) {
    setMapUnavailable();
    return true;
  }

  const bool ready = _panel.prewarmPools(kTilesPerPoll, kMarkersPerPoll);
  if (ready) {
    _pool_prewarm_pending = false;
  } else if (_panel.poolBuildFailed()) {
    setMapUnavailable();
  }
  return !_pool_prewarm_pending;
}

bool TrackerScreen::processStartupWork() {
  switch (_startup_phase) {
    case kStartupNone:
      _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkStartup);
      return false;

    case kStartupSkipFrame:
      _startup_phase = kStartupBuildTiles;
      return true;

    case kStartupBuildTiles:
      if (!_panel_attached) ensure_panel_attached();
      if (!_panel_attached) {
        setMapUnavailable();
        return true;
      }
      if (_panel.buildPendingTiles(kTilesPerPoll)) {
        _startup_phase = kStartupLoadPrefs;
      } else if (_panel.tileBuildFailed()) {
        setMapUnavailable();
      }
      return true;

    case kStartupLoadPrefs: {
#if defined(ESP_PLATFORM)
      esp_task_wdt_reset();
#endif
      if (!_panel_attached) ensure_panel_attached();
      if (!_panel_attached) {
        setMapUnavailable();
        return true;
      }
      if (!_panel.tilesReady()) {
        _startup_phase = kStartupBuildTiles;
        return true;
      }
      if (_root) lv_obj_update_layout(_root);
      biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
      if (map::mapFixedTestEnabled()) {
        map::mapFixedTestOverrideGps(gps);
      }
      const bool gps_usable = usable_gps(gps);
      float gps_home[2] = {(float)gps.lat_deg, (float)gps.lon_deg};
      _panel.load_prefs(gps_usable ? gps_home : nullptr);
      _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, gps_usable);
      _auto_center_on_first_fix = !gps_usable;
      if (!gps_usable && !_panel.hasSavedCenter()) {
        const biz::MapPlotUi& plot = _biz.mapPlotUi();
        if (plot.drawable && plot.marker_count > 0) {
          _panel.centerOnLocation((float)plot.center_lat, (float)plot.center_lon, false);
        }
      }
      _panel.syncViewportLayout();
      _startup_phase = map::map_sd_ready() ? kStartupApplySd : kStartupProbeSd;
      return true;
    }

    case kStartupProbeSd: {
#if defined(ESP_PLATFORM)
      esp_task_wdt_reset();
#endif
      const bool probe_done = map::map_sd_probe_once();
      if (!probe_done) return true;
      _startup_phase = kStartupApplySd;
      return true;
    }

    case kStartupApplySd: {
      if (!_panel_attached) ensure_panel_attached();
      if (!_panel_attached) {
        setMapUnavailable();
        return true;
      }
      if (!_panel.tilesReady()) {
        _startup_phase = kStartupBuildTiles;
        return true;
      }
      _panel.refreshSdTiles();
      _panel.applySdPrefs();
      _panel.syncViewportLayout();
      _map_unavailable = false;
      setMapToolbarBusy(false);
      syncMapToolbarVisibility();
      _startup_phase = kStartupNone;
      _map_work = static_cast<uint8_t>((_map_work & ~kMapWorkStartup) | kMapWorkRefresh);
      return true;
    }

    default:
      _startup_phase = kStartupNone;
      _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkStartup);
      return true;
  }
}

void TrackerScreen::processMapWork() {
  if (_pan_active || (_panel_attached && _panel.panDragging())) return;

  if (_map_work & kMapWorkStartup) {
    (void)processStartupWork();
    return;
  }

  if (!_panel_attached) ensure_panel_attached();
  if (!_panel_attached) {
    setMapUnavailable();
    return;
  }

  if (!_panel.tilesReady()) {
    if (_panel.tileBuildFailed()) {
      setMapUnavailable();
      return;
    }
    setMapToolbarBusy(true);
    _startup_phase = kStartupBuildTiles;
    _map_work = static_cast<uint8_t>(_map_work | kMapWorkStartup);
    return;
  }

  if (_map_work & kMapWorkRefresh) {
    _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkRefresh);
    refresh();
    if (_panel.tilesLoadPending()) {
      _last_tile_work_ms = millis();
      _map_work = static_cast<uint8_t>(_map_work | kMapWorkTiles);
    }
    return;
  }

  _panel.refreshSdTiles();
  if ((_map_work & kMapWorkTiles) || _panel.tilesLoadPending()) {
    const uint32_t now_ms = millis();
    const uint32_t pan_wait_ms =
        remaining_wait_ms(now_ms, _last_pan_ms, MAP_UI_TILE_LOAD_AFTER_PAN_MS);
    const uint32_t load_wait_ms =
        remaining_wait_ms(now_ms, _last_tile_work_ms, MAP_UI_TILE_LOAD_INTERVAL_MS);
    const uint32_t wait_ms = pan_wait_ms > load_wait_ms ? pan_wait_ms : load_wait_ms;
    if (wait_ms > 0) return;
    _panel.commit();
    _last_tile_work_ms = millis();
    if (!_panel.tilesLoadPending()) {
      _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkTiles);
      update_status_line();
      syncMapToolbarBusy();
      raiseMapToolbar();
    }
  }
}

void TrackerScreen::refresh() {
  if (!_panel_attached) ensure_panel_attached();
  if (!_panel_attached) return;
  refresh_markers_only();
}

bool TrackerScreen::hasMapWork() const {
  return _pool_prewarm_pending || _map_work != kMapWorkNone ||
         (_panel_attached && _panel.tilesLoadPending());
}

uint32_t TrackerScreen::nextMapWorkDelayMs() const {
  if (_pool_prewarm_pending) return kPoolPrewarmIntervalMs;
  if ((_map_work & kMapWorkNonTile) != 0) return 0;
  if ((_map_work & kMapWorkTiles) || (_panel_attached && _panel.tilesLoadPending())) {
    const uint32_t now_ms = millis();
    const uint32_t pan_wait_ms =
        remaining_wait_ms(now_ms, _last_pan_ms, MAP_UI_TILE_LOAD_AFTER_PAN_MS);
    const uint32_t load_wait_ms =
        remaining_wait_ms(now_ms, _last_tile_work_ms, MAP_UI_TILE_LOAD_INTERVAL_MS);
    return pan_wait_ms > load_wait_ms ? pan_wait_ms : load_wait_ms;
  }
  return 0;
}

void TrackerScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::GpsChanged ||
      event.type == AppStateEventType::ContactLocationChanged) {
    requestMapWork(kMapWorkRefresh);
  }
}

void TrackerScreen::onRefreshRequested() {
  requestMapWork(kMapWorkRefresh);
}

_lv_obj_t* TrackerScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_top(_root, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, 2, LV_PART_MAIN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

  _lbl_status = ht_label_create(_root, meta_id::MapStatusLabel, "Map");
  if (_lbl_status) {
    lv_label_set_text_static(_lbl_status, _status_text);
    lv_obj_set_width(_lbl_status, lv_pct(100));
    lv_label_set_long_mode(_lbl_status, LV_LABEL_LONG_CLIP);
  }

  _map_viewport = ht_obj_create(_root, meta_id::MapViewport);
  if (_map_viewport) {
    lv_obj_set_width(_map_viewport, lv_pct(100));
    lv_obj_set_flex_grow(_map_viewport, 1);
    lv_obj_set_style_min_height(_map_viewport, 140, LV_PART_MAIN);
    lv_obj_set_style_pad_all(_map_viewport, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_map_viewport, LV_OBJ_FLAG_SCROLLABLE);
    _toolbar = ht_obj_create(_map_viewport, meta_id::MapToolbar);
    if (_toolbar) {
      lv_obj_set_size(_toolbar, map_toolbar_width(3),
                      kMapToolbarPad * 2 + kMapToolBtnH);
      lv_obj_align(_toolbar, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
      lv_obj_set_flex_flow(_toolbar, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(_toolbar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);
      lv_obj_set_style_pad_all(_toolbar, kMapToolbarPad, LV_PART_MAIN);
      lv_obj_set_style_pad_column(_toolbar, kMapToolbarGap, LV_PART_MAIN);
      lv_obj_clear_flag(_toolbar, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(_toolbar, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
      lv_obj_move_foreground(_toolbar);
      lv_obj_add_event_cb(
          _map_viewport,
          [](lv_event_t* e) {
            if (LV_EVENT_SIZE_CHANGED != lv_event_get_code(e)) return;
            auto* self = static_cast<TrackerScreen*>(lv_event_get_user_data(e));
            if (self) self->raiseMapToolbar();
          },
          LV_EVENT_SIZE_CHANGED, this);

      _btn_zoom_out = ht_btn_create(_toolbar, meta_id::MapToolbarButton);
      _btn_zoom_in = ht_btn_create(_toolbar, meta_id::MapToolbarButton);
      _btn_gps = ht_btn_create(_toolbar, meta_id::MapToolbarButton);
      _lv_obj_t* const buttons[] = {
          _btn_zoom_out, _btn_zoom_in, _btn_gps,
      };
      for (_lv_obj_t* button : buttons) {
        if (!button) continue;
        lv_obj_set_size(button, kMapToolBtnW, kMapToolBtnH);
        lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
        lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
      }
      if (_btn_zoom_out) {
        (void)add_tool_btn_label(_btn_zoom_out, "-");
        lv_obj_add_event_cb(
            _btn_zoom_out,
            +[](lv_event_t* e) {
              auto* self = static_cast<TrackerScreen*>(lv_event_get_user_data(e));
              if (!self) return;
              self->runMapToolAction(+[](TrackerScreen& scr) {
                scr._panel.zoom_out();
                scr._panel.refreshOverlays();
                scr.update_status_line();
              });
            },
            LV_EVENT_CLICKED, this);
        addFocusItem(_btn_zoom_out);
      }
      if (_btn_zoom_in) {
        (void)add_tool_btn_label(_btn_zoom_in, "+");
        lv_obj_add_event_cb(
            _btn_zoom_in,
            +[](lv_event_t* e) {
              auto* self = static_cast<TrackerScreen*>(lv_event_get_user_data(e));
              if (!self) return;
              self->runMapToolAction(+[](TrackerScreen& scr) {
                scr._panel.zoom_in();
                scr._panel.refreshOverlays();
                scr.update_status_line();
              });
            },
            LV_EVENT_CLICKED, this);
        addFocusItem(_btn_zoom_in);
      }
      if (_btn_gps) {
        (void)add_tool_btn_label(_btn_gps, "G");
        lv_obj_add_event_cb(
            _btn_gps,
            +[](lv_event_t* e) {
              auto* self = static_cast<TrackerScreen*>(lv_event_get_user_data(e));
              if (!self) return;
              self->runMapToolAction(+[](TrackerScreen& scr) { scr.center_map_on_current_gps(); });
            },
            LV_EVENT_CLICKED, this);
        addFocusItem(_btn_gps);
      }
      syncMapToolbarVisibility();
      raiseMapToolbar();
    }
  }

  _map_work_timer = lv_timer_create(mapWorkTimerCb, 1U, this);
  if (_map_work_timer) {
    lv_timer_set_repeat_count(_map_work_timer, -1);
    lv_timer_pause(_map_work_timer);
    _pool_prewarm_pending = true;
    scheduleMapWork(1U);
  } else {
    setMapUnavailable();
  }

  return _root;
}

}  // namespace heltec::meshcore::ui

#endif  // ENV_INCLUDE_MAP
