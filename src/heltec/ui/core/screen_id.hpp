#pragma once
#include "ht_meta_data.hpp"

#include <stdint.h>

namespace heltec::meshcore::ui
{

namespace meta_id
{
constexpr uint16_t ScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x00);
constexpr uint16_t HomeScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x01);
constexpr uint16_t GpsScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x02);
constexpr uint16_t RadioScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x03);
constexpr uint16_t RecentScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x04);
constexpr uint16_t CompassScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x05);
constexpr uint16_t FindFriendScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x06);
constexpr uint16_t TrackerScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x07);
constexpr uint16_t SystemRoot = ht_meta_id(MetaIdScope::Screen, 0x08);
constexpr uint16_t ScreenClipLabel = ht_meta_id(MetaIdScope::Screen, 0x09);
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
