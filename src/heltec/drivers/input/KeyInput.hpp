#pragma once

/**
 * @file KeyInput.hpp
 * @brief Unified key input device and queued key delivery.
 */

#include <stdint.h>

struct _lv_group_t;

namespace Heltec::MeshCore::Dal::KeyInput
{

using DownStateFn = bool (*)();
using PollFn = void (*)();

enum class KeyDelivery : uint8_t
{
    Tracked,
    Pulse,
    IndependentPulse,
};

bool initialize();
void set_group(_lv_group_t *group);
void set_down_state_provider(DownStateFn fn);
void set_poll_hook(PollFn fn);
/** Queue simulated input-device state for LVGL delivery. */
bool postState(uint32_t key, KeyDelivery delivery = KeyDelivery::Tracked);

/** Send one completed logical key after the current LVGL event finishes. */
bool postKey(uint32_t key);

} // namespace Heltec::MeshCore::Dal::KeyInput
