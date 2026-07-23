#pragma once

#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"
#include "compass_dial_widget.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
struct FindFriendUi;
}

namespace heltec::meshcore::ui {

class FindFriendScreen : public AbstractScreen {
 public:
  FindFriendScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  _lv_obj_t* create(_lv_obj_t* parent) override;
  void onEnter() override;
  void onExit() override;
  eScreenId screenId() const override { return eScreenId::FindFriend; }

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void render(const biz::FindFriendUi& u);
  void refresh();
  void showInfoOnly(const char* target, const char* dist, const char* status);
  void runDeferredEnterActions();

  CompassDialWidget _dial;
  lv_obj_t* _lbl_target = nullptr;
  lv_obj_t* _lbl_dist = nullptr;
  lv_obj_t* _lbl_status = nullptr;
  int16_t _ring_heading_tenths = 0;
  float _turn_show_deg = 0.f;
  bool _gps_fix = false;
  bool _on_target = false;
  bool _defer_cycle_target = false;
};

}  // namespace heltec::meshcore::ui
