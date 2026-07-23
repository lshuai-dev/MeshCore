#pragma once

#include "input_types.hpp"

namespace heltec::meshcore::ui {

class InputAdapter {
 public:
  static const InputAdapter& active();

  AdaptedInput adapt(const InputEvent& event) const;

 private:
  AdaptedInput adaptButton(const InputEvent& event) const;
  AdaptedInput adaptKeyboard(const InputEvent& event) const;
};

}  // namespace heltec::meshcore::ui
