#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id
{

constexpr uint16_t MapViewport = ht_meta_id(MetaIdScope::Map, 0x00);
constexpr uint16_t MapStatusLabel = ht_meta_id(MetaIdScope::Map, 0x01);
constexpr uint16_t MapToolbar = ht_meta_id(MetaIdScope::Map, 0x02);
constexpr uint16_t MapToolbarButton = ht_meta_id(MetaIdScope::Map, 0x03);
constexpr uint16_t MapToolbarButtonLabel = ht_meta_id(MetaIdScope::Map, 0x04);
constexpr uint16_t MapTileLayer = ht_meta_id(MetaIdScope::Map, 0x05);
constexpr uint16_t MapRangeLayer = ht_meta_id(MetaIdScope::Map, 0x06);
constexpr uint16_t MapMarkerLayer = ht_meta_id(MetaIdScope::Map, 0x07);
constexpr uint16_t MapTile = ht_meta_id(MetaIdScope::Map, 0x08);
constexpr uint16_t MapTilePlaceholder = ht_meta_id(MetaIdScope::Map, 0x09);
constexpr uint16_t MapMarker = ht_meta_id(MetaIdScope::Map, 0x0A);
constexpr uint16_t MapMarkerLabel = ht_meta_id(MetaIdScope::Map, 0x0B);
constexpr uint16_t MapRangeRing = ht_meta_id(MetaIdScope::Map, 0x0C);
constexpr uint16_t MapRangeLabel = ht_meta_id(MetaIdScope::Map, 0x0D);
constexpr uint16_t MapTileImage = ht_meta_id(MetaIdScope::Map, 0x20);

} // namespace heltec::meshcore::ui::meta_id
