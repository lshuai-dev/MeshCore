#pragma once
#include "heltec/ui/core/abstract_screen.hpp"
#include "heltec/ui/core/app_state_event.hpp"

namespace heltec::meshcore::biz
{
class IBizFacade;
}

namespace heltec::meshcore::ui
{

class GPSScreen : public AbstractScreen
{
  public:
    GPSScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon)
    {
    }
    eScreenId screenId() const override
    {
        return eScreenId::GPS;
    }
    void onEnter() override;
    void onExit() override;

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void onSurfaceRefresh() override;
    void updateGps(const biz::IBizFacade::GpsStatus &s);
    void refreshSnapshot();
    void setSwitchState(_lv_obj_t *sw, bool on);
    bool ensureGpsEnabled(_lv_obj_t *dependent_switch);
    void setAdvIntervalIndex(uint16_t index);
    void updateConditionalVisibility();
    void closeAdvDropdown();
    void realignAdvDropdownList();
    static void onGpsSwitchKey(lv_event_t *e);
    static void onGpsSwitchValueChanged(lv_event_t *e);
    static void onAdvDropdownKey(lv_event_t *e);
    static void onAdvDropdownValueChanged(lv_event_t *e);
    static void onAdvDropdownStateEvent(lv_event_t *e);
    static void onAdvDropdownReleasedPre(lv_event_t *e);
    static void realignAdvDropdownListAsync(void *user_data);

    _lv_obj_t *_row_power = nullptr;
    _lv_obj_t *_lbl_gps_prefix = nullptr;
    _lv_obj_t *_sw_gps = nullptr;
    _lv_obj_t *_row_location_share = nullptr;
    _lv_obj_t *_lbl_location_share = nullptr;
    _lv_obj_t *_sw_location_share = nullptr;
    _lv_obj_t *_row_adv_interval = nullptr;
    _lv_obj_t *_lbl_adv_interval = nullptr;
    _lv_obj_t *_dd_adv_interval = nullptr;
    _lv_obj_t *_lbl_fix = nullptr;
    _lv_obj_t *_lbl_sat = nullptr;
    _lv_obj_t *_lbl_latlon = nullptr;
    _lv_obj_t *_lbl_alt = nullptr;
    _lv_obj_t *_lbl_speed = nullptr;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    _lv_obj_t *_row_track = nullptr;
    _lv_obj_t *_lbl_track = nullptr;
    _lv_obj_t *_sw_track = nullptr;
#endif
    char _sat_text[16] = "sat --";
    char _latlon_text[56] = "lat -- lon --";
    char _alt_text[24] = "alt --";
    char _speed_text[24] = "speed -- km/h";
    char _adv_interval_options[64] = {};
    biz::IBizFacade::GpsStatus _gps{};
    bool _syncing_switches = false;
    bool _syncing_adv_interval = false;
    bool _adv_dropdown_open = false;
    uint16_t _adv_dropdown_original_index = 0;
};

} // namespace heltec::meshcore::ui
