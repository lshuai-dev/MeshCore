#pragma once

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

enum class ScreenId : uint8_t
{
    Home,
    Recent,
    Radio,
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    Compass,
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    FindFriend,
    GPS,
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    Tracker,
#endif
    System,
    Count,
    None = UINT8_MAX
};

constexpr uint8_t kScreenCount = static_cast<uint8_t>(ScreenId::Count);

} // namespace Heltec::MeshCore::Ui
