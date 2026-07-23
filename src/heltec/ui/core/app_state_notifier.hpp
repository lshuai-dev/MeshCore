#pragma once

#include <stdint.h>

#include "app_state_event.hpp"

namespace heltec::meshcore::ui {

class AppStateObserver {
 public:
  virtual ~AppStateObserver() = default;
  virtual void onAppStateChanged(const AppStateEvent& event) = 0;
};

class AppStateNotifier {
 public:
  static constexpr uint8_t kMaxObservers = 12;

  bool addObserver(AppStateObserver* observer);
  void removeObserver(AppStateObserver* observer);
  void notify(const AppStateEvent& event);
  uint8_t observerCount() const { return _count; }

 private:
  AppStateObserver* _observers[kMaxObservers] = {};
  uint8_t _count = 0;
};

AppStateNotifier& app_state_notifier();

}  // namespace heltec::meshcore::ui
