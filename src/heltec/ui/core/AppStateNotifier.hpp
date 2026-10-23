#pragma once

#include "AppStateEvent.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

using AppStateSink = void (*)(void *user_data, const AppStateEvent &event);

class AppStateNotifier
{
  public:
    static constexpr uint8_t kMaxSinks = 2;

    void bind(AppStateSink sink, void *user_data);
    void clear();
    void notify(const AppStateEvent &event);

  private:
    struct Binding
    {
        AppStateSink sink = nullptr;
        void *user_data = nullptr;
    };

    Binding _bindings[kMaxSinks] = {};
};

AppStateNotifier &app_state_notifier();

} // namespace Heltec::MeshCore::Ui
