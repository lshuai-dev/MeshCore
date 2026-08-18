#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id
{

constexpr uint16_t AppOverlayLayer = ht_meta_id(MetaIdScope::App, 0x00);
constexpr uint16_t AppFrameLayout = ht_meta_id(MetaIdScope::App, 0x01);
constexpr uint16_t AppContent = ht_meta_id(MetaIdScope::App, 0x02);
constexpr uint16_t AppScreenRoot = ht_meta_id(MetaIdScope::App, 0x03);
constexpr uint16_t AppTileView = ht_meta_id(MetaIdScope::App, 0x04);
constexpr uint16_t AppTile = ht_meta_id(MetaIdScope::App, 0x05);
constexpr uint16_t AppBackgroundImage = ht_meta_id(MetaIdScope::App, 0x06);

} // namespace heltec::meshcore::ui::meta_id
