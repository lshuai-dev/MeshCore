#pragma once

#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"
#include "../core/biz_facade.hpp"

struct _lv_timer_t;

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RecentRow = ht_meta_id(MetaIdScope::Screen, 0x60);
constexpr MetaId RecentName = ht_meta_id(MetaIdScope::Screen, 0x61);
constexpr MetaId RecentAge = ht_meta_id(MetaIdScope::Screen, 0x62);
}

class RecentScreen : public AbstractScreen {
 public:
  RecentScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  eScreenId screenId() const override { return eScreenId::Recent; }
  void onEnter() override;
  void onExit() override;

  protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;
  bool onKey(uint32_t key) override;

 private:
  // The advert cache capacity is independent of the number of LVGL row
  // controls. Only as many controls as fit completely in the viewport are
  // created; those controls are reused for each data window.
  static constexpr int kAdvertCapacity = 16;

  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  bool createRowControl(int index);
  int calculateRowControlCount();
  void refreshRows();
  void renderWindow();
  void moveSelection(int delta);
  void focusSelection();
  static void onRowFocused(_lv_event_t* event);
  static void refreshTimerCallback(_lv_timer_t* timer);

  biz::IBizFacade::RecentlyHeardItem _items[kAdvertCapacity]{};
  _lv_obj_t* _rows[kAdvertCapacity] = {};
  _lv_obj_t* _names[kAdvertCapacity] = {};
  _lv_obj_t* _ages[kAdvertCapacity] = {};
  _lv_timer_t* _refresh_timer = nullptr;
  char _name_text[kAdvertCapacity][32] = {};
  char _age_text[kAdvertCapacity][12] = {};
  int _row_control_count = 0;
  int _item_count = 0;
  int _selected_index = 0;
  int _window_start = 0;
};

}  // namespace heltec::meshcore::ui
