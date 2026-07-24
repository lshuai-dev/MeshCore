#pragma once

#include "heltec/ui/core/biz_facade.hpp"
#include "heltec/ui/core/screen_id.hpp"
#include "heltec/ui/core/ui_surface.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

/** V4 grid nav overlay. */
class NavigationPane : public UiSurface {
 public:
  explicit NavigationPane(biz::IBizFacade& biz) : UiSurface(biz) {}
  ~NavigationPane();

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void setIcon(uint8_t screen_index, const lv_img_dsc_t* image);
  void setLabel(uint8_t screen_index, const char* label);
  void setFooterSlot(uint8_t screen_index);
  void setSelectedIndex(uint8_t screen_index, bool preview = false);
  bool isTransitioning() const { return _ring_fade_busy; }
  uint8_t focusedIndex() const;
  _lv_obj_t* navButtonHost() const;
  _lv_obj_t* navFocusWidget() const { return _nav ? _nav : _root; }
  /** Slide the pane left, then emit NavClose. Returns false when it should close immediately. */
  bool requestCloseAnimation();
  void setTileView(_lv_obj_t* tileview);
  void setFrameRoot(_lv_obj_t* frame) { _frame_root = frame; }

  void onEnter() override;
  void onExit() override;
  uint16_t inputRebindDelayMs() const override;

 private:
  bool onKey(uint32_t lv_key) override;
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
  void onCellTouchEvent(lv_event_t* e);
  void onCellClicked(_lv_obj_t* cell);
  bool commitFocused();
  void sendTilePreview(uint8_t tile_idx);
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
  bool _touch_active = false;
  bool _touch_dragged = false;
  lv_point_t _touch_origin = {0, 0};
  const char* _labels[kMaxButtons] = {};
  uint8_t _footer_id = static_cast<uint8_t>(eScreenId::None);
};

}  // namespace heltec::meshcore::ui
