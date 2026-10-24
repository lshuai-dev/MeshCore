#pragma once

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

enum class AppStateEventType : uint8_t
{
    GpsChanged,
    CompassChanged,
    CompanionChanged,
    UnreadMessageCountChanged,

    RadioChanged,
    RecentHeardChanged,
    ContactLocationChanged,
    FindFriendChanged,
    TeamChanged,
    ConfigChanged,
    PreviewChanged,
    AlertChanged,
    Count,
};

struct AppStateEvent
{
    // 事件只表示领域状态已失效，接收方应从权威状态源读取最新快照。
    AppStateEventType type = AppStateEventType::ConfigChanged;
};

} // namespace Heltec::MeshCore::Ui
