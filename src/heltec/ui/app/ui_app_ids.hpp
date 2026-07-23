#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {

constexpr MetaId AppOverlayLayer = ht_meta_id(MetaIdScope::App, 0x00);
constexpr MetaId AppFrameLayout = ht_meta_id(MetaIdScope::App, 0x01);
constexpr MetaId AppContent = ht_meta_id(MetaIdScope::App, 0x02);
constexpr MetaId AppScreenRoot = ht_meta_id(MetaIdScope::App, 0x03);
constexpr MetaId AppTileView = ht_meta_id(MetaIdScope::App, 0x04);
constexpr MetaId AppTile = ht_meta_id(MetaIdScope::App, 0x05);
constexpr MetaId AppBackgroundImage = ht_meta_id(MetaIdScope::App, 0x06);

}  // namespace heltec::meshcore::ui::meta_id
