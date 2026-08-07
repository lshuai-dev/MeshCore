#pragma once

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {

constexpr MetaId LicenseGateRoot = ht_meta_id(MetaIdScope::LicenseGate, 0x00);
constexpr MetaId LicenseGateTitle = ht_meta_id(MetaIdScope::LicenseGate, 0x01);
constexpr MetaId LicenseGateChip = ht_meta_id(MetaIdScope::LicenseGate, 0x02);

}  // namespace heltec::meshcore::ui::meta_id
