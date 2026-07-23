#pragma once
#include <stdint.h>

#include "ht_meta_data.hpp"

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId ScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x00);
constexpr MetaId HomeScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x01);
constexpr MetaId GpsScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x02);
constexpr MetaId RadioScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x03);
constexpr MetaId RecentScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x04);
constexpr MetaId CompassScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x05);
constexpr MetaId FindFriendScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x06);
constexpr MetaId TrackerScreenRoot = ht_meta_id(MetaIdScope::Screen, 0x07);
constexpr MetaId SystemRoot = ht_meta_id(MetaIdScope::Screen, 0x08);
constexpr MetaId ScreenClipLabel = ht_meta_id(MetaIdScope::Screen, 0x09);
}

enum class eScreenId : uint8_t {
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

}  // namespace heltec::meshcore::ui
