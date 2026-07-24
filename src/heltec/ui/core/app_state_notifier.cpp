#include "app_state_notifier.hpp"

namespace heltec::meshcore::ui {

AppStateNotifier& app_state_notifier() {
  static AppStateNotifier notifier;
  return notifier;
}

void AppStateNotifier::bind(AppStateSink sink, void* user_data) {
  _sink = sink;
  _user_data = user_data;
}

void AppStateNotifier::clear() {
  _sink = nullptr;
  _user_data = nullptr;
}

void AppStateNotifier::notify(const AppStateEvent& event) {
  if (_sink) _sink(_user_data, event);
}

}  // namespace heltec::meshcore::ui
