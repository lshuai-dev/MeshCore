#pragma once

#include <stdint.h>
#include <lvgl.h>

namespace heltec::meshcore::ui {

class InputHost {
 public:
  virtual ~InputHost() = default;

  virtual bool isReady() const = 0;
  virtual bool isDisplayOn() const = 0;
  virtual void toggleDisplay(uint32_t now_ms) = 0;
  virtual void onBacklightTurnedOn() = 0;
  virtual void notifyDisplayActivity(uint32_t now_ms) = 0;
  virtual void reconcileInput() = 0;
  virtual void ensureTileKeypadFocus() = 0;
  virtual lv_obj_t* frameRoot() const = 0;
};

}  // namespace heltec::meshcore::ui
