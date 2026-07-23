#pragma once

#include "../core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {

constexpr MetaId SendMessageOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x06);
constexpr MetaId SendMessageTitle = ht_meta_id(MetaIdScope::Overlay, 0xC0);
constexpr MetaId SendMessageList = ht_meta_id(MetaIdScope::Overlay, 0xC1);
constexpr MetaId SendMessageTouchList = ht_meta_id(MetaIdScope::Overlay, 0xC2);
constexpr MetaId SendMessageRow = ht_meta_id(MetaIdScope::Overlay, 0xC3);
constexpr MetaId SendMessageFooter = ht_meta_id(MetaIdScope::Overlay, 0xC4);
constexpr MetaId SendMessageRowLabel = ht_meta_id(MetaIdScope::Overlay, 0xC8);

}  // namespace heltec::meshcore::ui::meta_id
