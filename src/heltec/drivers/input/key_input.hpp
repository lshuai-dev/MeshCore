#pragma once

/**
 * @file key_input.hpp
 * @brief Unified key input device and queued key delivery.
 */

#include <stdint.h>

struct _lv_group_t;

namespace heltec::meshcore::dal::key_input {

using DownStateFn = bool (*)();
using PollFn = void (*)();

enum class KeyDelivery : uint8_t {
  Tracked,
  Pulse,
};

bool initialize();
void set_group(_lv_group_t* group);
void set_down_state_provider(DownStateFn fn);
void set_poll_hook(PollFn fn);
/** Queue a key for LVGL delivery. Returns false only when key is invalid or the queue is full. */
bool post(uint32_t key, KeyDelivery delivery = KeyDelivery::Tracked);

}  // namespace heltec::meshcore::dal::key_input
