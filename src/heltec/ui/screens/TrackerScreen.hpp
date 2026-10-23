#pragma once

#include "heltec/ui/core/AbstractScreen.hpp"
#include "heltec/ui/core/AppStateEvent.hpp"
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
#include "heltec/app/TeamGeofence.hpp"
#endif

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "heltec/ui/map/MapPanel.hpp"

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
}

namespace Heltec::MeshCore::Ui
{

extern const lv_obj_class_t ht_tracker_screen_root_class;

class TrackerScreen : public AbstractScreen
{
  public:
    TrackerScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon);
    void onEnter() override;
    void onExit() override;
    bool hitMapViewport(lv_coord_t x, lv_coord_t y) const;
    bool hitMapToolbar(lv_coord_t x, lv_coord_t y) const;

  protected:
    bool buildSurface() override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void onActivated() override;
    void ensure_panel_attached();
    void refreshMap(bool refresh_sd_tiles);
    void center_map_on_current_gps();
    void requestMapWork(uint8_t work, uint32_t delay_ms = 0);
    void scheduleMapWork(uint32_t delay_ms = 0);
    void processMapWork();
    bool processPoolPrewarm();
    bool processStartupWork();
    bool hasMapWork() const;
    uint32_t nextMapWorkDelayMs() const;
    void update_status_line();
    void syncMapToolbarVisibility();
    void raiseMapToolbar();
    void setMapToolbarBusy(bool busy);
    void syncMapToolbarBusy();
    void setMapUnavailable();
    void runMapToolAction(void (*action)(TrackerScreen &self));
    void bindPanGestures(_lv_obj_t *obj);
    static void onMapViewportSizeChanged(lv_event_t *event);
    static void onZoomOutClicked(lv_event_t *event);
    static void onZoomInClicked(lv_event_t *event);
    static void onGpsClicked(lv_event_t *event);
    static void zoomOutMap(TrackerScreen &screen);
    static void zoomInMap(TrackerScreen &screen);
    static void centerMapOnGps(TrackerScreen &screen);
    static void on_map_gesture(lv_event_t *e);
    static void mapWorkTimerCb(lv_timer_t *timer);

    Map::MapPanel _panel;
    _lv_obj_t *_toolbar = nullptr;
    _lv_obj_t *_lbl_status = nullptr;
    _lv_obj_t *_map_viewport = nullptr;
    _lv_obj_t *_btn_zoom_in = nullptr;
    _lv_obj_t *_btn_zoom_out = nullptr;
    _lv_obj_t *_btn_gps = nullptr;
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    _lv_obj_t *_btn_fence = nullptr;
    _lv_obj_t *_btn_fence_delete = nullptr;
    static void onFenceToolLongPressed(lv_event_t *event);
    static void onFenceOverlayDraw(lv_event_t *event);
    void syncFenceTools();
    void syncFenceOverlay();
    void addFencePointAt(lv_coord_t screen_x, lv_coord_t screen_y);
    _lv_obj_t *_fence_overlay = nullptr;
    lv_point_t _fence_points[Heltec::MeshCore::Team::TeamGeofence::kMaxPoints] = {};
    bool _fence_point_visible[Heltec::MeshCore::Team::TeamGeofence::kMaxPoints] = {};
#endif
    char _status_text[112] = "Map";
    lv_point_t _pan_origin = { 0, 0 };
    int32_t _pan_applied_x = 0;
    int32_t _pan_applied_y = 0;
    bool _pan_active = false;
    bool _pan_moved = false;
    bool _user_panned = false;
    bool _auto_center_on_first_fix = false;
    bool _panel_attached = false;
    bool _toolbar_busy = false;
    int _team_member_count_cached = -1;
    int8_t _toolbar_gps_usable = -1;
    bool _map_unavailable = false;
    bool _pool_prewarm_pending = false;
    bool _map_work_timer_active = false;
    lv_timer_t *_map_work_timer = nullptr;
    int _team_location_count = 0;
    int _team_online_count = 0;
    bool _team_refresh_enabled = false;
    uint8_t _map_work = 0;
    uint8_t _startup_phase = 0;
    uint32_t _map_work_due_ms = 0;
    uint32_t _last_pan_ms = 0;
    uint32_t _last_tile_work_ms = 0;
    uint32_t _next_team_refresh_ms = 0;
};

} // namespace Heltec::MeshCore::Ui

#endif // ENV_INCLUDE_MAP
