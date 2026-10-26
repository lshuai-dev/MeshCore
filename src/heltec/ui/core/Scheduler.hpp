#pragma once

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

using UiMotionExec = void (*)(void *target, int32_t value);
using UiMotionReady = void (*)(void *user_data);

enum class UiMotionPath : uint8_t
{
    Linear = 0,
    EaseIn,
    EaseOut,
    EaseInOut,
};

struct UiMotionSpec
{
    void *target = nullptr;
    UiMotionExec exec = nullptr;
    UiMotionReady ready = nullptr;
    void *ready_data = nullptr;
    int32_t start_value = 0;
    int32_t end_value = 0;
    uint16_t duration_ms = 0;
    UiMotionPath path = UiMotionPath::Linear;
};

/** Create the fixed-slot scheduler timer once during UI startup. */
bool initMotion();
bool startMotion(const UiMotionSpec &spec);
uint8_t cancelMotion(void *target, UiMotionExec exec = nullptr);
bool isMotionActive(void *target, UiMotionExec exec = nullptr);

} // namespace Heltec::MeshCore::Ui
