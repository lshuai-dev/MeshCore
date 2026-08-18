#pragma once

#include "../core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id
{

constexpr uint16_t SendMessageOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x06);
constexpr uint16_t SendMessageTitle = ht_meta_id(MetaIdScope::Overlay, 0xC0);
constexpr uint16_t SendMessageList = ht_meta_id(MetaIdScope::Overlay, 0xC1);
constexpr uint16_t SendMessageTouchList = ht_meta_id(MetaIdScope::Overlay, 0xC2);
constexpr uint16_t SendMessageRow = ht_meta_id(MetaIdScope::Overlay, 0xC3);
constexpr uint16_t SendMessageFooter = ht_meta_id(MetaIdScope::Overlay, 0xC4);
constexpr uint16_t SendMessageRowLabel = ht_meta_id(MetaIdScope::Overlay, 0xC8);

} // namespace heltec::meshcore::ui::meta_id
