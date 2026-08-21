#pragma once

#include <stdint.h>

namespace heltec::meshcore::ui
{

using UiDeferredCallback = void (*)(void *user_data);

/** Create the fixed queue timer once during UI startup. */
bool deferred_init();
bool defer(UiDeferredCallback callback, void *user_data);
uint8_t defer_cancel(UiDeferredCallback callback, void *user_data);

} // namespace heltec::meshcore::ui
