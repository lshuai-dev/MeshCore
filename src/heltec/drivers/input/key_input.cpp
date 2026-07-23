#include "key_input.hpp"

#include <lvgl.h>

#include "btn_debug.hpp"

namespace heltec::meshcore::dal::key_input {
namespace {

static lv_indev_drv_t s_drv;
static lv_indev_t* s_indev = nullptr;
static bool s_drv_registered = false;
static DownStateFn s_down_state_provider = nullptr;
static PollFn s_poll_hook = nullptr;

struct QueuedKey {
  uint32_t key = 0;
  KeyDelivery delivery = KeyDelivery::Tracked;
};

static constexpr uint8_t kQueueCapacity = 8;
static QueuedKey s_queue[kQueueCapacity];
static uint8_t s_queue_head = 0;
static uint8_t s_queue_tail = 0;
static uint8_t s_queue_count = 0;

enum class DeliveryState : uint8_t {
  Idle,
  TrackedPress,
  PulseRelease,
  WaitPhysicalRelease,
};

static DeliveryState s_state = DeliveryState::Idle;
static uint32_t s_current_key = LV_KEY_ENTER;
static bool s_release_wait_requested = false;

static bool is_down() {
  return s_down_state_provider && s_down_state_provider();
}

static void reset_state() {
  s_queue_head = 0;
  s_queue_tail = 0;
  s_queue_count = 0;
  s_state = DeliveryState::Idle;
  s_current_key = LV_KEY_ENTER;
  s_release_wait_requested = false;
}

static bool enqueue_key(uint32_t key, KeyDelivery delivery) {
  if (key == 0) return false;
  if (s_queue_count >= kQueueCapacity) {
    BTN_UI_LOG("key_input queue full; drop key=0x%lX", (unsigned long)key);
    return false;
  }
  s_queue[s_queue_tail].key = key;
  s_queue[s_queue_tail].delivery = delivery;
  s_queue_tail = (uint8_t)((s_queue_tail + 1) % kQueueCapacity);
  ++s_queue_count;
  return true;
}

static bool dequeue_key(QueuedKey& out) {
  if (s_queue_count == 0) return false;
  out = s_queue[s_queue_head];
  s_queue_head = (uint8_t)((s_queue_head + 1) % kQueueCapacity);
  --s_queue_count;
  return true;
}

static void wait_current_release() {
  lv_indev_t* indev = s_indev;
  if (!indev) indev = lv_indev_get_act();
  if (indev) lv_indev_wait_release(indev);
}

static void read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  (void)drv;

  data->key = s_current_key;

  if (s_state == DeliveryState::PulseRelease) {
    BTN_UI_LOG("key_input release key=0x%lX", (unsigned long)s_current_key);
    data->state = LV_INDEV_STATE_RELEASED;
    s_release_wait_requested = true;
    s_state = DeliveryState::WaitPhysicalRelease;
    return;
  }

  if (s_state == DeliveryState::WaitPhysicalRelease) {
    if (s_release_wait_requested) {
      wait_current_release();
      s_release_wait_requested = false;
    }
    // The synthetic pulse has already delivered both PRESSED and RELEASED.
    // While the physical button is still held, keep LVGL released so a new
    // group or the focused keyboard button cannot receive a trailing press.
    const bool pressed = is_down();
    data->state = LV_INDEV_STATE_RELEASED;
    if (!pressed) {
      BTN_UI_LOG("key_input release barrier done key=0x%lX",
                 (unsigned long)s_current_key);
      s_state = DeliveryState::Idle;
      data->continue_reading = s_queue_count > 0;
    }
    return;
  }

  if (s_poll_hook) s_poll_hook();

  if (s_state == DeliveryState::Idle) {
    QueuedKey queued;
    if (dequeue_key(queued)) {
      BTN_UI_LOG("key_input press key=0x%lX delivery=%u",
                 (unsigned long)queued.key, (unsigned)queued.delivery);
      s_current_key = queued.key;
      data->key = s_current_key;
      data->state = LV_INDEV_STATE_PRESSED;
      if (queued.delivery == KeyDelivery::Pulse) {
        s_state = DeliveryState::PulseRelease;
        data->continue_reading = true;
      } else {
        s_state = DeliveryState::TrackedPress;
      }
      return;
    }
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  if (is_down()) {
    data->state = LV_INDEV_STATE_PRESSED;
    return;
  }

  BTN_UI_LOG("key_input tracked release key=0x%lX", (unsigned long)s_current_key);
  data->state = LV_INDEV_STATE_RELEASED;
  s_state = DeliveryState::Idle;
  data->continue_reading = s_queue_count > 0;
}

}  // namespace

bool initialize() {
  reset_state();
  if (!s_drv_registered) {
    lv_indev_drv_init(&s_drv);
    s_drv.type = LV_INDEV_TYPE_KEYPAD;
    s_drv.read_cb = read_cb;
    s_indev = lv_indev_drv_register(&s_drv);
    s_drv_registered = true;
  }
  return s_indev != nullptr;
}

void set_group(_lv_group_t* group) {
  if (!s_indev) return;
  lv_indev_set_group(s_indev, group);
}

void set_down_state_provider(DownStateFn fn) {
  s_down_state_provider = fn;
}

void set_poll_hook(PollFn fn) {
  s_poll_hook = fn;
}

bool post(uint32_t key, KeyDelivery delivery) {
  return enqueue_key(key, delivery);
}

}  // namespace heltec::meshcore::dal::key_input
