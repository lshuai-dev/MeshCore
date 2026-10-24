#include "KeyInput.hpp"

#include <lvgl.h>

namespace Heltec::MeshCore::Dal::KeyInput
{
namespace
{

static constexpr uint8_t kQueueCapacity = 8;

static lv_group_t *s_group = nullptr;
static uint32_t s_queue[kQueueCapacity] = {};
static uint8_t s_head = 0;
static uint8_t s_tail = 0;
static uint8_t s_count = 0;
static bool s_dispatch_pending = false;
static bool s_dispatching = false;

static void clearQueue()
{
    s_head = 0;
    s_tail = 0;
    s_count = 0;
}

static bool scheduleDispatch();

static void dispatchKey(void *user_data)
{
    (void)user_data;
    if (s_count == 0)
    {
        s_dispatch_pending = false;
        return;
    }

    const uint32_t key = s_queue[s_head];
    s_head = static_cast<uint8_t>((s_head + 1U) % kQueueCapacity);
    --s_count;

    // Keep the pending flag set while handlers run so recursive posts reuse
    // the next callback instead of creating a second dispatcher.
    if (s_group != nullptr)
    {
        s_dispatching = true;
        (void)lv_group_send_data(s_group, key);
        s_dispatching = false;
    }

    s_dispatch_pending = false;
    if (s_count > 0 && !scheduleDispatch())
    {
        clearQueue();
    }
}

static bool scheduleDispatch()
{
    if (s_dispatch_pending)
    {
        return true;
    }
    if (lv_async_call(dispatchKey, nullptr) != LV_RES_OK)
    {
        return false;
    }
    s_dispatch_pending = true;
    return true;
}

} // namespace

void setGroup(_lv_group_t *group)
{
    s_group = group;
    if (group != nullptr)
    {
        return;
    }

    clearQueue();
    if (s_dispatching)
    {
        // Surface changes can occur synchronously inside dispatchKey(). Do not
        // delete the LVGL async timer while its callback is still running.
        return;
    }

    (void)lv_async_call_cancel(dispatchKey, nullptr);
    s_dispatch_pending = false;
}

bool postKey(uint32_t key)
{
    if (key == 0 || s_count >= kQueueCapacity)
    {
        return false;
    }

    s_queue[s_tail] = key;
    s_tail = static_cast<uint8_t>((s_tail + 1U) % kQueueCapacity);
    ++s_count;
    if (scheduleDispatch())
    {
        return true;
    }

    clearQueue();
    return false;
}

bool isDispatching()
{
    return s_dispatching;
}

} // namespace Heltec::MeshCore::Dal::KeyInput
