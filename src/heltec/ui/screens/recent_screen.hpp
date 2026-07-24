#pragma once
#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RecentRowLabel = ht_meta_id(MetaIdScope::Screen, 0x60);
}

class RecentScreen : public AbstractScreen {
 public:
  RecentScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  eScreenId screenId() const override { return eScreenId::Recent; }

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  static constexpr int kMaxRows = 10;

  void refreshRecent();

  _lv_obj_t* _scroll = nullptr;
  _lv_obj_t* _rows[kMaxRows] = {nullptr};
  char _row_text[kMaxRows][48] = {};
};

}  // namespace heltec::meshcore::ui
