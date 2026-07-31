/**
 * @file abstract_screen.hpp
 * @brief 全屏 Screen 基类。
 */
#pragma once

#include <stdint.h>
#include <lvgl.h>

#include "screen_id.hpp"
#include "ui_surface.hpp"

struct _lv_obj_t;

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

class AbstractScreen : public UiSurface {
 public:
  AbstractScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon,
                 bool root_scroll_focus = true);
  virtual ~AbstractScreen() = default;

  virtual const char* title() const;
  virtual const lv_img_dsc_t* icon() const;
  virtual eScreenId screenId() const = 0;
  _lv_obj_t* tile() const;
  void onEnter() override;
  void onExit() override;
  _lv_obj_t* focusedObject() const override;

  protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  bool onKey(uint32_t key) override;
  void addFocusItem(_lv_obj_t* object, _lv_obj_t* frame = nullptr,bool focus_on_pointer_press = true);

 private:
  static constexpr uint8_t kMaxFocusItems = 16;


  bool isAvailableFocusItem(const _lv_obj_t* obj) const;
  _lv_obj_t* firstAvailableFocusItem() const;
  static void onFocusItemChanged(lv_event_t* e);

  const char* _title;
  const lv_img_dsc_t* _icon;
  bool _root_scroll_focus;
  bool _root_focus_fallback = false;
  _lv_obj_t* _focus_items[kMaxFocusItems] = {};
  uint8_t _focus_item_count = 0;
};

}  // namespace heltec::meshcore::ui
