#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id
{

constexpr uint16_t LicenseGateRoot = ht_meta_id(MetaIdScope::LicenseGate, 0x00);
constexpr uint16_t LicenseGateTitle = ht_meta_id(MetaIdScope::LicenseGate, 0x01);
constexpr uint16_t LicenseGateChip = ht_meta_id(MetaIdScope::LicenseGate, 0x02);

} // namespace heltec::meshcore::ui::meta_id
