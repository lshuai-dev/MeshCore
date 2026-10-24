#pragma once

#include "../core/AbstractScreen.hpp"
#include "../core/AppStateEvent.hpp"
#include "CompassDialWidget.hpp"

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
struct CompassUi;
} // namespace Heltec::MeshCore::Biz

namespace Heltec::MeshCore::Ui
{

class CompassScreen final : public AbstractScreen
{
  public:
    CompassScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon)
    {
    }

    void skipAutoCalibrationOnce();

  protected:
    void onEnter() override;
    void onExit() override;
    void onActivated() override;
    bool build() override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void refresh_ui(const Biz::CompassUi &c);
    void refresh();
    void showUnavailable(const Biz::CompassUi &c, const char *hdg_text);
    void clearMagLabels();
    void updateDialHeading(float heading_deg);
    void maybeScheduleAutoCal();

    CompassDialWidget _dial;
    lv_obj_t *_lbl_hdg = nullptr;
    lv_obj_t *_lbl_q = nullptr;
    lv_obj_t *_lbl_mag = nullptr;
    char _hdg_text[20] = "HDG:--";
    char _q_text[24] = "Q:--";
    char _mag_text[56] = "X:--\nY:--\nZ:--";
    int16_t _heading_dial_track_tenths = -10000;
    int16_t _heading_label_tenths = -10000;
    int16_t _last_mag_centi[3] = { 10000, 10000, 10000 };
    int8_t _last_quality = -1;
    uint8_t _state_flags = 0;
    bool _pending_auto_cal = false;
    bool _skip_auto_cal_once = false;
    uint32_t _auto_cal_due_ms = 0;
};

} // namespace Heltec::MeshCore::Ui
