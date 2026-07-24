#pragma once

#include <stdint.h>

#include "app_state_event.hpp"

namespace heltec::meshcore::ui {

using AppStateSink = void (*)(void* user_data, const AppStateEvent& event);

class AppStateNotifier {
 public:
  void bind(AppStateSink sink, void* user_data);
  void clear();
  void notify(const AppStateEvent& event);

 private:
  AppStateSink _sink = nullptr;
  void* _user_data = nullptr;
};

AppStateNotifier& app_state_notifier();

}  // namespace heltec::meshcore::ui
