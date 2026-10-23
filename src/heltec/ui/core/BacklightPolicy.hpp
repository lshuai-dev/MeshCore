#pragma once

#include "InputTypes.hpp"

namespace Heltec::MeshCore::Ui
{

class IUiInputHost;

class BacklightPolicy
{
  public:
    /** Returns true if the raw input was consumed by wake/backlight policy. */
    static bool handle(IUiInputHost &host, const InputEvent &event);

    /** Returns true if the command was consumed by backlight policy. */
    static bool handleCommand(IUiInputHost &host, InputCommand command, uint32_t now_ms);

    static BacklightMode mode();
};

} // namespace Heltec::MeshCore::Ui
