#pragma once

#include <stdint.h>

#include "ui/core/abstract_overlay.hpp"
#include "ui/widgets/button_roller.hpp"

struct _lv_obj_t;

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RepeatModeOverlayRoot =
    ht_meta_id(MetaIdScope::Overlay, 0x09);
constexpr MetaId RepeatModeTitle = ht_meta_id(MetaIdScope::Overlay, 0xD0);
constexpr MetaId RepeatModeList = ht_meta_id(MetaIdScope::Overlay, 0xD1);
}  // namespace meta_id

class RepeatModeOverlay : public AbstractOverlay {
 public:
  explicit RepeatModeOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  void onEnter() override;
#if defined(HELTEC_V4_R8_TFT)
  bool hitRoller(int16_t x, int16_t y) const;
#endif

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  static constexpr uint8_t kMaxFrequencyItems = 3;

  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;

  void applySelection(uint8_t item_index);
  void cancelSelection();
  void setApplying(bool applying);
#if defined(HELTEC_V4_R8_TFT)
  static void onRollerEvent(lv_event_t* event);
#else
  static void onItemClicked(lv_event_t* event);
#endif

#if defined(HELTEC_V4_R8_TFT)
  _lv_obj_t* _touch_roller = nullptr;
  char _roller_options[kMaxFrequencyItems * 28]{};
#else
  ButtonRoller _roller;
  _lv_obj_t* _frequency_buttons[kMaxFrequencyItems]{};
#endif
  int8_t _frequency_indices[kMaxFrequencyItems]{};
  char _frequency_labels[kMaxFrequencyItems][28]{};
  uint8_t _frequency_count = 0;
  bool _applying = false;
};

}  // namespace heltec::meshcore::ui
