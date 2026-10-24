#pragma once

/**
 * @file KeyInput.hpp
 * @brief Queued logical key delivery to the active LVGL group.
 */

#include <stdint.h>

struct _lv_group_t;

namespace Heltec::MeshCore::Dal::KeyInput
{

void setGroup(_lv_group_t *group);
/** Queue one completed logical key for delivery after the current LVGL event. */
bool postKey(uint32_t key);
/** True only while a queued logical key is being delivered. */
bool isDispatching();

} // namespace Heltec::MeshCore::Dal::KeyInput
