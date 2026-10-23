#include "AppStateNotifier.hpp"

namespace Heltec::MeshCore::Ui
{

AppStateNotifier &app_state_notifier()
{
    static AppStateNotifier notifier;
    return notifier;
}

void AppStateNotifier::bind(AppStateSink sink, void *user_data)
{
    if (!sink)
    {
        return;
    }
    for (Binding &binding : _bindings)
    {
        if (binding.sink == sink && binding.user_data == user_data)
        {
            return;
        }
        if (!binding.sink)
        {
            binding.sink = sink;
            binding.user_data = user_data;
            return;
        }
    }
}

void AppStateNotifier::clear()
{
    for (Binding &binding : _bindings)
    {
        binding = Binding{};
    }
}

void AppStateNotifier::notify(const AppStateEvent &event)
{
    for (const Binding &binding : _bindings)
    {
        if (binding.sink)
            binding.sink(binding.user_data, event);
    }
}

} // namespace Heltec::MeshCore::Ui
