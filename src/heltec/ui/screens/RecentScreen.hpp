#pragma once
#include "../core/AbstractScreen.hpp"
#include "../core/AppStateEvent.hpp"
#include "../core/BizFacade.hpp"
struct _lv_timer_t;

namespace heltec::meshcore::ui {
class RecentScreen : public AbstractScreen {
  public:
    RecentScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon) {
    }
    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;
    bool onKey(uint32_t key) override;

  private:
    static constexpr int kAdvertCapacity = 16;
    static constexpr int kMaxRowControls = 8;

    void onAppStateChanged(const AppStateEvent &event) override;
    void onActivated() override;
    bool createRowControl(int index);
    int calculateRowControlCount();
    void refreshRows();
    void renderWindow();
    void movePage(int delta);
    static void refreshTimerCallback(_lv_timer_t *timer);

    biz::IBizFacade::RecentlyHeardItem _items[kAdvertCapacity]{};
    _lv_obj_t *_rows[kMaxRowControls] = {};
    _lv_obj_t *_names[kMaxRowControls] = {};
    _lv_obj_t *_ages[kMaxRowControls] = {};
    _lv_timer_t *_refresh_timer = nullptr;
    char _age_text[kMaxRowControls][12] = {};
    uint32_t _age_tick_ms = 0;
    uint8_t _row_control_count = 0;
    uint8_t _item_count = 0;
    uint8_t _window_start = 0;
};

} // namespace heltec::meshcore::ui
