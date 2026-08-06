#include "momentary_button.hpp"
#include "momentary_button_detail.hpp"

#include <Arduino.h>
#include "btn_debug.hpp"
#include "key_input.hpp"

#ifndef HELTEC_BUTTON_MAX_CLICKS
#define HELTEC_BUTTON_MAX_CLICKS 3
#endif

namespace heltec::meshcore::dal::momentary_button {
namespace {

static_assert(MOMENTARY_BUTTON_MAX >= 1 && MOMENTARY_BUTTON_MAX <= 4,
              "MOMENTARY_BUTTON_MAX must be 1..4");

// Keep enough timestamped transitions to reconstruct a full multi-click made
// while a blocking e-paper update owns the UI task.
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
static constexpr uint8_t kEdgeCap = 16;
#else
static constexpr uint8_t kEdgeCap = 4;
#endif

#if defined(ESP_PLATFORM)
#define BTN_ISR IRAM_ATTR
#else
#define BTN_ISR
#endif

struct ButtonState {
  int8_t pin = -1;
  uint8_t active_level = 1;
  uint8_t prev = 0;
  uint8_t candidate_level = 0;
  unsigned long candidate_since = 0;
  unsigned long down_at = 0;
  unsigned long last_edge_at = 0;
  uint8_t clicks = 0;
  unsigned long last_release = 0;
  KeyMap map;
  volatile uint8_t edge_head = 0;
  volatile uint8_t edge_tail = 0;
  volatile uint8_t edge_level[kEdgeCap] = {};
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
  volatile uint32_t edge_at[kEdgeCap] = {};
#endif
};

static Config s_cfg;
static ButtonState s_buttons[MOMENTARY_BUTTON_MAX];
static bool s_gpio_ready = false;
static bool s_irq_attached = false;
static GestureEmitFn s_gesture_emit = nullptr;

static inline bool is_pressed(uint8_t level, uint8_t active_level) {
  return level == active_level;
}

static uint32_t key_for_clicks(const KeyMap& map, uint8_t count) {
  if (count == 1) return map.click;
  if (count == 2) return map.dbl;
  return map.tri;
}

static ButtonGesture gesture_for_clicks(uint8_t count) {
  if (count <= 1) return ButtonGesture::Click;
  if (count == 2) return ButtonGesture::Double;
  return ButtonGesture::Triple;
}

static void emit_gesture(uint8_t slot, ButtonGesture gesture) {
  if (s_gesture_emit) {
    s_gesture_emit(slot, gesture);
    return;
  }

  const ButtonState& button = s_buttons[slot];
  const uint32_t key = (gesture == ButtonGesture::Long)
                           ? button.map.long_press
                           : key_for_clicks(button.map, static_cast<uint8_t>(gesture));
  key_input::post(key, gesture == ButtonGesture::Long
                           ? key_input::KeyDelivery::Pulse
                           : key_input::KeyDelivery::Tracked);
}

static void BTN_ISR enqueue_level(uint8_t slot) {
  if (slot >= MOMENTARY_BUTTON_MAX) return;
  ButtonState& button = s_buttons[slot];
  if (button.pin < 0) return;

  const uint8_t level = (uint8_t)digitalRead((uint8_t)button.pin);
  const uint8_t next = (uint8_t)((button.edge_tail + 1U) % kEdgeCap);
  if (next == button.edge_head) {
    button.edge_head = (uint8_t)((button.edge_head + 1U) % kEdgeCap);
  }
  button.edge_level[button.edge_tail] = level;
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
  button.edge_at[button.edge_tail] = millis();
#endif
  button.edge_tail = next;
}

#if MOMENTARY_BUTTON_MAX >= 1
static void BTN_ISR isr0() { enqueue_level(0); }
#endif
#if MOMENTARY_BUTTON_MAX >= 2
static void BTN_ISR isr1() { enqueue_level(1); }
#endif
#if MOMENTARY_BUTTON_MAX >= 3
static void BTN_ISR isr2() { enqueue_level(2); }
#endif
#if MOMENTARY_BUTTON_MAX >= 4
static void BTN_ISR isr3() { enqueue_level(3); }
#endif

static void (*isr_for_slot(uint8_t slot))() {
  switch (slot) {
#if MOMENTARY_BUTTON_MAX >= 1
    case 0:
      return isr0;
#endif
#if MOMENTARY_BUTTON_MAX >= 2
    case 1:
      return isr1;
#endif
#if MOMENTARY_BUTTON_MAX >= 3
    case 2:
      return isr2;
#endif
#if MOMENTARY_BUTTON_MAX >= 4
    case 3:
      return isr3;
#endif
    default:
      return nullptr;
  }
}

static void detach_irqs() {
  if (!s_irq_attached) return;
  for (uint8_t i = 0; i < MOMENTARY_BUTTON_MAX; ++i) {
    const int8_t pin = s_buttons[i].pin;
    if (pin < 0) continue;
    const int irq = digitalPinToInterrupt((uint8_t)pin);
    if (irq >= 0) detachInterrupt(irq);
  }
  s_irq_attached = false;
}

static void attach_irqs() {
  detach_irqs();
  for (uint8_t i = 0; i < MOMENTARY_BUTTON_MAX; ++i) {
    const int8_t pin = s_buttons[i].pin;
    if (pin < 0) continue;

    void (*isr)() = isr_for_slot(i);
    if (!isr) continue;

    const int irq = digitalPinToInterrupt((uint8_t)pin);
    if (irq >= 0) attachInterrupt(irq, isr, CHANGE);
  }
  s_irq_attached = true;
}

static void accept_level(ButtonState& button, uint8_t slot, uint8_t level,
                         unsigned long now) {
  if (level == button.prev) return;

  if (is_pressed(level, button.active_level)) {
    button.down_at = now;
    BTN_UI_LOG("gpio pin=%d press lvl=%u idle=%u", (int)button.pin, (unsigned)level,
               (unsigned)button.prev);
  } else if (button.down_at > 0) {
    const unsigned long held = now - button.down_at;
    if (s_cfg.long_press_ms == 0 || held < s_cfg.long_press_ms) {
      ++button.clicks;
      if (HELTEC_BUTTON_MAX_CLICKS >= 3 && button.clicks >= 3) {
        emit_gesture(slot, ButtonGesture::Triple);
        button.clicks = 0;
        button.last_release = 0;
      } else if (button.clicks > HELTEC_BUTTON_MAX_CLICKS) {
        emit_gesture(slot, gesture_for_clicks(HELTEC_BUTTON_MAX_CLICKS));
        button.clicks = 1;
        button.last_release = now;
      } else {
        button.last_release = now;
      }
    }
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
    else if (button.clicks > 0) {
      button.clicks = 0;
      button.last_release = 0;
    } else {
      BTN_UI_LOG("slot=%u pin=%d replayed long -> gesture", (unsigned)slot,
                 (int)button.pin);
      emit_gesture(slot, ButtonGesture::Long);
    }
#endif
    BTN_UI_LOG("gpio pin=%d release held=%lums clicks=%u", (int)button.pin,
               (unsigned long)held, (unsigned)button.clicks);
    button.down_at = 0;
  }

  button.prev = level;
}

static void drain_edges(ButtonState& button, uint8_t slot, unsigned long now) {
  while (button.edge_head != button.edge_tail) {
    const uint8_t level = button.edge_level[button.edge_head];
    button.edge_head = (uint8_t)((button.edge_head + 1U) % kEdgeCap);
    if (level == button.prev) continue;
    if (s_cfg.debounce_ms > 0 && button.last_edge_at > 0 &&
        (now - button.last_edge_at) < s_cfg.debounce_ms) {
      BTN_UI_LOG("gpio pin=%d debounce lvl=%u", (int)button.pin, (unsigned)level);
      continue;
    }
    button.last_edge_at = now;
    accept_level(button, slot, level, now);
  }
}

static void confirm_stable_level(ButtonState& button, uint8_t slot,
                                 unsigned long now) {
  const uint8_t pending_edges =
      (uint8_t)((button.edge_tail + kEdgeCap - button.edge_head) % kEdgeCap);
  if (pending_edges > 0) {
    BTN_UI_LOG("gpio pin=%d poll pending_edges=%u current=%u accepted=%u",
               (int)button.pin, (unsigned)pending_edges,
               (unsigned)digitalRead((uint8_t)button.pin), (unsigned)button.prev);
  }

#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
  // Replay timestamped candidates instead of looking only at the current GPIO
  // level. A complete click can otherwise disappear while panel->update()
  // blocks polling and the pin has already returned to its idle level.
  while (button.edge_head != button.edge_tail) {
    const uint8_t index = button.edge_head;
    const uint8_t level = button.edge_level[index];
    const unsigned long edge_at = button.edge_at[index];
    button.edge_head = (uint8_t)((button.edge_head + 1U) % kEdgeCap);
    if (level == button.candidate_level) continue;

    if (button.candidate_level != button.prev &&
        (edge_at - button.candidate_since) >= s_cfg.debounce_ms) {
      accept_level(button, slot, button.candidate_level,
                   button.candidate_since + s_cfg.debounce_ms);
    }
    button.candidate_level = level;
    button.candidate_since = edge_at;
    BTN_UI_LOG("gpio pin=%d candidate lvl=%u", (int)button.pin, (unsigned)level);
  }
#else
  button.edge_head = button.edge_tail;
#endif

  const uint8_t level = (uint8_t)digitalRead((uint8_t)button.pin);
  if (level != button.candidate_level) {
    button.candidate_level = level;
    button.candidate_since = now;
    BTN_UI_LOG("gpio pin=%d candidate lvl=%u", (int)button.pin, (unsigned)level);
  }

  if (button.candidate_level == button.prev) return;
  if (s_cfg.debounce_ms > 0 &&
      (now - button.candidate_since) < s_cfg.debounce_ms) {
    return;
  }

  accept_level(button, slot, button.candidate_level,
               button.candidate_since + s_cfg.debounce_ms);
}

static void tick_button(ButtonState& button, uint8_t slot, unsigned long now) {
  if (button.pin < 0) return;

  if (s_cfg.stable_level_debounce) {
    confirm_stable_level(button, slot, now);
  } else {
    drain_edges(button, slot, now);
  }

  if (s_cfg.long_press_ms > 0 && button.down_at > 0 &&
      (now - button.down_at) >= s_cfg.long_press_ms) {
    if (button.clicks > 0) {
      button.clicks = 0;
      button.last_release = 0;
    } else {
      BTN_UI_LOG("slot=%u pin=%d long -> gesture", (unsigned)slot, (int)button.pin);
      emit_gesture(slot, ButtonGesture::Long);
    }
    button.down_at = 0;
    return;
  }

  if (button.clicks > 0 && button.down_at == 0 &&
      (now - button.last_release) >= s_cfg.multi_click_window_ms) {
    const uint8_t count = button.clicks > HELTEC_BUTTON_MAX_CLICKS ? HELTEC_BUTTON_MAX_CLICKS
                                                                   : button.clicks;
    BTN_UI_LOG("slot=%u pin=%d clicks=%u -> gesture", (unsigned)slot, (int)button.pin,
               (unsigned)count);
    emit_gesture(slot, gesture_for_clicks(count));
    button.clicks = 0;
    button.last_release = 0;
  }
}

static void poll_buttons() {
  const unsigned long now = millis();
  for (uint8_t i = 0; i < MOMENTARY_BUTTON_MAX; ++i) {
    tick_button(s_buttons[i], i, now);
  }
}

static void setup_gpio() {
  if (s_gpio_ready) return;
  detach_irqs();

  for (uint8_t i = 0; i < MOMENTARY_BUTTON_MAX; ++i) {
    const ButtonConfig& config = s_cfg.buttons[i];
    ButtonState& button = s_buttons[i];
    button = ButtonState{};
    button.pin = config.pin;
    button.active_level = config.active_level;
    button.map = config.map;
    if (button.pin < 0) continue;

    pinMode((uint8_t)button.pin, config.pin_mode);
    button.prev = (uint8_t)digitalRead((uint8_t)button.pin);
    button.candidate_level = button.prev;
    button.candidate_since = millis();
    BTN_UI_LOG("init slot=%u pin=%d mode=%u active=%u lvl=%u irq=%d", (unsigned)i,
               (int)button.pin, (unsigned)config.pin_mode, (unsigned)config.active_level,
               (unsigned)button.prev, (int)digitalPinToInterrupt((uint8_t)button.pin));
  }

  attach_irqs();
  s_gpio_ready = true;
}

static bool any_button_active() {
  const unsigned long now = millis();
  for (uint8_t i = 0; i < MOMENTARY_BUTTON_MAX; ++i) {
    const int8_t pin = s_buttons[i].pin;
    if (pin < 0) continue;
    if (s_cfg.stable_level_debounce) {
      // key_input queries this provider while waiting for a long-press key to
      // be physically released, a state in which its normal poll hook pauses.
      confirm_stable_level(s_buttons[i], i, now);
    }
    const uint8_t level = s_cfg.stable_level_debounce
                              ? s_buttons[i].prev
                              : (uint8_t)digitalRead((uint8_t)pin);
    if (is_pressed(level, s_buttons[i].active_level)) return true;
  }
  return false;
}

}  // namespace

void configure(const Config& cfg) {
  detach_irqs();
  s_cfg = cfg;
  s_gpio_ready = false;
}

bool initialize() {
  setup_gpio();
  key_input::set_poll_hook(poll_buttons);
  key_input::set_down_state_provider(any_button_active);
  return key_input::initialize();
}

void set_gesture_emit(GestureEmitFn fn) {
  s_gesture_emit = fn;
}

bool anyPhysicalPressed() {
  return any_button_active();
}

}  // namespace heltec::meshcore::dal::momentary_button
