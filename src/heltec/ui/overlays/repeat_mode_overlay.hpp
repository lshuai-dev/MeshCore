#pragma once

#include "ui/core/framed_overlay.hpp"
#include "ui/widgets/button_roller.hpp"

#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::ui
{

class RepeatModeOverlay : public FramedOverlay
{
  public:
    explicit RepeatModeOverlay(biz::IBizFacade &biz) : FramedOverlay(biz)
    {
    }

    void onEnter() override;
#if defined(HELTEC_V4_R8_TFT)
    bool hitRoller(int16_t x, int16_t y) const;
#endif

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    static constexpr uint8_t kMaxFrequencyItems = 3;

    _lv_obj_t *createRoot(_lv_obj_t *parent) override;
    _lv_obj_t *focusTarget() const override;
    bool onKey(uint32_t key) override;

    void applySelection(uint8_t item_index);
    void cancelSelection();
    void setApplying(bool applying);
#if defined(HELTEC_V4_R8_TFT)
    static void onRollerEvent(lv_event_t *event);
#else
    static void onItemClicked(lv_event_t *event);
#endif

#if defined(HELTEC_V4_R8_TFT)
    _lv_obj_t *_touch_roller = nullptr;
    char _roller_options[kMaxFrequencyItems * 28]{};
#else
    ButtonRoller _roller;
    _lv_obj_t *_frequency_buttons[kMaxFrequencyItems]{};
#endif
    int8_t _frequency_indices[kMaxFrequencyItems]{};
    char _frequency_labels[kMaxFrequencyItems][28]{};
    uint8_t _frequency_count = 0;
    bool _applying = false;
};

} // namespace heltec::meshcore::ui
