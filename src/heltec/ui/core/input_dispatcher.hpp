#pragma once

#include "input_types.hpp"

namespace heltec::meshcore::ui {

class InputHost;

class InputDispatcher {
 public:
  /** Returns true if the command was consumed. */
  static bool dispatch(InputHost& host, InputCommand command, uint32_t now_ms);
};

}  // namespace heltec::meshcore::ui
