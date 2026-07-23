#include "app_state_ui_dispatcher.hpp"

#include "surface_manager.hpp"
#include "ui_events.h"

#include <lvgl.h>

namespace heltec::meshcore::ui {

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

void AppStateUiDispatcher::onAppStateChanged(const AppStateEvent& event) {
  const uint8_t idx = eventIndex(event.type);
  if (idx >= kEventTypeCount) return;
  _latest[idx] = event;
  _pending_mask |= static_cast<uint16_t>(1U << idx);
  scheduleDispatch();
}

void AppStateUiDispatcher::scheduleDispatch() {
  if ((!_surfaces && !_global_handler) || !hasPending() || _dispatch_scheduled) return;
  if (LV_RES_OK != lv_async_call([](void* user_data) {
        auto* dispatcher = static_cast<AppStateUiDispatcher*>(user_data);
        if (!dispatcher) return;
        dispatcher->_dispatch_scheduled = false;
        dispatcher->dispatchPending();
        dispatcher->scheduleDispatch();
      }, this)) {
    return;
  }
  _dispatch_scheduled = true;
}

void AppStateUiDispatcher::dispatchPending() {
  if (!_surfaces && !_global_handler) return;

  const uint16_t mask = _pending_mask;
  if (mask == 0) return;

  AppStateEvent pending[kEventTypeCount]{};
  for (uint8_t i = 0; i < kEventTypeCount; ++i) {
    if ((mask & static_cast<uint16_t>(1U << i)) == 0) continue;
    pending[i] = _latest[i];
  }
  _pending_mask = 0;

  for (uint8_t i = 0; i < kEventTypeCount; ++i) {
    if ((mask & static_cast<uint16_t>(1U << i)) == 0) continue;
    AppStateEvent event = pending[i];
    if (_global_handler) {
      _global_handler(_global_handler_user_data, event);
    }
    if (_surfaces) {
      _surfaces->dispatchEventToActive(UiEventType::AppStateChanged, &event);
    }
  }
}

}  // namespace heltec::meshcore::ui
