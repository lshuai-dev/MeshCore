#pragma once

#include <stdint.h>

struct _lv_timer_t;

namespace heltec::meshcore::ui {

using UiMotionExec = void (*)(void* target, int32_t value);
using UiMotionReady = void (*)(void* user_data);

enum class UiMotionPath : uint8_t {
  Linear = 0,
  EaseIn,
  EaseOut,
  EaseInOut,
};

struct UiMotionSpec {
  void* target = nullptr;
  UiMotionExec exec = nullptr;
  UiMotionReady ready = nullptr;
  void* ready_data = nullptr;
  int32_t start_value = 0;
  int32_t end_value = 0;
  uint16_t duration_ms = 0;
  UiMotionPath path = UiMotionPath::Linear;
};

/** Fixed-slot animation runner that avoids lv_anim_start() allocations. */
class UiMotionScheduler {
 public:
  static constexpr uint8_t kCapacity = 16;

  bool createTimer();
  bool start(const UiMotionSpec& spec);
  uint8_t cancel(void* target, UiMotionExec exec = nullptr);
  bool active(void* target, UiMotionExec exec = nullptr) const;

 private:
  struct Slot {
    UiMotionSpec spec{};
    uint32_t started_ms = 0;
    uint16_t generation = 0;
    bool active = false;
  };

  static void timerCallback(_lv_timer_t* timer);
  void tick();
  void arm();
  static int32_t interpolate(const Slot& slot, uint32_t elapsed_ms);

  Slot _slots[kCapacity]{};
  _lv_timer_t* _timer = nullptr;
};

UiMotionScheduler& ui_motion_scheduler();
bool ui_motion_start(const UiMotionSpec& spec);
uint8_t ui_motion_cancel(void* target, UiMotionExec exec = nullptr);
bool ui_motion_active(void* target, UiMotionExec exec = nullptr);

}  // namespace heltec::meshcore::ui
