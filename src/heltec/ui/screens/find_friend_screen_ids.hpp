#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id
{

constexpr uint16_t FindFriendDialRow = ht_meta_id(MetaIdScope::Screen, 0xC0);
constexpr uint16_t FindFriendSettingRow = ht_meta_id(MetaIdScope::Screen, 0xC1);
constexpr uint16_t FindFriendSettingLabel = ht_meta_id(MetaIdScope::Screen, 0xC2);
constexpr uint16_t FindFriendSwitch = ht_meta_id(MetaIdScope::Screen, 0xC3);
constexpr uint16_t FindFriendDropdown = ht_meta_id(MetaIdScope::Screen, 0xC4);
constexpr uint16_t FindFriendActionRow = ht_meta_id(MetaIdScope::Screen, 0xC5);
constexpr uint16_t FindFriendActionLabel = ht_meta_id(MetaIdScope::Screen, 0xC6);
constexpr uint16_t FindFriendDropdownList = ht_meta_id(MetaIdScope::Screen, 0xC7);

} // namespace heltec::meshcore::ui::meta_id
