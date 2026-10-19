#pragma once
#include "../core/AbstractScreen.hpp"
#include "../core/AppStateEvent.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

class RadioScreen : public AbstractScreen {
  public:
    RadioScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon) {
    }

  protected:
    bool buildSurface() override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void onActivated() override;
    void updateRadio(const biz::IBizFacade::RadioStatus &radio);
    void refreshSnapshot();
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
    void setLnaSwitchState(bool on);
    static void handleSwitchKey(lv_event_t *e);
    static void handleLNAValueChanged(lv_event_t *e);
#endif

    _lv_obj_t *_line1 = nullptr;
    _lv_obj_t *_line2 = nullptr;
    _lv_obj_t *_line3 = nullptr;
    _lv_obj_t *_line4 = nullptr;
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
    _lv_obj_t *_sw_lna = nullptr;
    bool _syncing_lna = false;
#endif
    char _line_text[4][32] = {};
};

} // namespace heltec::meshcore::ui
