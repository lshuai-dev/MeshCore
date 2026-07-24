#include "ui_deferred_queue.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

namespace {
UiDeferredQueue s_ui_deferred_queue;
}

UiDeferredQueue& ui_deferred_queue() {
  return s_ui_deferred_queue;
}

bool ui_defer(UiDeferredCallback callback, void* user_data) {
  return s_ui_deferred_queue.post(callback, user_data);
}

uint8_t ui_defer_cancel(UiDeferredCallback callback, void* user_data) {
  return s_ui_deferred_queue.cancel(callback, user_data);
}

bool UiDeferredQueue::createTimer() {
  if (_timer) return true;
  _timer = lv_timer_create(timerCallback, 1U, this);
  if (!_timer) return false;
  lv_timer_set_repeat_count(_timer, -1);
  lv_timer_pause(_timer);
  if (_pending_count != 0) armForNextTick();
  return true;
}

bool UiDeferredQueue::post(UiDeferredCallback callback, void* user_data) {
  if (!callback || !_timer || _pending_count >= kCapacity) return false;
  Entry& entry = _pending[_pending_count++];
  entry.callback = callback;
  entry.user_data = user_data;
  if (!_dispatching) armForNextTick();
  return true;
}

uint8_t UiDeferredQueue::removeMatches(Entry* entries, uint8_t count,
                                       UiDeferredCallback callback, void* user_data) {
  uint8_t write = 0;
  for (uint8_t read = 0; read < count; ++read) {
    const Entry& entry = entries[read];
    if (entry.callback == callback && entry.user_data == user_data) continue;
    if (write != read) entries[write] = entry;
    ++write;
  }
  for (uint8_t i = write; i < count; ++i) entries[i] = {};
  return write;
}

uint8_t UiDeferredQueue::cancel(UiDeferredCallback callback, void* user_data) {
  if (!callback) return 0;
  const uint8_t pending_before = _pending_count;
  _pending_count = removeMatches(_pending, _pending_count, callback, user_data);

  uint8_t dispatch_removed = 0;
  if (_dispatching && _dispatch_index < _dispatch_count) {
    for (uint8_t i = _dispatch_index; i < _dispatch_count; ++i) {
      Entry& entry = _dispatch[i];
      if (entry.callback == callback && entry.user_data == user_data) {
        entry = {};
        ++dispatch_removed;
      }
    }
  }

  if (_timer && !_dispatching && _pending_count == 0) lv_timer_pause(_timer);
  return static_cast<uint8_t>((pending_before - _pending_count) + dispatch_removed);
}

void UiDeferredQueue::armForNextTick() {
  if (!_timer || _pending_count == 0) return;
  lv_timer_set_period(_timer, 1U);
  lv_timer_ready(_timer);
  lv_timer_resume(_timer);
}

void UiDeferredQueue::timerCallback(lv_timer_t* timer) {
  auto* queue = timer ? static_cast<UiDeferredQueue*>(timer->user_data) : nullptr;
  if (!queue) return;
  lv_timer_pause(timer);
  queue->dispatch();
}

void UiDeferredQueue::dispatch() {
  if (_dispatching || _pending_count == 0) return;

  _dispatch_count = _pending_count;
  for (uint8_t i = 0; i < _dispatch_count; ++i) {
    _dispatch[i] = _pending[i];
    _pending[i] = {};
  }
  _pending_count = 0;
  _dispatch_index = 0;
  _dispatching = true;

  while (_dispatch_index < _dispatch_count) {
    Entry entry = _dispatch[_dispatch_index];
    _dispatch[_dispatch_index] = {};
    ++_dispatch_index;
    if (entry.callback) entry.callback(entry.user_data);
  }

  _dispatching = false;
  _dispatch_count = 0;
  _dispatch_index = 0;
  if (_pending_count != 0) armForNextTick();
}

}  // namespace heltec::meshcore::ui
