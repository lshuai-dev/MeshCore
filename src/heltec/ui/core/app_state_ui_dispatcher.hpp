#pragma once

#include <stdint.h>

#include "app_state_event.hpp"
#include "app_state_notifier.hpp"

namespace heltec::meshcore::ui {

class SurfaceManager;

class AppStateUiDispatcher final : public AppStateObserver {
 public:
  using GlobalHandler = void (*)(void* user_data, const AppStateEvent& event);

  void bindSurfaceManager(SurfaceManager& surfaces);
  void bindGlobalHandler(GlobalHandler handler, void* user_data);
  void onAppStateChanged(const AppStateEvent& event) override;
  bool hasPending() const { return _pending_mask != 0; }

 private:
  static constexpr uint8_t kEventTypeCount =
      static_cast<uint8_t>(AppStateEventType::Count);
  static_assert(kEventTypeCount <= 16, "AppState pending mask is too small");

  static uint8_t eventIndex(AppStateEventType type);
  void scheduleDispatch();
  void dispatchPending();

  AppStateEvent _latest[kEventTypeCount]{};
  SurfaceManager* _surfaces = nullptr;
  GlobalHandler _global_handler = nullptr;
  void* _global_handler_user_data = nullptr;
  uint16_t _pending_mask = 0;
  bool _dispatch_scheduled = false;
};

}  // namespace heltec::meshcore::ui
