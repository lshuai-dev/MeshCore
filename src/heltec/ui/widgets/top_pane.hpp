#pragma once
#include <lvgl.h>
#include <stdint.h>

#include "ui/core/ht_meta_data.hpp"

struct _lv_obj_t;
namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId TopPaneRoot = ht_meta_id(MetaIdScope::TopPane, 0x00);
constexpr MetaId TopPaneLeftSlot = ht_meta_id(MetaIdScope::TopPane, 0x01);
constexpr MetaId TopPaneCenterSlot = ht_meta_id(MetaIdScope::TopPane, 0x02);
constexpr MetaId TopPaneRightSlot = ht_meta_id(MetaIdScope::TopPane, 0x03);
constexpr MetaId TopPaneTitle = ht_meta_id(MetaIdScope::TopPane, 0x04);
constexpr MetaId TopPaneBattery = ht_meta_id(MetaIdScope::TopPane, 0x05);
constexpr MetaId TopPaneBatteryOutline = ht_meta_id(MetaIdScope::TopPane, 0x06);
constexpr MetaId TopPaneBatteryFill = ht_meta_id(MetaIdScope::TopPane, 0x07);
constexpr MetaId TopPaneBatteryCap = ht_meta_id(MetaIdScope::TopPane, 0x08);
}

struct UiTopPaneMetrics {
  lv_coord_t height;
  lv_coord_t radius;
  lv_coord_t battery_w;
  lv_coord_t battery_h;
  lv_coord_t battery_pad_r;
  lv_coord_t pad_top;
};

class TopPane {
public:
  TopPane() = default;

  bool create(_lv_obj_t* parent = nullptr);

  void setTitle(const char* title);
  void setBatteryMilliVolts(uint16_t mv);
  _lv_obj_t* root() const { return _root; }
#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
  void enableTouchShell(lv_event_cb_t on_short_press, lv_event_cb_t on_long_press, void* user_data);
#endif

protected:
  void renderBattery(uint16_t uiMv);

private:
  static void onRootSizeChanged(lv_event_t* e);
#if defined(HELTEC_V4_R8_TFT)
  static void onBatteryDraw(lv_event_t* e);
#else
  static void onBatteryOutlineSizeChanged(lv_event_t* e);
#endif

  void layoutBattery();

  _lv_obj_t*  _root = nullptr;
  _lv_obj_t*  _title = nullptr;
#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
  _lv_obj_t*  _touch_slot = nullptr;
#endif
  _lv_obj_t*  _bat_cont = nullptr;
#if !defined(HELTEC_V4_R8_TFT)
  _lv_obj_t*  _bat_fill = nullptr;
#endif
  uint16_t    _bat_mv = 0;
};

}  // namespace heltec::meshcore::ui
