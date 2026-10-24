#pragma once

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

class UiApp;

enum class BacklightMode : uint8_t
{
    AutoTimeout = 0,
    ManualToggle,
};

class BacklightPolicy
{
  public:
    static bool handle(UiApp &app, uint32_t now_ms);

    static BacklightMode mode();
};

} // namespace Heltec::MeshCore::Ui
