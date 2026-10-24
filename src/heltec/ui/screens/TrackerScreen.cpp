#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "TrackerScreen.hpp"

#include "heltec/ui/map/GeoPoint.hpp"
#include "heltec/ui/map/MapTileStore.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include "ui/core/UiMessage.hpp"

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <string.h>
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

namespace
{
enum MapWork : uint8_t
{
    kMapWorkNone = 0,
    kMapWorkStartup = 1 << 0,
    kMapWorkRefresh = 1 << 1,
    kMapWorkTiles = 1 << 2,
};

constexpr uint8_t kMapWorkNonTile = static_cast<uint8_t>(kMapWorkStartup | kMapWorkRefresh);

constexpr uint8_t kStartupNone = 0;
constexpr uint8_t kStartupSkipFrame = 1;
constexpr uint8_t kStartupBuildTiles = 2;
constexpr uint8_t kStartupLoadPrefs = 3;
constexpr uint8_t kStartupProbeSd = 4;
constexpr uint8_t kStartupApplySd = 5;
constexpr int kTilesPerPoll = 3;
constexpr int kMarkersPerPoll = 4;
constexpr uint32_t kPoolPrewarmIntervalMs = 10;
constexpr uint32_t kTeamRefreshPeriodMs = 60000UL;

constexpr lv_coord_t kMapToolBtnW = 38;
constexpr lv_coord_t kMapToolBtnH = 38;
constexpr lv_coord_t kMapToolbarPad = 3;
constexpr lv_coord_t kMapToolbarGap = 3;

lv_coord_t map_toolbar_width(int visible_buttons)
{
    if (visible_buttons < 1)
        visible_buttons = 1;
    return static_cast<lv_coord_t>(kMapToolbarPad * 2 + kMapToolBtnW * visible_buttons +
                                   kMapToolbarGap * (visible_buttons - 1));
}

uint32_t remaining_wait_ms(uint32_t now_ms, uint32_t start_ms, uint32_t wait_ms)
{
    if (start_ms == 0 || wait_ms == 0)
        return 0;
    const uint32_t elapsed = now_ms - start_ms;
    return elapsed >= wait_ms ? 0 : wait_ms - elapsed;
}

uint32_t ms_until(uint32_t now_ms, uint32_t due_ms)
{
    const int32_t delta = (int32_t)(due_ms - now_ms);
    return delta <= 0 ? 0U : (uint32_t)delta;
}

bool usable_gps(const Heltec::MeshCore::Biz::IBizFacade::GpsStatus &gps)
{
    return gps.fix_valid && isfinite(gps.lat_deg) && isfinite(gps.lon_deg) &&
           gps.lat_deg >= -90.0 && gps.lat_deg <= 90.0 && gps.lon_deg >= -180.0 &&
           gps.lon_deg <= 180.0 && (gps.lat_deg != 0.0 || gps.lon_deg != 0.0);
}
} // namespace

namespace Heltec::MeshCore::Ui
{

const lv_obj_class_t ht_tracker_screen_root_class = {
    &lv_obj_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_obj_t),
};

TrackerScreen::TrackerScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
    : AbstractScreen(biz, title, icon)
{
}

namespace
{

constexpr const lv_obj_class_t *kTrackerToolbarClass = &ht_tracker_toolbar_class;
constexpr const lv_obj_class_t *kTrackerToolbarButtonClass = &ht_tracker_toolbar_button_class;
constexpr const lv_obj_class_t *kTrackerToolbarLabelClass = &ht_tracker_toolbar_label_class;

_lv_obj_t *add_tool_btn_label(_lv_obj_t *btn, const char *text)
{
    if (!btn)
        return nullptr;
    _lv_obj_t *lb = ht_label_create(kTrackerToolbarLabelClass, btn, text);
    if (!lb)
        return nullptr;
    lv_obj_align(lb, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(lb, LV_OBJ_FLAG_CLICKABLE);
    return lb;
}

} // namespace

bool TrackerScreen::hitMapViewport(lv_coord_t x, lv_coord_t y) const
{
    if (!_map_viewport)
        return false;
    lv_area_t a;
    lv_obj_get_coords(_map_viewport, &a);
    return x >= a.x1 && x <= a.x2 && y >= a.y1 && y <= a.y2;
}

bool TrackerScreen::hitMapToolbar(lv_coord_t x, lv_coord_t y) const
{
    if (!_toolbar)
        return false;
    const _lv_obj_t *buttons[] = {
        _btn_zoom_out, _btn_zoom_in, _btn_gps,
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
        _btn_fence, _btn_fence_delete,
#endif
    };
    for (const _lv_obj_t *button : buttons)
    {
        if (!button || lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN))
            continue;
        lv_area_t a;
        lv_obj_get_coords(button, &a);
        if (x >= a.x1 && x <= a.x2 && y >= a.y1 && y <= a.y2)
            return true;
    }
    return false;
}

void TrackerScreen::on_map_gesture(lv_event_t *e)
{
    auto *self = static_cast<TrackerScreen *>(lv_event_get_user_data(e));
    if (!self)
        return;
    const lv_event_code_t code = lv_event_get_code(e);
    if (LV_EVENT_PRESSED != code && LV_EVENT_PRESSING != code && LV_EVENT_RELEASED != code &&
        LV_EVENT_PRESS_LOST != code)
    {
        return;
    }
    if (self->_map_unavailable || !self->_panel_attached || self->_startup_phase != kStartupNone ||
        !self->_panel.tilesReady() || self->_panel.tileBuildFailed())
    {
        return;
    }
    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev)
        indev = lv_indev_get_act();
    if (!indev)
        return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    const bool on_toolbar = self->hitMapToolbar(p.x, p.y);
    if (on_toolbar && code == LV_EVENT_PRESSED)
        return;

    if (code == LV_EVENT_PRESSED)
    {
        self->_pan_origin = p;
        self->_pan_applied_x = 0;
        self->_pan_applied_y = 0;
        self->_pan_active = true;
        self->_pan_moved = false;
    }
    else if (code == LV_EVENT_PRESSING && self->_pan_active)
    {
        const int32_t total_x = (int32_t)p.x - (int32_t)self->_pan_origin.x;
        const int32_t total_y = (int32_t)p.y - (int32_t)self->_pan_origin.y;
        if (!self->_pan_moved)
        {
            const int32_t abs_x = total_x < 0 ? -total_x : total_x;
            const int32_t abs_y = total_y < 0 ? -total_y : total_y;
            if (abs_x < MAP_UI_PAN_START_PX && abs_y < MAP_UI_PAN_START_PX)
                return;
            self->_pan_moved = true;
            self->_user_panned = true;
            self->_auto_center_on_first_fix = false;
            self->_panel.setPanDragging(true);
        }
        const int32_t dx = total_x - self->_pan_applied_x;
        const int32_t dy = total_y - self->_pan_applied_y;
        if (dx != 0 || dy != 0)
        {
            self->_last_pan_ms = millis();
            self->_pan_applied_x = total_x;
            self->_pan_applied_y = total_y;
            self->_panel.pan_pixels((int16_t)dx, (int16_t)dy);
        }
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        const bool completed_pan = self->_pan_active && self->_pan_moved;
        if (completed_pan)
        {
            const int32_t total_x = (int32_t)p.x - (int32_t)self->_pan_origin.x;
            const int32_t total_y = (int32_t)p.y - (int32_t)self->_pan_origin.y;
            const int32_t dx = total_x - self->_pan_applied_x;
            const int32_t dy = total_y - self->_pan_applied_y;
            if (dx != 0 || dy != 0)
            {
                self->_panel.pan_pixels((int16_t)dx, (int16_t)dy);
            }
        }
        if (completed_pan)
        {
            self->_last_pan_ms = millis();
            self->_panel.finish_pan();
            self->_panel.setPanDragging(false);
            self->_panel.refreshOverlays();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
            self->syncFenceOverlay();
#endif
            self->_panel.save_prefs();
            self->syncMapToolbarVisibility();
            self->update_status_line();
        }
        else if (self->_pan_active && !self->_pan_moved)
        {
            self->_panel.setPanDragging(false);
            self->_panel.refreshOverlays();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
            if (self->_biz.teamGeofenceState() == Heltec::MeshCore::Team::GeofenceState::Editing)
                self->addFencePointAt(p.x, p.y);
#endif
        }
        self->_pan_active = false;
        self->_pan_moved = false;
        if (completed_pan)
        {
            // End the gesture first, then schedule overlays immediately when they
            // were queued during the drag; tile work keeps the post-pan delay.
            self->_map_work = static_cast<uint8_t>(self->_map_work | kMapWorkTiles);
            const uint32_t delay_ms = self->nextMapWorkDelayMs();
            self->scheduleMapWork(delay_ms);
        }
        else if (self->hasMapWork())
        {
            // A short press can pause the work timer without ever becoming a pan.
            // Resume any GPS/contact refresh queued while the pointer was down.
            self->scheduleMapWork();
        }
    }
}

void TrackerScreen::update_status_line()
{
    if (!_lbl_status || !_panel_attached)
        return;
    char sd_buf[20];
    const char *fs = Heltec::MeshCore::Ui::Map::map_tile_store_fs_label();
    const char *sd_tag = "noSD";
    if (_panel.sd_tiles())
    {
        if (fs)
        {
            lv_snprintf(sd_buf, sizeof(sd_buf), "SD %s", fs);
            sd_tag = sd_buf;
        }
        else
        {
            sd_tag = "SD";
        }
    }
    const int32_t lat_centi =
        (int32_t)(_panel.center_lat() * 100.0f + (_panel.center_lat() >= 0.f ? 0.5f : -0.5f));
    const int32_t lon_centi =
        (int32_t)(_panel.center_lon() * 100.0f + (_panel.center_lon() >= 0.f ? 0.5f : -0.5f));
    const int32_t lat_abs = lat_centi < 0 ? -lat_centi : lat_centi;
    const int32_t lon_abs = lon_centi < 0 ? -lon_centi : lon_centi;
    char next_status[sizeof(_status_text)] = {};
    lv_snprintf(next_status, sizeof(next_status),
                "z%u %s | %s%ld.%02ld,%s%ld.%02ld loc:%d vis:%d/%d team:%d/%d",
                (unsigned)_panel.zoom(),
                sd_tag, lat_centi < 0 ? "-" : "", (long)(lat_abs / 100), (long)(lat_abs % 100),
                lon_centi < 0 ? "-" : "", (long)(lon_abs / 100), (long)(lon_abs % 100),
                _team_location_count, _panel.visibleMarkerCount(), _team_location_count,
                _team_online_count,
                _team_member_count_cached >= 0 ? _team_member_count_cached : 0);
    if (strcmp(_status_text, next_status) != 0)
    {
        strncpy(_status_text, next_status, sizeof(_status_text) - 1U);
        _status_text[sizeof(_status_text) - 1U] = '\0';
        lv_label_set_text_static(_lbl_status, _status_text);
    }
}

void TrackerScreen::bindPanGestures(_lv_obj_t *obj)
{
    if (!obj)
        return;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_PRESSING, this);
    lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_RELEASED, this);
    lv_obj_add_event_cb(obj, on_map_gesture, LV_EVENT_PRESS_LOST, this);
}

void TrackerScreen::ensure_panel_attached()
{
    if (_panel_attached)
        return;
    if (!_map_viewport)
        return;

    _panel.attach(_map_viewport);
    _panel_attached = _panel.attached();
    if (_panel_attached && _toolbar)
    {
        raiseMapToolbar();
    }
    if (_panel_attached)
    {
        bindPanGestures(_panel.touchLayer());
        _lv_obj_t *const overlay_layer = _panel.overlayLayer();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
        if (!_fence_overlay && overlay_layer)
        {
            _fence_overlay = lv_obj_create(overlay_layer);
            if (_fence_overlay)
            {
                lv_obj_remove_style_all(_fence_overlay);
                lv_obj_set_size(_fence_overlay, lv_pct(100), lv_pct(100));
                lv_obj_set_pos(_fence_overlay, 0, 0);
                lv_obj_clear_flag(_fence_overlay,
                                  LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_add_event_cb(_fence_overlay, onFenceOverlayDraw, LV_EVENT_DRAW_MAIN, this);
                lv_obj_move_background(_fence_overlay);
            }
        }
        syncFenceOverlay();
#else
        (void)overlay_layer;
#endif
    }
}

void TrackerScreen::refreshMap(bool refresh_sd_tiles)
{
    if (!_panel_attached)
        ensure_panel_attached();
    if (!_panel_attached)
        return;
    if (refresh_sd_tiles)
        _panel.refreshSdTiles();
    const Biz::MapPlotUi &plot = _biz.mapPlotUi();
    Biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
    const bool gps_usable = usable_gps(gps);
    _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, gps_usable);
    if (gps_usable && _auto_center_on_first_fix && !_user_panned)
    {
        _panel.centerOnLocation((float)gps.lat_deg, (float)gps.lon_deg);
        _auto_center_on_first_fix = false;
    }
    _panel.set_markers(plot.markers, plot.marker_count);
    const Biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    _team_member_count_cached = team.has_team ? team.member_count : 0;
    _team_location_count = plot.marker_count;
    _team_online_count = plot.online_count;
    _panel.refreshOverlays();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    syncFenceOverlay();
#endif
    if (_panel.tilesLoadPending())
    {
        requestMapWork(kMapWorkTiles, nextMapWorkDelayMs());
    }
    syncMapToolbarVisibility();
    update_status_line();
}

void TrackerScreen::center_map_on_current_gps()
{
    if (!_panel_attached)
        return;
    Biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
    if (!usable_gps(gps))
    {
        syncMapToolbarVisibility();
        update_status_line();
        return;
    }
    _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, true);
    _panel.centerOnLocation((float)gps.lat_deg, (float)gps.lon_deg);
    _user_panned = false;
    _auto_center_on_first_fix = false;
    refreshMap(true);
    raiseMapToolbar();
}


#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
void TrackerScreen::syncFenceTools()
{
    const auto state = _biz.teamGeofenceState();
    const Biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    const bool can_edit = team.has_team && team.is_leader;
    if (_btn_fence)
    {
        if (can_edit)
            lv_obj_clear_flag(_btn_fence, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(_btn_fence, LV_OBJ_FLAG_HIDDEN);
        if (can_edit && state != Heltec::MeshCore::Team::GeofenceState::Active)
            lv_obj_clear_state(_btn_fence, LV_STATE_DISABLED);
        else
            lv_obj_add_state(_btn_fence, LV_STATE_DISABLED);
        if (state == Heltec::MeshCore::Team::GeofenceState::Editing)
            lv_obj_add_state(_btn_fence, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(_btn_fence, LV_STATE_CHECKED);
        if (_lv_obj_t *label = lv_obj_get_child(_btn_fence, 0))
            lv_label_set_text_static(label,
                                     state == Heltec::MeshCore::Team::GeofenceState::Editing
                                         ? LV_SYMBOL_CLOSE
                                         : "F");
    }
    if (_btn_fence_delete)
    {
        if (can_edit)
            lv_obj_clear_flag(_btn_fence_delete, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(_btn_fence_delete, LV_OBJ_FLAG_HIDDEN);
        if (can_edit && state == Heltec::MeshCore::Team::GeofenceState::Active)
            lv_obj_clear_state(_btn_fence_delete, LV_STATE_DISABLED);
        else
            lv_obj_add_state(_btn_fence_delete, LV_STATE_DISABLED);
    }
    raiseMapToolbar();
}

void TrackerScreen::syncFenceOverlay()
{
    if (!_fence_overlay)
        return;
    memset(_fence_point_visible, 0, sizeof(_fence_point_visible));
    for (uint8_t index = 0; index < _biz.teamGeofencePointCount(); ++index)
    {
        Heltec::MeshCore::Team::GeofencePoint point{};
        if (!_biz.teamGeofencePoint(index, point))
            continue;
        _fence_point_visible[index] = _panel.locationToPoint(
            static_cast<float>(point.latitude_e7) / 10000000.0f,
            static_cast<float>(point.longitude_e7) / 10000000.0f,
            _fence_points[index]);
    }
    lv_obj_invalidate(_fence_overlay);
}

void TrackerScreen::onFenceOverlayDraw(lv_event_t *event)
{
    auto *self = static_cast<TrackerScreen *>(lv_event_get_user_data(event));
    lv_draw_ctx_t *const draw_ctx = lv_event_get_draw_ctx(event);
    if (!self || !draw_ctx || !self->_fence_overlay)
        return;

    const uint8_t count = self->_biz.teamGeofencePointCount();
    if (count == 0)
        return;
    lv_area_t overlay_area;
    lv_obj_get_coords(self->_fence_overlay, &overlay_area);

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = ui_color_accent();
    line_dsc.width = 2;
    line_dsc.round_start = true;
    line_dsc.round_end = true;
    auto absolute_point = [&overlay_area](const lv_point_t &point)
    {
        return lv_point_t{ static_cast<lv_coord_t>(overlay_area.x1 + point.x),
                           static_cast<lv_coord_t>(overlay_area.y1 + point.y) };
    };
    for (uint8_t index = 1; index < count; ++index)
    {
        if (!self->_fence_point_visible[index - 1U] || !self->_fence_point_visible[index])
            continue;
        const lv_point_t start = absolute_point(self->_fence_points[index - 1U]);
        const lv_point_t end = absolute_point(self->_fence_points[index]);
        lv_draw_line(draw_ctx, &line_dsc, &start, &end);
    }
    if (self->_biz.teamGeofenceState() == Heltec::MeshCore::Team::GeofenceState::Active && count > 2U &&
        self->_fence_point_visible[count - 1U] && self->_fence_point_visible[0])
    {
        const lv_point_t start = absolute_point(self->_fence_points[count - 1U]);
        const lv_point_t end = absolute_point(self->_fence_points[0]);
        lv_draw_line(draw_ctx, &line_dsc, &start, &end);
    }

    lv_draw_rect_dsc_t point_dsc;
    lv_draw_rect_dsc_init(&point_dsc);
    point_dsc.radius = LV_RADIUS_CIRCLE;
    point_dsc.bg_opa = LV_OPA_COVER;
    for (uint8_t index = 0; index < count; ++index)
    {
        if (!self->_fence_point_visible[index])
            continue;
        point_dsc.bg_color = index == 0 ? ui_color_success() : ui_color_accent();
        const lv_point_t point = absolute_point(self->_fence_points[index]);
        lv_area_t point_area = { static_cast<lv_coord_t>(point.x - 4),
                                 static_cast<lv_coord_t>(point.y - 4),
                                 static_cast<lv_coord_t>(point.x + 4),
                                 static_cast<lv_coord_t>(point.y + 4) };
        lv_draw_rect(draw_ctx, &point_dsc, &point_area);
    }
}

void TrackerScreen::addFencePointAt(lv_coord_t screen_x, lv_coord_t screen_y)
{
    float latitude = 0.0f;
    float longitude = 0.0f;
    if (!_panel.screenToLocation(screen_x, screen_y, latitude, longitude))
        return;
    const auto result = _biz.addTeamGeofencePoint(static_cast<int32_t>(latitude * 10000000.0f),
                                                  static_cast<int32_t>(longitude * 10000000.0f));
    if (result == Heltec::MeshCore::Team::GeofenceAddResult::Completed)
        showAlert("Fence active", 1200);
    else if (result == Heltec::MeshCore::Team::GeofenceAddResult::Full)
        showAlert("Fence full; tap first point", 1500);
    syncFenceOverlay();
    syncFenceTools();
}

void TrackerScreen::onFenceToolLongPressed(lv_event_t *event)
{
    auto *self = static_cast<TrackerScreen *>(lv_event_get_user_data(event));
    if (!self || lv_event_get_code(event) != LV_EVENT_LONG_PRESSED)
        return;
    const auto *button = lv_event_get_target(event);
    if (button == self->_btn_fence)
    {
        if (self->_biz.teamGeofenceState() == Heltec::MeshCore::Team::GeofenceState::Editing)
            self->_biz.cancelTeamGeofenceEdit();
        else if (self->_biz.beginTeamGeofenceEdit())
            self->showAlert("Fence marking started", 1500);
    }
    else if (button == self->_btn_fence_delete)
    {
        self->_biz.deleteTeamGeofence();
    }
    self->syncFenceOverlay();
    self->syncFenceTools();
}
#endif

void TrackerScreen::syncMapToolbarVisibility()
{
    if (!_btn_gps)
        return;
    Biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
    const bool gps_usable = usable_gps(gps);
    const int8_t gps_state = gps_usable ? 1 : 0;
    if (_toolbar_gps_usable == gps_state)
        return;
    _toolbar_gps_usable = gps_state;
    if (gps_usable)
    {
        lv_obj_clear_flag(_btn_gps, LV_OBJ_FLAG_HIDDEN);
        if (!_toolbar_busy && !_map_unavailable)
            lv_obj_clear_state(_btn_gps, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_clear_flag(_btn_gps, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_state(_btn_gps, LV_STATE_DISABLED);
    }
    raiseMapToolbar();
}

void TrackerScreen::raiseMapToolbar()
{
    if (!_toolbar || !_map_viewport)
        return;
    const _lv_obj_t *const buttons[] = {
        _btn_zoom_out, _btn_zoom_in, _btn_gps,
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
        _btn_fence, _btn_fence_delete,
#endif
    };
    int count = 0;
    for (const _lv_obj_t *button : buttons)
    {
        if (button && !lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN))
            ++count;
    }
    const lv_coord_t available = lv_obj_get_content_width(_map_viewport) - 8;
    const lv_coord_t desired = map_toolbar_width(count);
    if (available > 0)
        lv_obj_set_width(_toolbar, desired < available ? desired : available);
    lv_obj_align(_toolbar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_move_foreground(_toolbar);
    lv_obj_clear_flag(_toolbar, LV_OBJ_FLAG_HIDDEN);
}

void TrackerScreen::setMapToolbarBusy(bool busy)
{
    _toolbar_busy = busy;
    _lv_obj_t *btns[] = { _btn_zoom_out, _btn_zoom_in, _btn_gps };
    for (_lv_obj_t *btn : btns)
    {
        if (!btn)
            continue;
        if (busy || _map_unavailable || (btn == _btn_gps && _toolbar_gps_usable == 0))
        {
            lv_obj_add_state(btn, LV_STATE_DISABLED);
        }
        else
        {
            lv_obj_clear_state(btn, LV_STATE_DISABLED);
        }
        if (_lv_obj_t *lb = lv_obj_get_child(btn, 0))
        {
            if (busy || _map_unavailable || (btn == _btn_gps && _toolbar_gps_usable == 0))
            {
                lv_obj_add_state(lb, LV_STATE_DISABLED);
            }
            else
            {
                lv_obj_clear_state(lb, LV_STATE_DISABLED);
            }
        }
    }
}

void TrackerScreen::syncMapToolbarBusy()
{
    if (!_toolbar_busy || !_panel_attached)
        return;
    if (!_panel.tilesLoadPending())
    {
        setMapToolbarBusy(false);
    }
}

void TrackerScreen::setMapUnavailable()
{
    _map_unavailable = true;
    _pool_prewarm_pending = false;
    _map_work = kMapWorkNone;
    _startup_phase = kStartupNone;
    if (_map_work_timer)
        lv_timer_pause(_map_work_timer);
    _map_work_timer_active = false;
    setMapToolbarBusy(false);
    raiseMapToolbar();
    if (_lbl_status)
    {
        lv_snprintf(_status_text, sizeof(_status_text), "Map unavailable");
        lv_label_set_text_static(_lbl_status, _status_text);
    }
}

void TrackerScreen::runMapToolAction(void (*action)(TrackerScreen &self))
{
    if (_map_unavailable || _toolbar_busy || !_panel_attached || !_panel.tilesReady() ||
        _panel.tileBuildFailed() || !action)
    {
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
    if (_panel.tilesLoadPending())
        requestMapWork(kMapWorkTiles);
}

void TrackerScreen::onMapViewportSizeChanged(lv_event_t *event)
{
    auto *const self = static_cast<TrackerScreen *>(lv_event_get_user_data(event));
    if (self)
        self->raiseMapToolbar();
}

void TrackerScreen::onZoomOutClicked(lv_event_t *event)
{
    auto *const self = static_cast<TrackerScreen *>(lv_event_get_user_data(event));
    if (self)
        self->runMapToolAction(zoomOutMap);
}

void TrackerScreen::onZoomInClicked(lv_event_t *event)
{
    auto *const self = static_cast<TrackerScreen *>(lv_event_get_user_data(event));
    if (self)
        self->runMapToolAction(zoomInMap);
}

void TrackerScreen::onGpsClicked(lv_event_t *event)
{
    auto *const self = static_cast<TrackerScreen *>(lv_event_get_user_data(event));
    if (self)
        self->runMapToolAction(centerMapOnGps);
}

void TrackerScreen::zoomOutMap(TrackerScreen &screen)
{
    screen._panel.zoom_out();
    screen.refreshMap(false);
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    screen.syncFenceOverlay();
#endif
    screen.update_status_line();
}

void TrackerScreen::zoomInMap(TrackerScreen &screen)
{
    screen._panel.zoom_in();
    screen.refreshMap(false);
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    screen.syncFenceOverlay();
#endif
    screen.update_status_line();
}

void TrackerScreen::centerMapOnGps(TrackerScreen &screen)
{
    screen.center_map_on_current_gps();
}

void TrackerScreen::onEnter()
{
    AbstractScreen::onEnter();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    syncFenceTools();
#endif
    _biz.setMapForegroundActive(true);
    using namespace Heltec::MeshCore::Ui::Map;
    map_tile_store_on_screen_enter();
    if (!_map_work_timer || _panel.poolBuildFailed())
    {
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
    _team_refresh_enabled = _biz.teamStatus().has_team;
    _next_team_refresh_ms = _team_refresh_enabled ? millis() + kTeamRefreshPeriodMs : 0;
    ensure_panel_attached();
    setMapToolbarBusy(true);
    raiseMapToolbar();
    if (_panel_attached)
    {
        _panel.refreshSdTiles();
    }
    _map_work = kMapWorkStartup;
    if (_panel_attached && _panel.tilesReady())
    {
        _startup_phase = kStartupLoadPrefs;
    }
    else
    {
        _startup_phase = _panel_attached ? kStartupSkipFrame : kStartupNone;
    }
    if (_startup_phase == kStartupNone)
        _map_work = kMapWorkRefresh;
    scheduleMapWork();
}

void TrackerScreen::onExit()
{
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    if (_biz.teamGeofenceState() == Heltec::MeshCore::Team::GeofenceState::Editing)
    {
        _biz.cancelTeamGeofenceEdit();
        syncFenceOverlay();
        syncFenceTools();
    }
#endif
    _biz.setMapForegroundActive(false);
    if (_panel_attached)
    {
        if (_pan_active || _panel.panDragging())
        {
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
    _team_refresh_enabled = false;
    _next_team_refresh_ms = 0;
    setMapToolbarBusy(false);
    if (_toolbar)
        lv_obj_add_flag(_toolbar, LV_OBJ_FLAG_HIDDEN);
    if (_map_work_timer && !_pool_prewarm_pending)
        lv_timer_pause(_map_work_timer);
    if (!_pool_prewarm_pending)
        _map_work_timer_active = false;
    _map_work = kMapWorkNone;
    _startup_phase = kStartupNone;
    AbstractScreen::onExit();
    if (_pool_prewarm_pending && !_map_work_timer_active)
    {
        scheduleMapWork(kPoolPrewarmIntervalMs);
    }
}

void TrackerScreen::mapWorkTimerCb(lv_timer_t *timer)
{
    auto *self = timer ? static_cast<TrackerScreen *>(timer->user_data) : nullptr;
    if (!self)
        return;
    if (timer)
        lv_timer_pause(timer);
    self->_map_work_timer_active = false;
    if (!self->_root)
        return;
    if (self->_pool_prewarm_pending)
    {
        (void)self->processPoolPrewarm();
        if (self->_pool_prewarm_pending)
        {
            self->scheduleMapWork(kPoolPrewarmIntervalMs);
            return;
        }
    }
    if (lv_obj_has_flag(self->_root, LV_OBJ_FLAG_HIDDEN))
        return;
    self->processMapWork();
    if (!self->_pan_active && !self->_panel.panDragging() && self->hasMapWork())
    {
        self->scheduleMapWork(self->nextMapWorkDelayMs());
    }
}

void TrackerScreen::requestMapWork(uint8_t work, uint32_t delay_ms)
{
    if (_map_unavailable)
        return;
    _map_work = static_cast<uint8_t>(_map_work | work);
    if (_pan_active || (_panel_attached && _panel.panDragging()))
        return;
    scheduleMapWork(delay_ms);
}

void TrackerScreen::scheduleMapWork(uint32_t delay_ms)
{
    if (_map_unavailable)
        return;
    if (!_root || (lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN) && !_pool_prewarm_pending))
        return;
    if (!hasMapWork())
        return;

    if (delay_ms == 0)
        delay_ms = 1;
    const uint32_t now_ms = millis();
    if (_map_work_timer_active && ms_until(now_ms, _map_work_due_ms) <= delay_ms)
        return;

    if (!_map_work_timer)
    {
        setMapUnavailable();
        return;
    }
    lv_timer_set_period(_map_work_timer, delay_ms);
    _map_work_due_ms = now_ms + delay_ms;
    _map_work_timer_active = true;
    lv_timer_reset(_map_work_timer);
    lv_timer_resume(_map_work_timer);
}

bool TrackerScreen::processPoolPrewarm()
{
    if (!_pool_prewarm_pending)
        return true;
    if (!_panel_attached)
        ensure_panel_attached();
    if (!_panel_attached)
    {
        setMapUnavailable();
        return true;
    }

    const bool ready = _panel.prewarmPools(kTilesPerPoll, kMarkersPerPoll);
    if (ready)
    {
        _pool_prewarm_pending = false;
    }
    else if (_panel.poolBuildFailed())
    {
        setMapUnavailable();
    }
    return !_pool_prewarm_pending;
}

bool TrackerScreen::processStartupWork()
{
    switch (_startup_phase)
    {
    case kStartupNone:
        _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkStartup);
        return false;

    case kStartupSkipFrame:
        _startup_phase = kStartupBuildTiles;
        return true;

    case kStartupBuildTiles:
        if (!_panel_attached)
            ensure_panel_attached();
        if (!_panel_attached)
        {
            setMapUnavailable();
            return true;
        }
        if (_panel.buildPendingTiles(kTilesPerPoll))
        {
            _startup_phase = kStartupLoadPrefs;
        }
        else if (_panel.tileBuildFailed())
        {
            setMapUnavailable();
        }
        return true;

    case kStartupLoadPrefs:
    {
#if defined(ESP_PLATFORM)
        esp_task_wdt_reset();
#endif
        if (!_panel_attached)
            ensure_panel_attached();
        if (!_panel_attached)
        {
            setMapUnavailable();
            return true;
        }
        if (!_panel.tilesReady())
        {
            _startup_phase = kStartupBuildTiles;
            return true;
        }
        if (_root)
            lv_obj_update_layout(_root);
        Biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
        const bool gps_usable = usable_gps(gps);
        float gps_home[2] = { (float)gps.lat_deg, (float)gps.lon_deg };
        _panel.load_prefs(gps_usable ? gps_home : nullptr);
        _panel.set_gps((float)gps.lat_deg, (float)gps.lon_deg, gps_usable);
        _auto_center_on_first_fix = !gps_usable;
        if (!gps_usable && !_panel.hasSavedCenter())
        {
            const Biz::MapPlotUi &plot = _biz.mapPlotUi();
            if (plot.marker_count > 0)
            {
                _panel.centerOnLocation(static_cast<float>(plot.markers[0].lat_micro) * 1.0e-6f,
                                        static_cast<float>(plot.markers[0].lon_micro) * 1.0e-6f,
                                        false);
            }
        }
        _panel.syncViewportLayout();
        _startup_phase = Map::map_tile_store_ready() ? kStartupApplySd : kStartupProbeSd;
        return true;
    }

    case kStartupProbeSd:
    {
#if defined(ESP_PLATFORM)
        esp_task_wdt_reset();
#endif
        const bool probe_done = Map::map_tile_store_probe_once();
        if (!probe_done)
            return true;
        _startup_phase = kStartupApplySd;
        return true;
    }

    case kStartupApplySd:
    {
        if (!_panel_attached)
            ensure_panel_attached();
        if (!_panel_attached)
        {
            setMapUnavailable();
            return true;
        }
        if (!_panel.tilesReady())
        {
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

void TrackerScreen::processMapWork()
{
    if (_pan_active || (_panel_attached && _panel.panDragging()))
        return;

    if (_map_work & kMapWorkStartup)
    {
        (void)processStartupWork();
        return;
    }

    if (!_panel_attached)
        ensure_panel_attached();
    if (!_panel_attached)
    {
        setMapUnavailable();
        return;
    }

    if (!_panel.tilesReady())
    {
        if (_panel.tileBuildFailed())
        {
            setMapUnavailable();
            return;
        }
        setMapToolbarBusy(true);
        _startup_phase = kStartupBuildTiles;
        _map_work = static_cast<uint8_t>(_map_work | kMapWorkStartup);
        return;
    }

    if (_map_work & kMapWorkRefresh)
    {
        _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkRefresh);
        refreshMap(false);
        if (_panel.tilesLoadPending())
        {
            _last_tile_work_ms = millis();
            _map_work = static_cast<uint8_t>(_map_work | kMapWorkTiles);
        }
        return;
    }

    if (_team_refresh_enabled &&
        static_cast<int32_t>(millis() - _next_team_refresh_ms) >= 0)
    {
        _next_team_refresh_ms = millis() + kTeamRefreshPeriodMs;
        refreshMap(false);
        return;
    }

    _panel.refreshSdTiles();
    if ((_map_work & kMapWorkTiles) || _panel.tilesLoadPending())
    {
        const uint32_t now_ms = millis();
        const uint32_t pan_wait_ms =
            remaining_wait_ms(now_ms, _last_pan_ms, MAP_UI_TILE_LOAD_AFTER_PAN_MS);
        const uint32_t load_wait_ms =
            remaining_wait_ms(now_ms, _last_tile_work_ms, MAP_UI_TILE_LOAD_INTERVAL_MS);
        const uint32_t wait_ms = pan_wait_ms > load_wait_ms ? pan_wait_ms : load_wait_ms;
        if (wait_ms > 0)
            return;
        _panel.commit();
        _last_tile_work_ms = millis();
        if (!_panel.tilesLoadPending())
        {
            _map_work = static_cast<uint8_t>(_map_work & ~kMapWorkTiles);
            update_status_line();
            syncMapToolbarBusy();
            raiseMapToolbar();
        }
    }
}

bool TrackerScreen::hasMapWork() const
{
    return _pool_prewarm_pending || _map_work != kMapWorkNone || _team_refresh_enabled ||
           (_panel_attached && _panel.tilesLoadPending());
}

uint32_t TrackerScreen::nextMapWorkDelayMs() const
{
    if (_pool_prewarm_pending)
        return kPoolPrewarmIntervalMs;
    if ((_map_work & kMapWorkNonTile) != 0)
        return 0;
    if ((_map_work & kMapWorkTiles) || (_panel_attached && _panel.tilesLoadPending()))
    {
        const uint32_t now_ms = millis();
        const uint32_t pan_wait_ms =
            remaining_wait_ms(now_ms, _last_pan_ms, MAP_UI_TILE_LOAD_AFTER_PAN_MS);
        const uint32_t load_wait_ms =
            remaining_wait_ms(now_ms, _last_tile_work_ms, MAP_UI_TILE_LOAD_INTERVAL_MS);
        return pan_wait_ms > load_wait_ms ? pan_wait_ms : load_wait_ms;
    }
    if (_team_refresh_enabled)
        return ms_until(millis(), _next_team_refresh_ms);
    return 0;
}

void TrackerScreen::onAppStateChanged(const AppStateEvent &event)
{
    if (event.type == AppStateEventType::TeamChanged)
    {
        const Biz::IBizFacade::TeamStatus team = _biz.teamStatus();
        _team_member_count_cached = team.has_team ? team.member_count : 0;
        _team_refresh_enabled = team.has_team;
        _next_team_refresh_ms = _team_refresh_enabled ? millis() + kTeamRefreshPeriodMs : 0;
        if (!team.has_team || !team.is_leader)
        {
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
            _biz.deleteTeamGeofence();
            syncFenceOverlay();
#endif
        }
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
        syncFenceTools();
#endif
    }
    if (event.type == AppStateEventType::GpsChanged ||
        event.type == AppStateEventType::ContactLocationChanged ||
        event.type == AppStateEventType::RecentHeardChanged ||
        event.type == AppStateEventType::TeamChanged)
    {
        requestMapWork(kMapWorkRefresh);
    }
}

void TrackerScreen::onActivated()
{
    requestMapWork(kMapWorkRefresh);
}

bool TrackerScreen::build()
{
    if (!AbstractScreen::build())
        return false;
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_status = ht_label_create(&ht_map_status_label_class, _root, "Map");
    if (_lbl_status)
    {
        lv_label_set_text_static(_lbl_status, _status_text);
        lv_obj_set_width(_lbl_status, lv_pct(100));
        lv_label_set_long_mode(_lbl_status, LV_LABEL_LONG_CLIP);
    }

    _map_viewport = ht_class_create(&ht_map_viewport_class, _root);
    if (_map_viewport)
    {
        lv_obj_set_width(_map_viewport, lv_pct(100));
        lv_obj_set_flex_grow(_map_viewport, 1);
        lv_obj_clear_flag(_map_viewport, LV_OBJ_FLAG_SCROLLABLE);
        _toolbar = ht_class_create(kTrackerToolbarClass, _map_viewport);
        if (_toolbar)
        {
            const int tool_count =
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
                5;
#else
                3;
#endif
            lv_obj_set_size(_toolbar, map_toolbar_width(tool_count),
                            kMapToolbarPad * 2 + kMapToolBtnH);
            lv_obj_align(_toolbar, LV_ALIGN_BOTTOM_MID, 0, -4);
            lv_obj_set_flex_flow(_toolbar, LV_FLEX_FLOW_ROW);
            lv_obj_set_height(_toolbar, LV_SIZE_CONTENT);
            lv_obj_set_flex_align(_toolbar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(_toolbar, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(_toolbar, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
            lv_obj_move_foreground(_toolbar);
            lv_obj_add_event_cb(_map_viewport, onMapViewportSizeChanged,
                                LV_EVENT_SIZE_CHANGED, this);

            _btn_zoom_out = ht_btn_create(kTrackerToolbarButtonClass, _toolbar);
            _btn_zoom_in = ht_btn_create(kTrackerToolbarButtonClass, _toolbar);
            _btn_gps = ht_btn_create(kTrackerToolbarButtonClass, _toolbar);
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
            _btn_fence = ht_btn_create(kTrackerToolbarButtonClass, _toolbar);
            _btn_fence_delete = ht_btn_create(kTrackerToolbarButtonClass, _toolbar);
#endif
            _lv_obj_t *const buttons[] = {
                _btn_zoom_out,
                _btn_zoom_in,
                _btn_gps,
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
                _btn_fence, _btn_fence_delete,
#endif
            };
            for (_lv_obj_t *button : buttons)
            {
                if (!button)
                    continue;
                lv_obj_set_size(button, kMapToolBtnW, kMapToolBtnH);
                lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
            }
            if (_btn_zoom_out)
            {
                (void)add_tool_btn_label(_btn_zoom_out, "-");
                lv_obj_add_event_cb(_btn_zoom_out, onZoomOutClicked, LV_EVENT_CLICKED, this);
                addControl(_btn_zoom_out);
            }
            if (_btn_zoom_in)
            {
                (void)add_tool_btn_label(_btn_zoom_in, "+");
                lv_obj_add_event_cb(_btn_zoom_in, onZoomInClicked, LV_EVENT_CLICKED, this);
                addControl(_btn_zoom_in);
            }
            if (_btn_gps)
            {
                (void)add_tool_btn_label(_btn_gps, "G");
                lv_obj_add_event_cb(_btn_gps, onGpsClicked, LV_EVENT_CLICKED, this);
                addControl(_btn_gps);
            }
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
            if (_btn_fence)
            {
                (void)add_tool_btn_label(_btn_fence, "F");
                lv_obj_add_event_cb(_btn_fence, onFenceToolLongPressed,
                                    LV_EVENT_LONG_PRESSED, this);
                addControl(_btn_fence);
            }
            if (_btn_fence_delete)
            {
                (void)add_tool_btn_label(_btn_fence_delete, "D");
                lv_obj_add_event_cb(_btn_fence_delete, onFenceToolLongPressed,
                                    LV_EVENT_LONG_PRESSED, this);
                addControl(_btn_fence_delete);
            }
            syncFenceTools();
#endif
            syncMapToolbarVisibility();
            raiseMapToolbar();
        }
    }

    _map_work_timer = lv_timer_create(mapWorkTimerCb, 1U, this);
    if (_map_work_timer)
    {
        lv_timer_set_repeat_count(_map_work_timer, -1);
        lv_timer_pause(_map_work_timer);
        _pool_prewarm_pending = true;
        scheduleMapWork(1U);
    }
    else
    {
        setMapUnavailable();
    }

    return true;
}

} // namespace Heltec::MeshCore::Ui

#endif // ENV_INCLUDE_MAP
