#pragma once

#include "input_types.hpp"

namespace heltec::meshcore::ui {

class InputHost;

class BacklightPolicy {
 public:
  /** Returns true if the raw input was consumed by wake/backlight policy. */
  static bool handle(InputHost& host, const InputEvent& event);

  /** Returns true if the command was consumed by backlight policy. */
  static bool handleCommand(InputHost& host, InputCommand command, uint32_t now_ms);

  static BacklightMode mode();
};

}  // namespace heltec::meshcore::ui
