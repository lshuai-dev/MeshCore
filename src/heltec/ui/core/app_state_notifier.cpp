#include "app_state_notifier.hpp"

namespace heltec::meshcore::ui {

AppStateNotifier& app_state_notifier() {
  static AppStateNotifier notifier;
  return notifier;
}

bool AppStateNotifier::addObserver(AppStateObserver* observer) {
  if (!observer) return false;
  for (uint8_t i = 0; i < _count; ++i) {
    if (_observers[i] == observer) return true;
  }
  if (_count >= kMaxObservers) return false;
  _observers[_count++] = observer;
  return true;
}

void AppStateNotifier::removeObserver(AppStateObserver* observer) {
  if (!observer) return;
  for (uint8_t i = 0; i < _count; ++i) {
    if (_observers[i] != observer) continue;
    _observers[i] = _observers[_count - 1];
    _observers[_count - 1] = nullptr;
    --_count;
    return;
  }
}

void AppStateNotifier::notify(const AppStateEvent& event) {
  AppStateObserver* snapshot[kMaxObservers] = {};
  const uint8_t n = _count;
  for (uint8_t i = 0; i < n; ++i) snapshot[i] = _observers[i];
  for (uint8_t i = 0; i < n; ++i) {
    if (snapshot[i]) snapshot[i]->onAppStateChanged(event);
  }
}

}  // namespace heltec::meshcore::ui
