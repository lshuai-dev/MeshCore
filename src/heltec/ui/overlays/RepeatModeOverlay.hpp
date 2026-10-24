#pragma once

#include "ui/core/FramedOverlay.hpp"
#include "ui/widgets/ButtonRoller.hpp"

#include <stddef.h>
#include <stdint.h>

struct _lv_obj_t;

namespace Heltec::MeshCore::Ui
{

class RepeatModeOverlay : public FramedOverlay
{
  public:
    explicit RepeatModeOverlay(Biz::IBizFacade &biz);

#if HELTEC_UI_HAS_TOUCH
    bool hitRoller(int16_t x, int16_t y) const;
#endif

  protected:
    void onEnter() override;
    bool build() override;

  private:
    static constexpr uint8_t kMaxFrequencyItems = 3;
    static constexpr size_t kFrequencyLabelCapacity = 28;

    bool onKey(uint32_t key) override;

    void applySelection(uint8_t item_index);
    void cancelSelection();
    void setApplying(bool applying);
#if HELTEC_UI_HAS_TOUCH
    static void onRollerEvent(lv_event_t *event);
#else
    static void onItemClicked(lv_event_t *event);
#endif

#if HELTEC_UI_HAS_TOUCH
    _lv_obj_t *_touch_roller = nullptr;
    char _roller_options[kMaxFrequencyItems * kFrequencyLabelCapacity]{};
#else
    ButtonRoller _roller;
#endif
    int8_t _frequency_indices[kMaxFrequencyItems]{};
    uint8_t _frequency_count = 0;
    bool _applying = false;
};

} // namespace Heltec::MeshCore::Ui
