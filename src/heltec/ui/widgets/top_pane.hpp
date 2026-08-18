#pragma once
#include "ui/core/ht_meta_data.hpp"

#include <lvgl.h>
#include <stdint.h>

struct _lv_obj_t;
namespace heltec::meshcore::ui
{

namespace meta_id
{
constexpr uint16_t TopPaneRoot = ht_meta_id(MetaIdScope::TopPane, 0x00);
constexpr uint16_t TopPaneLeftSlot = ht_meta_id(MetaIdScope::TopPane, 0x01);
constexpr uint16_t TopPaneCenterSlot = ht_meta_id(MetaIdScope::TopPane, 0x02);
constexpr uint16_t TopPaneRightSlot = ht_meta_id(MetaIdScope::TopPane, 0x03);
constexpr uint16_t TopPaneTitle = ht_meta_id(MetaIdScope::TopPane, 0x04);
constexpr uint16_t TopPaneBattery = ht_meta_id(MetaIdScope::TopPane, 0x05);
constexpr uint16_t TopPaneBatteryOutline = ht_meta_id(MetaIdScope::TopPane, 0x06);
constexpr uint16_t TopPaneBatteryFill = ht_meta_id(MetaIdScope::TopPane, 0x07);
constexpr uint16_t TopPaneBatteryCap = ht_meta_id(MetaIdScope::TopPane, 0x08);
} // namespace meta_id

class TopPane
{
  public:
    TopPane() = default;

    bool create(_lv_obj_t *parent = nullptr);

    void setTitle(const char *title);
    void setBatteryStatus(uint16_t millivolts, uint8_t percent);
    _lv_obj_t *root() const
    {
        return _root;
    }
#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
    void enableTouchShell(lv_event_cb_t on_short_press, lv_event_cb_t on_long_press,
                          void *user_data);
#endif

  protected:
    void renderBattery();

  private:
#if defined(HELTEC_V4_R8_TFT)
    static void onBatteryDraw(lv_event_t *e);
#else
    static void onBatteryOutlineSizeChanged(lv_event_t *e);
#endif

    _lv_obj_t *_root = nullptr;
    _lv_obj_t *_title = nullptr;
#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
    _lv_obj_t *_touch_slot = nullptr;
#endif
    _lv_obj_t *_bat_cont = nullptr;
#if !defined(HELTEC_V4_R8_TFT)
    _lv_obj_t *_bat_fill = nullptr;
#endif
    uint8_t _bat_percent = 0;
    bool _bat_present = false;
};

} // namespace heltec::meshcore::ui
