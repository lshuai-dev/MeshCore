#pragma once

#include <stdint.h>

struct _lv_timer_t;

namespace heltec::meshcore::ui {

using UiDeferredCallback = void (*)(void* user_data);

/**
 * Fixed-capacity replacement for lv_async_call().
 *
 * The queue and its single LVGL timer are created during UI startup and then
 * reused for the lifetime of the application, avoiding runtime timer
 * allocation/free cycles.
 */
class UiDeferredQueue {
 public:
  static constexpr uint8_t kCapacity = 16;

  bool createTimer();
  bool post(UiDeferredCallback callback, void* user_data);
  uint8_t cancel(UiDeferredCallback callback, void* user_data);
  bool hasPending() const { return _pending_count != 0 || _dispatch_count != 0; }

 private:
  struct Entry {
    UiDeferredCallback callback = nullptr;
    void* user_data = nullptr;
  };

  static void timerCallback(_lv_timer_t* timer);
  void dispatch();
  void armForNextTick();
  static uint8_t removeMatches(Entry* entries, uint8_t count,
                               UiDeferredCallback callback, void* user_data);

  Entry _pending[kCapacity]{};
  Entry _dispatch[kCapacity]{};
  _lv_timer_t* _timer = nullptr;
  uint8_t _pending_count = 0;
  uint8_t _dispatch_count = 0;
  uint8_t _dispatch_index = 0;
  bool _dispatching = false;
};

UiDeferredQueue& ui_deferred_queue();
bool ui_defer(UiDeferredCallback callback, void* user_data);
uint8_t ui_defer_cancel(UiDeferredCallback callback, void* user_data);

}  // namespace heltec::meshcore::ui
