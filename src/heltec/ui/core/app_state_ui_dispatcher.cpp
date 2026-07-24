#include "app_state_ui_dispatcher.hpp"

#include "app_state_notifier.hpp"
#include "surface_manager.hpp"
#include "ui_events.h"

#include <lvgl.h>

namespace heltec::meshcore::ui {

void AppStateUiDispatcher::bindNotifier(AppStateNotifier& notifier) {
  notifier.bind(
      +[](void* user_data, const AppStateEvent& event) {
        auto* dispatcher = static_cast<AppStateUiDispatcher*>(user_data);
        if (dispatcher) dispatcher->onAppStateChanged(event);
      },
      this);
}

uint8_t AppStateUiDispatcher::eventIndex(AppStateEventType type) {
  return static_cast<uint8_t>(type);
}

void AppStateUiDispatcher::bindSurfaceManager(SurfaceManager& surfaces) {
  _surfaces = &surfaces;
  scheduleDispatch();
}

void AppStateUiDispatcher::bindGlobalHandler(GlobalHandler handler, void* user_data) {
  _global_handler = handler;
  _global_handler_user_data = user_data;
  scheduleDispatch();
}

bool AppStateUiDispatcher::createTimer() {
  if (_dispatch_timer) return true;
  _dispatch_timer = lv_timer_create(dispatchTimerCallback, 1U, this);
  if (!_dispatch_timer) return false;
  lv_timer_set_repeat_count(_dispatch_timer, -1);
  lv_timer_pause(_dispatch_timer);
  scheduleDispatch();
  return true;
}

void AppStateUiDispatcher::onAppStateChanged(const AppStateEvent& event) {
  const uint8_t idx = eventIndex(event.type);
  if (idx >= kEventTypeCount) return;
  _latest[idx] = event;
  _pending_mask |= static_cast<uint16_t>(1U << idx);
  scheduleDispatch();
}

void AppStateUiDispatcher::scheduleDispatch() {
  if ((!_surfaces && !_global_handler) || !hasPending()) return;
  if (!_dispatch_timer) return;
  lv_timer_resume(_dispatch_timer);
  lv_timer_ready(_dispatch_timer);
}

void AppStateUiDispatcher::dispatchTimerCallback(lv_timer_t* timer) {
  auto* dispatcher = timer
                         ? static_cast<AppStateUiDispatcher*>(timer->user_data)
                         : nullptr;
  if (!dispatcher) return;
  dispatcher->dispatchPending();
  if (!dispatcher->hasPending()) lv_timer_pause(timer);
}

void AppStateUiDispatcher::dispatchPending() {
  if (!_surfaces && !_global_handler) return;

  const uint16_t mask = _pending_mask;
  if (mask == 0) return;

  for (uint8_t i = 0; i < kEventTypeCount; ++i) {
    const uint16_t bit = static_cast<uint16_t>(1U << i);
    if ((mask & bit) == 0) continue;
    const AppStateEvent event = _latest[i];
    _pending_mask &= static_cast<uint16_t>(~bit);
    if (_global_handler) {
      _global_handler(_global_handler_user_data, event);
    }
    if (_surfaces) {
      _surfaces->dispatchEventToActive(UiEventType::AppStateChanged, &event);
    }
  }
}

}  // namespace heltec::meshcore::ui
