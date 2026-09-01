#pragma once
#include <stdint.h>

namespace heltec::meshcore::ui
{

enum class eScreenId : uint8_t
{
    Home,
    Recent,
    Radio,
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    Compass,
    FindFriend,
#endif
    GPS,
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    Tracker,
#endif
    System,
    kScreenCnt,
    None = 0xFF
};

constexpr uint8_t kScreenCnt = static_cast<uint8_t>(eScreenId::kScreenCnt);

} // namespace heltec::meshcore::ui
