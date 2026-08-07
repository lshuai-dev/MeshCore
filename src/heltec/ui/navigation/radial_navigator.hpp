#pragma once
#include "heltec/ui/core/biz_facade.hpp"
#include "heltec/ui/core/screen_id.hpp"
#include "heltec/ui/core/ui_surface.hpp"
#include "ui/navigation/ui_navigator.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {
class RadialNavigator : public UiSurface {
 public:
  explicit RadialNavigator(biz::IBizFacade& biz) : UiSurface(biz) {}
  ~RadialNavigator();

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void configure(const UiNavigationItem* items, uint8_t count);
  void setIcon(uint8_t screen_index, const lv_img_dsc_t* image);
  void setSelectedIndex(uint8_t screen_index, bool preview = false);
  bool isTransitioning() const { return false; }
  uint8_t focusedIndex() const;
  _lv_obj_t* navButtonHost() const { return itemHost(); }
  _lv_obj_t* navFocusWidget() const { return _nav ? _nav : _root; }
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
  uint8_t slotForId(uint8_t id) const;
  uint8_t focusedSlot() const;
  bool panelVisible() const;
  void openPanel();
  void resetPanel();
  void layoutRing(bool animate, bool snap_theta = true, bool update_emphasis = true);
  void updateGeometry();
  void setNavButtonsInteractive(bool interactive);
  void stepNavFocus(int delta);
  void onCellTouchEvent(lv_event_t* e);
  void onCellClicked(_lv_obj_t* cell);
  void sendTilePreview(uint8_t tile_idx);
  void clearAnimations();
  void ensureDefaultFocus();
  void invalidateSlotCache();
  void rebuildSlotCache(uint8_t count);
  _lv_obj_t* frameRoot() const;

  static constexpr uint8_t kMaxButtons = static_cast<uint8_t>(eScreenId::kScreenCnt);
  static constexpr uint8_t kNoEmphasis = 0xFF;

  _lv_obj_t* _nav = nullptr;
  _lv_obj_t* _tileview = nullptr;
  _lv_obj_t* _frame_root = nullptr;
  float _slot_cos[kMaxButtons] = {};
  float _slot_sin[kMaxButtons] = {};
  float _ring_theta = 0.f;
  uint8_t _emphasis_index = kNoEmphasis;
  uint8_t _ring_layout_focus = kNoEmphasis;
  uint8_t _ring_focus_slot = 0;
  uint8_t _slot_cache_count = 0;
  bool _updating_geometry = false;
  bool _geometry_valid = false;
  bool _cached_panel_visible = false;
  lv_coord_t _cached_nav_x = 0;
  lv_coord_t _cached_nav_y = 0;
  lv_coord_t _cached_nav_side = 0;
  bool _touch_active = false;
  bool _touch_dragged = false;
  lv_point_t _touch_origin = {0, 0};
};

}  // namespace heltec::meshcore::ui
