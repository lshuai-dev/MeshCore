#pragma once
#include "ui/app/theme_profile.hpp"
#include "ui/core/widget_classes.hpp"

#include <lvgl.h>
#include <stdint.h>

struct _lv_obj_t;
namespace heltec::meshcore::ui
{

class TopPane
{
  public:
    TopPane() = default;

    bool create(_lv_obj_t *parent = nullptr);

    void setTitle(const char *title);
    void setTime(uint32_t epoch_seconds);
    void setBatteryStatus(uint16_t millivolts, uint8_t percent);
    _lv_obj_t *root() const
    {
        return _root;
    }
  protected:
    void renderBattery();

  private:
#if !HELTEC_UI_DISPLAY_MONO
    static void onBatteryDraw(lv_event_t *e);
#else
    static void onBatteryOutlineSizeChanged(lv_event_t *e);
#endif

    _lv_obj_t *_root = nullptr;
    _lv_obj_t *_time = nullptr;
    _lv_obj_t *_title = nullptr;
    _lv_obj_t *_bat_percent_label = nullptr;
    _lv_obj_t *_bat_cont = nullptr;
#if HELTEC_UI_DISPLAY_MONO
    _lv_obj_t *_bat_fill = nullptr;
#endif
    uint8_t _bat_percent = 0;
    bool _bat_present = false;
    char _time_text[6] = { '-', '-', ':', '-', '-', '\0' };
    char _battery_text[5] = {};
};

} // namespace heltec::meshcore::ui
