#pragma once
#include "heltec/ui/core/AbstractScreen.hpp"
#include "heltec/ui/core/AppStateEvent.hpp"

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
}

namespace Heltec::MeshCore::Ui
{

class GPSScreen : public AbstractScreen
{
  public:
    GPSScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon)
    {
    }

  protected:
    void onEnter() override;
    void onExit() override;
    void onActivated() override;
    bool build() override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void updateGps(const Biz::IBizFacade::GpsStatus &s);
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

} // namespace Heltec::MeshCore::Ui
