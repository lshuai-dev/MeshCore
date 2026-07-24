#include "ui_motion_scheduler.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

namespace {
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

  static void timerCallback(lv_timer_t* timer);
  void tick();
  void arm();
  static int32_t interpolate(const Slot& slot, uint32_t elapsed_ms);

  Slot _slots[kCapacity]{};
  lv_timer_t* _timer = nullptr;
};

UiMotionScheduler s_ui_motion_scheduler;
}

bool ui_motion_init() { return s_ui_motion_scheduler.createTimer(); }

bool ui_motion_start(const UiMotionSpec& spec) {
  return s_ui_motion_scheduler.start(spec);
}

uint8_t ui_motion_cancel(void* target, UiMotionExec exec) {
  return s_ui_motion_scheduler.cancel(target, exec);
}

bool ui_motion_active(void* target, UiMotionExec exec) {
  return s_ui_motion_scheduler.active(target, exec);
}

bool UiMotionScheduler::createTimer() {
  if (_timer) return true;
  _timer = lv_timer_create(timerCallback, 1U, this);
  if (!_timer) return false;
  lv_timer_set_repeat_count(_timer, -1);
  lv_timer_pause(_timer);
  return true;
}

bool UiMotionScheduler::start(const UiMotionSpec& spec) {
  if (!_timer || !spec.target || !spec.exec) return false;

  Slot* selected = nullptr;
  for (Slot& slot : _slots) {
    if (slot.active && slot.spec.target == spec.target && slot.spec.exec == spec.exec) {
      selected = &slot;
      break;
    }
  }
  if (!selected) {
    for (Slot& slot : _slots) {
      if (!slot.active) {
        selected = &slot;
        break;
      }
    }
  }
  if (!selected) return false;

  ++selected->generation;
  selected->spec = spec;
  selected->started_ms = lv_tick_get();
  selected->active = true;
  spec.exec(spec.target, spec.start_value);

  if (spec.duration_ms == 0) {
    selected->active = false;
    spec.exec(spec.target, spec.end_value);
    if (spec.ready) spec.ready(spec.ready_data);
    return true;
  }

  arm();
  return true;
}

uint8_t UiMotionScheduler::cancel(void* target, UiMotionExec exec) {
  if (!target) return 0;
  uint8_t cancelled = 0;
  for (Slot& slot : _slots) {
    if (!slot.active || slot.spec.target != target) continue;
    if (exec && slot.spec.exec != exec) continue;
    slot.active = false;
    ++slot.generation;
    ++cancelled;
  }
  return cancelled;
}

bool UiMotionScheduler::active(void* target, UiMotionExec exec) const {
  if (!target) return false;
  for (const Slot& slot : _slots) {
    if (!slot.active || slot.spec.target != target) continue;
    if (!exec || slot.spec.exec == exec) return true;
  }
  return false;
}

void UiMotionScheduler::arm() {
  if (!_timer) return;
  lv_timer_set_period(_timer, 1U);
  lv_timer_ready(_timer);
  lv_timer_resume(_timer);
}

void UiMotionScheduler::timerCallback(lv_timer_t* timer) {
  auto* scheduler = timer ? static_cast<UiMotionScheduler*>(timer->user_data) : nullptr;
  if (scheduler) scheduler->tick();
}

int32_t UiMotionScheduler::interpolate(const Slot& slot, uint32_t elapsed_ms) {
  lv_anim_t descriptor;
  lv_anim_init(&descriptor);
  descriptor.start_value = slot.spec.start_value;
  descriptor.end_value = slot.spec.end_value;
  descriptor.time = slot.spec.duration_ms;
  descriptor.act_time = static_cast<int32_t>(elapsed_ms);

  switch (slot.spec.path) {
    case UiMotionPath::EaseIn:
      return lv_anim_path_ease_in(&descriptor);
    case UiMotionPath::EaseOut:
      return lv_anim_path_ease_out(&descriptor);
    case UiMotionPath::EaseInOut:
      return lv_anim_path_ease_in_out(&descriptor);
    case UiMotionPath::Linear:
    default:
      return lv_anim_path_linear(&descriptor);
  }
}

void UiMotionScheduler::tick() {
  const uint32_t now_ms = lv_tick_get();
  bool any_active = false;

  for (Slot& slot : _slots) {
    if (!slot.active) continue;
    const uint16_t generation = slot.generation;
    const uint32_t elapsed = now_ms - slot.started_ms;
    const bool complete = elapsed >= slot.spec.duration_ms;
    const int32_t value = complete ? slot.spec.end_value : interpolate(slot, elapsed);
    const UiMotionExec exec = slot.spec.exec;
    void* const target = slot.spec.target;
    exec(target, value);

    if (!slot.active || slot.generation != generation) continue;
    if (complete) {
      const UiMotionReady ready = slot.spec.ready;
      void* const ready_data = slot.spec.ready_data;
      slot.active = false;
      ++slot.generation;
      if (ready) ready(ready_data);
    } else {
      any_active = true;
    }
  }

  if (!any_active) {
    for (const Slot& slot : _slots) {
      if (slot.active) {
        any_active = true;
        break;
      }
    }
  }
  if (!any_active && _timer) lv_timer_pause(_timer);
}

}  // namespace heltec::meshcore::ui
