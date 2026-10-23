#pragma once

#include "InputTypes.hpp"

namespace Heltec::MeshCore::Ui
{

class IUiInputHost;

class InputDispatcher
{
  public:
    /** Returns true if the command was consumed. */
    static bool dispatch(IUiInputHost &host, InputCommand command, uint32_t now_ms);
};

} // namespace Heltec::MeshCore::Ui
