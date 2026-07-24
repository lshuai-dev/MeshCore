#pragma once

#include <stdint.h>

#include "app_state_event.hpp"
#include "app_state_notifier.hpp"

struct _lv_timer_t;

namespace heltec::meshcore::ui {

class SurfaceManager;

class AppStateUiDispatcher final : public AppStateObserver {
 public:
  using GlobalHandler = void (*)(void* user_data, const AppStateEvent& event);

  void bindSurfaceManager(SurfaceManager& surfaces);
  void bindGlobalHandler(GlobalHandler handler, void* user_data);
  /** Allocate the single dispatch timer during UI startup. */
  bool createTimer();
  void onAppStateChanged(const AppStateEvent& event) override;
  bool hasPending() const { return _pending_mask != 0; }

 private:
  static constexpr uint8_t kEventTypeCount =
      static_cast<uint8_t>(AppStateEventType::Count);
  static_assert(kEventTypeCount <= 16, "AppState pending mask is too small");

  static uint8_t eventIndex(AppStateEventType type);
  static void dispatchTimerCallback(_lv_timer_t* timer);
  void scheduleDispatch();
  void dispatchPending();

  AppStateEvent _latest[kEventTypeCount]{};
  SurfaceManager* _surfaces = nullptr;
  GlobalHandler _global_handler = nullptr;
  void* _global_handler_user_data = nullptr;
  uint16_t _pending_mask = 0;
  _lv_timer_t* _dispatch_timer = nullptr;
};

}  // namespace heltec::meshcore::ui
