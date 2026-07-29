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
constexpr MetaId GpsPowerSwitch = ht_meta_id(MetaIdScope::Screen, 0x42);
constexpr MetaId GpsFixLabel = ht_meta_id(MetaIdScope::Screen, 0x43);
constexpr MetaId GpsSatLabel = ht_meta_id(MetaIdScope::Screen, 0x44);
constexpr MetaId GpsLatLonLabel = ht_meta_id(MetaIdScope::Screen, 0x45);
constexpr MetaId GpsAltLabel = ht_meta_id(MetaIdScope::Screen, 0x46);
constexpr MetaId GpsRawLabel = ht_meta_id(MetaIdScope::Screen, 0x47);
constexpr MetaId GpsTrackRow = ht_meta_id(MetaIdScope::Screen, 0x48);
constexpr MetaId GpsTrackLabel = ht_meta_id(MetaIdScope::Screen, 0x49);
constexpr MetaId GpsTrackSwitch = ht_meta_id(MetaIdScope::Screen, 0x4A);
}

class GPSScreen : public AbstractScreen {
 public:
  GPSScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  eScreenId screenId() const override { return eScreenId::GPS; }

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void updateGps(const biz::IBizFacade::GpsStatus& s);
  void refreshSnapshot();
  void setSwitchState(_lv_obj_t* sw, bool on);
  static void onGpsSwitchKey(lv_event_t* e);
  static void onGpsSwitchValueChanged(lv_event_t* e);

  _lv_obj_t* _row_power = nullptr;
  _lv_obj_t* _lbl_gps_prefix = nullptr;
  _lv_obj_t* _sw_gps = nullptr;
  _lv_obj_t* _lbl_fix = nullptr;
  _lv_obj_t* _lbl_sat = nullptr;
  _lv_obj_t* _lbl_latlon = nullptr;
  _lv_obj_t* _lbl_alt = nullptr;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _lv_obj_t* _row_track = nullptr;
  _lv_obj_t* _lbl_track = nullptr;
  _lv_obj_t* _sw_track = nullptr;
#endif
  char _sat_text[16] = "sat --";
  char _latlon_text[56] = "lat -- lon --";
  char _alt_text[24] = "alt --";
  biz::IBizFacade::GpsStatus _gps{};
  bool _syncing_switches = false;
};

}  // namespace heltec::meshcore::ui
