#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {

constexpr MetaId MapViewport = ht_meta_id(MetaIdScope::Map, 0x00);
constexpr MetaId MapStatusLabel = ht_meta_id(MetaIdScope::Map, 0x01);
constexpr MetaId MapToolbar = ht_meta_id(MetaIdScope::Map, 0x02);
constexpr MetaId MapToolbarButton = ht_meta_id(MetaIdScope::Map, 0x03);
constexpr MetaId MapToolbarButtonLabel = ht_meta_id(MetaIdScope::Map, 0x04);
constexpr MetaId MapTileLayer = ht_meta_id(MetaIdScope::Map, 0x05);
constexpr MetaId MapRangeLayer = ht_meta_id(MetaIdScope::Map, 0x06);
constexpr MetaId MapMarkerLayer = ht_meta_id(MetaIdScope::Map, 0x07);
constexpr MetaId MapTile = ht_meta_id(MetaIdScope::Map, 0x08);
constexpr MetaId MapTilePlaceholder = ht_meta_id(MetaIdScope::Map, 0x09);
constexpr MetaId MapMarker = ht_meta_id(MetaIdScope::Map, 0x0A);
constexpr MetaId MapMarkerLabel = ht_meta_id(MetaIdScope::Map, 0x0B);
constexpr MetaId MapRangeRing = ht_meta_id(MetaIdScope::Map, 0x0C);
constexpr MetaId MapRangeLabel = ht_meta_id(MetaIdScope::Map, 0x0D);
constexpr MetaId MapTileImage = ht_meta_id(MetaIdScope::Map, 0x20);

}  // namespace heltec::meshcore::ui::meta_id
