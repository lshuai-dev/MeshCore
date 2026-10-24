#pragma once

#include "heltec/drivers/input/MomentaryButton.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

class InputRouter
{
  public:
    static void init();

    static void onButtonGesture(
        uint8_t slot, Heltec::MeshCore::Dal::MomentaryButton::ButtonGesture gesture,
        uint32_t now_ms);

    static void onSyntheticKey(uint32_t lv_key);
};

} // namespace Heltec::MeshCore::Ui
