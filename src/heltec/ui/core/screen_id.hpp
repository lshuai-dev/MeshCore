#pragma once
#include "ht_meta_data.hpp"

#include <stdint.h>

namespace heltec::meshcore::ui
{

namespace meta_id
{
constexpr uint16_t HomeScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x01);
constexpr uint16_t TrackerScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x07);
} // namespace meta_id

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
