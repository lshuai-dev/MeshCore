#pragma once
#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId HomeIdLabel = ht_meta_id(MetaIdScope::Screen, 0x20);
constexpr MetaId HomeMessageLabel = ht_meta_id(MetaIdScope::Screen, 0x21);
constexpr MetaId HomeStatusLabel = ht_meta_id(MetaIdScope::Screen, 0x22);
}

class HomeScreen : public AbstractScreen {
 public:
  HomeScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  eScreenId screenId() const override { return eScreenId::Home; }

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void refreshSnapshot();
  void refreshLabels();

  _lv_obj_t* _lblId = nullptr;
  _lv_obj_t* _lblMsg = nullptr;
  _lv_obj_t* _lblStatus = nullptr;
  int _message_count = 0;
  bool _companion_connected = false;
  uint32_t _pairing_pin = 0;
  char _id_line[40] = {};
  char _message_line[24] = {};
  char _status_line[24] = {};
};

}  // namespace heltec::meshcore::ui
