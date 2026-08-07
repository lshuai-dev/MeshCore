#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {

constexpr MetaId FindFriendDialRow = ht_meta_id(MetaIdScope::Screen, 0xC0);
constexpr MetaId FindFriendSettingRow = ht_meta_id(MetaIdScope::Screen, 0xC1);
constexpr MetaId FindFriendSettingLabel = ht_meta_id(MetaIdScope::Screen, 0xC2);
constexpr MetaId FindFriendSwitch = ht_meta_id(MetaIdScope::Screen, 0xC3);
constexpr MetaId FindFriendDropdown = ht_meta_id(MetaIdScope::Screen, 0xC4);
constexpr MetaId FindFriendActionRow = ht_meta_id(MetaIdScope::Screen, 0xC5);
constexpr MetaId FindFriendActionLabel = ht_meta_id(MetaIdScope::Screen, 0xC6);
constexpr MetaId FindFriendDropdownList = ht_meta_id(MetaIdScope::Screen, 0xC7);

}  // namespace heltec::meshcore::ui::meta_id
