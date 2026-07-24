#pragma once

#include <stdint.h>

namespace heltec::meshcore::ui {

using UiDeferredCallback = void (*)(void* user_data);

/** Create the fixed queue timer once during UI startup. */
bool ui_deferred_init();
bool ui_defer(UiDeferredCallback callback, void* user_data);
uint8_t ui_defer_cancel(UiDeferredCallback callback, void* user_data);

}  // namespace heltec::meshcore::ui
