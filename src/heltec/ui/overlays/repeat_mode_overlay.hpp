#pragma once

#include "ui/core/framed_overlay.hpp"
#include "ui/widgets/button_roller.hpp"

#include <stddef.h>
#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::ui
{

class RepeatModeOverlay : public FramedOverlay
{
  public:
    explicit RepeatModeOverlay(biz::IBizFacade &biz);

    void onEnter() override;
#if HELTEC_HAS_TOUCH
    bool hitRoller(int16_t x, int16_t y) const;
#endif

  protected:
    bool buildSurface() override;

  private:
    static constexpr uint8_t kMaxFrequencyItems = 3;
    static constexpr size_t kFrequencyLabelCapacity = 28;

    _lv_obj_t *focusTarget() const override;
    bool onKey(uint32_t key) override;

    void applySelection(uint8_t item_index);
    void cancelSelection();
    void setApplying(bool applying);
#if HELTEC_HAS_TOUCH
    static void onRollerEvent(lv_event_t *event);
#else
    static void onItemClicked(lv_event_t *event);
#endif

#if HELTEC_HAS_TOUCH
    _lv_obj_t *_touch_roller = nullptr;
    char _roller_options[kMaxFrequencyItems * kFrequencyLabelCapacity]{};
#else
    ButtonRoller _roller;
#endif
    int8_t _frequency_indices[kMaxFrequencyItems]{};
    uint8_t _frequency_count = 0;
    bool _applying = false;
};

} // namespace heltec::meshcore::ui
