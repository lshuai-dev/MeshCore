#pragma once
#include "heltec/ui/core/AbstractScreen.hpp"
#include "heltec/ui/core/AppStateEvent.hpp"

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
    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void onActivated() override;
    void updateGps(const biz::IBizFacade::GpsStatus &s);
    void refreshSnapshot();
    void setSwitchState(_lv_obj_t *sw, bool on);
    bool ensureGpsEnabled(_lv_obj_t *dependent_switch);
    static void onGpsSwitchKey(lv_event_t *e);
    static void onGpsSwitchValueChanged(lv_event_t *e);

    _lv_obj_t *_sw_gps = nullptr;
    _lv_obj_t *_lbl_fix = nullptr;
    _lv_obj_t *_lbl_sat = nullptr;
    _lv_obj_t *_lbl_latlon = nullptr;
    _lv_obj_t *_lbl_alt = nullptr;
    _lv_obj_t *_lbl_speed = nullptr;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    _lv_obj_t *_sw_track = nullptr;
#endif
    char _sat_text[16] = "sat --";
    char _latlon_text[56] = "lat -- lon --";
    char _alt_text[24] = "alt --";
    char _speed_text[24] = "speed -- km/h";
    bool _syncing_switches = false;
};

} // namespace heltec::meshcore::ui
