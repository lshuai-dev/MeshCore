#pragma once

#include "InputTypes.hpp"

namespace Heltec::MeshCore::Ui
{

class InputPipeline
{
  public:
    static void init();

    /** GPIO gesture path (momentary_button). */
    static void onButtonGesture(uint8_t slot, ButtonGesture gesture, uint32_t now_ms);

    /** Synthetic LV_KEY path (touch gestures or other virtual inputs). */
    static void onSyntheticKey(uint32_t lv_key, bool pulse);
};

} // namespace Heltec::MeshCore::Ui
