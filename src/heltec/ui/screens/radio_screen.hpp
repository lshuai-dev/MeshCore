#pragma once
#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RadioLineLabel = ht_meta_id(MetaIdScope::Screen, 0x80);
constexpr MetaId RadioLnaRow = ht_meta_id(MetaIdScope::Screen, 0x81);
constexpr MetaId RadioLnaLabel = ht_meta_id(MetaIdScope::Screen, 0x82);
constexpr MetaId RadioLnaSwitch = ht_meta_id(MetaIdScope::Screen, 0x83);
}

class RadioScreen : public AbstractScreen {
 public:
  RadioScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  eScreenId screenId() const override { return eScreenId::Radio; }

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void updateRadio(const biz::IBizFacade::RadioStatus& radio);
  void refreshSnapshot();
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  void setLnaSwitchState(bool on);
  static void onLnaSwitchKey(lv_event_t* e);
  static void onLnaSwitchValueChanged(lv_event_t* e);
#endif

  _lv_obj_t* _line1 = nullptr;
  _lv_obj_t* _line2 = nullptr;
  _lv_obj_t* _line3 = nullptr;
  _lv_obj_t* _line4 = nullptr;
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  _lv_obj_t* _row_lna = nullptr;
  _lv_obj_t* _lbl_lna = nullptr;
  _lv_obj_t* _sw_lna = nullptr;
  bool _syncing_lna = false;
#endif
  char _line_text[4][48] = {};
  biz::IBizFacade::RadioStatus _last_radio{};
};

}  // namespace heltec::meshcore::ui
