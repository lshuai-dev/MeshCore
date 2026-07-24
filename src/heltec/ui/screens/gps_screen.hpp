#pragma once
#include "heltec/ui/core/abstract_screen.hpp"
#include "heltec/ui/core/app_state_event.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId GpsPowerRow = ht_meta_id(MetaIdScope::Screen, 0x40);
constexpr MetaId GpsPowerPrefix = ht_meta_id(MetaIdScope::Screen, 0x41);
constexpr MetaId GpsPowerState = ht_meta_id(MetaIdScope::Screen, 0x42);
constexpr MetaId GpsFixLabel = ht_meta_id(MetaIdScope::Screen, 0x43);
constexpr MetaId GpsSatLabel = ht_meta_id(MetaIdScope::Screen, 0x44);
constexpr MetaId GpsLatLonLabel = ht_meta_id(MetaIdScope::Screen, 0x45);
constexpr MetaId GpsAltLabel = ht_meta_id(MetaIdScope::Screen, 0x46);
constexpr MetaId GpsRawLabel = ht_meta_id(MetaIdScope::Screen, 0x47);
}

class GPSScreen : public AbstractScreen {
 public:
  GPSScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  _lv_obj_t* create(_lv_obj_t* parent) override;
  void onEnter() override;
  void onExit() override;
  eScreenId screenId() const override { return eScreenId::GPS; }

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void updateGps(const biz::IBizFacade::GpsStatus& s);
  void refreshSnapshot();

  _lv_obj_t* _row_power = nullptr;
  _lv_obj_t* _lbl_gps_prefix = nullptr;
  _lv_obj_t* _lbl_gps_state = nullptr;
  _lv_obj_t* _lbl_fix = nullptr;
  _lv_obj_t* _lbl_sat = nullptr;
  _lv_obj_t* _lbl_latlon = nullptr;
  _lv_obj_t* _lbl_alt = nullptr;
  char _sat_text[16] = "sat --";
  char _latlon_text[56] = "lat -- lon --";
  char _alt_text[24] = "alt --";
  biz::IBizFacade::GpsStatus _gps{};
};

}  // namespace heltec::meshcore::ui
