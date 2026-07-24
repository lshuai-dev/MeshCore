#pragma once

#include "heltec/ui/core/abstract_screen.hpp"
#include "heltec/ui/core/app_state_event.hpp"

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "heltec/ui/map/map_panel.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

class TrackerScreen : public AbstractScreen {
 public:
  TrackerScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon, false) {}
  _lv_obj_t* create(_lv_obj_t* parent) override;
  void onEnter() override;
  void onExit() override;
  eScreenId screenId() const override { return eScreenId::Tracker; }
  bool hitMapViewport(lv_coord_t x, lv_coord_t y) const;
  bool hitMapToolbar(lv_coord_t x, lv_coord_t y) const;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void ensure_panel_attached();
  void refresh_from_biz();
  void refresh_markers_only();
  void center_map_on_current_gps();
  void refresh();
  void requestMapWork(uint8_t work, uint32_t delay_ms = 0);
  void scheduleMapWork(uint32_t delay_ms = 0);
  void processMapWork();
  bool processPoolPrewarm();
  bool processStartupWork();
  bool hasMapWork() const;
  uint32_t nextMapWorkDelayMs() const;
  void update_status_line();
  void update_status_line(const biz::MapPlotUi& plot);
  void syncMapToolbarVisibility();
  void raiseMapToolbar();
  void setMapToolbarBusy(bool busy);
  void syncMapToolbarBusy();
  void setMapUnavailable();
  void runMapToolAction(void (*action)(TrackerScreen& self));
  void bindPanGestures(_lv_obj_t* obj);
  static void on_map_gesture(lv_event_t* e);
  static void mapWorkTimerCb(lv_timer_t* timer);

  map::MapPanel _panel;
  _lv_obj_t* _toolbar = nullptr;
  _lv_obj_t* _lbl_status = nullptr;
  _lv_obj_t* _map_viewport = nullptr;
  _lv_obj_t* _btn_zoom_in = nullptr;
  _lv_obj_t* _btn_zoom_out = nullptr;
  _lv_obj_t* _btn_gps = nullptr;
  char _status_text[112] = "Map";
  lv_point_t _pan_origin = {0, 0};
  int32_t _pan_applied_x = 0;
  int32_t _pan_applied_y = 0;
  bool _pan_active = false;
  bool _pan_moved = false;
  bool _user_panned = false;
  bool _auto_center_on_first_fix = false;
  bool _panel_attached = false;
  bool _toolbar_busy = false;
  int8_t _toolbar_gps_usable = -1;
  bool _map_unavailable = false;
  bool _pool_prewarm_pending = false;
  bool _map_work_timer_active = false;
  lv_timer_t* _map_work_timer = nullptr;
  int _contact_gps_count = 0;
  uint8_t _map_work = 0;
  uint8_t _startup_phase = 0;
  uint32_t _map_work_due_ms = 0;
  uint32_t _last_pan_ms = 0;
  uint32_t _last_tile_work_ms = 0;
};

}  // namespace heltec::meshcore::ui

#endif  // ENV_INCLUDE_MAP
