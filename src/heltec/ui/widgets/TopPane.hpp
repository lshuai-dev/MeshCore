#pragma once
#include "ui/widgets/SemanticClasses.hpp"

#include <lvgl.h>
#include <stdint.h>

struct _lv_obj_t;
namespace Heltec::MeshCore::Ui
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
#if !LV_USE_THEME_MONO
    static void onBatteryDraw(lv_event_t *e);
#else
    static void onBatteryOutlineSizeChanged(lv_event_t *e);
#endif

    _lv_obj_t *_root = nullptr;
    _lv_obj_t *_time = nullptr;
    _lv_obj_t *_title = nullptr;
    _lv_obj_t *_bat_percent_label = nullptr;
    _lv_obj_t *_bat_cont = nullptr;
#if LV_USE_THEME_MONO
    _lv_obj_t *_bat_fill = nullptr;
#endif
    uint8_t _bat_percent = 0;
    bool _bat_present = false;
    char _time_text[6] = { '-', '-', ':', '-', '-', '\0' };
    char _battery_text[5] = {};
};

} // namespace Heltec::MeshCore::Ui
