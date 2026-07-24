#pragma once

#include "heltec/ui/core/biz_facade.hpp"
#include "heltec/ui/core/screen_id.hpp"
#include "heltec/ui/core/ui_surface.hpp"
#include "heltec/ui/navigation/ui_navigator.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

/** V4 grid nav overlay. */
class NavigationPane : public UiSurface {
 public:
  explicit NavigationPane(biz::IBizFacade& biz) : UiSurface(biz) {}
  ~NavigationPane();

  void configure(const UiNavigationItem* items, uint8_t count);
  void setSelectedIndex(uint8_t screen_index);
  bool isTransitioning() const { return _ring_fade_busy; }
  uint8_t focusedIndex() const;
  /** Slide the pane left, then emit NavClose. Returns false when it should close immediately. */
  bool requestCloseAnimation();
  void bindView(_lv_obj_t* frame, _lv_obj_t* tileview);

  void onEnter() override;
  void onExit() override;
  uint16_t inputRebindDelayMs() const override;

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  bool onKey(uint32_t lv_key) override;
  void setIcon(uint8_t screen_index, const lv_img_dsc_t* image);
  void setLabel(uint8_t screen_index, const char* label);
  void setFooterSlot(uint8_t screen_index);
  _lv_obj_t* itemHost() const;
  _lv_obj_t* findCellById(uint8_t id) const;
  uint8_t btnCount() const;
  bool panelVisible() const;
  void openPanel();
  void resetPanel();
  void layoutGrid(bool update_emphasis = true);
  void layoutNav(bool animate, bool snap_theta = true, bool update_emphasis = true);
  void updateGeometry();
  void setNavButtonsInteractive(bool interactive);
  void stepNavFocus(int delta);
  void onCellClicked(_lv_obj_t* cell);
  bool commitFocused();
  _lv_obj_t* frameRoot() const;

  static constexpr uint8_t kMaxButtons = static_cast<uint8_t>(eScreenId::kScreenCnt);
  static constexpr uint8_t kNoEmphasis = 0xFF;

  _lv_obj_t* _nav = nullptr;
  _lv_obj_t* _tileview = nullptr;
  _lv_obj_t* _frame_root = nullptr;
  uint8_t _emphasis_index = kNoEmphasis;
  uint8_t _ring_layout_focus = kNoEmphasis;
  bool _ring_fade_busy = false;
  bool _close_animating = false;
  bool _updating_geometry = false;
  bool _layout_busy = false;
  const char* _labels[kMaxButtons] = {};
  uint8_t _footer_id = static_cast<uint8_t>(eScreenId::None);
};

}  // namespace heltec::meshcore::ui
