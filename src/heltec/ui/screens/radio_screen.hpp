#pragma once
#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RadioLineLabel = ht_meta_id(MetaIdScope::Screen, 0x80);
}

class RadioScreen : public AbstractScreen {
 public:
  RadioScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  _lv_obj_t* create(_lv_obj_t* parent) override;
  void onEnter() override;
  eScreenId screenId() const override { return eScreenId::Radio; }

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void updateRadio(const biz::IBizFacade::RadioStatus& radio);
  void refreshSnapshot();

  _lv_obj_t* _line1 = nullptr;
  _lv_obj_t* _line2 = nullptr;
  _lv_obj_t* _line3 = nullptr;
  _lv_obj_t* _line4 = nullptr;
  char _line_text[4][48] = {};
  biz::IBizFacade::RadioStatus _last_radio{};
};

}  // namespace heltec::meshcore::ui
