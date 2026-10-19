/**
 * @file    RadioCoreTca6408a.cpp
 * @brief   Radio Core TCA6408A GPIO expander implementation.
 */
#include "RadioCoreTca6408a.h"

namespace
{
constexpr uint8_t kDeviceAddress = 0x20U;
constexpr uint8_t kInputRegister = 0x00U;
constexpr uint8_t kPolarityRegister = 0x02U;
constexpr uint8_t kConfigurationRegister = 0x03U;
constexpr uint8_t kAllPinsInput = 0xFFU;
constexpr uint8_t kDipSwitchMask = 0x03U;
constexpr uint8_t kDipSwitchCount = 2U;
constexpr uint32_t kInputDebounceMs = 40U;
constexpr uint32_t kResetPulseMs = 1U;
constexpr uint32_t kResetRecoveryMs = 2U;
} // namespace

RadioCoreTCA6408A::RadioCoreTCA6408A(TwoWire &wire)
    : m_wire(wire),
      m_input_state(kAllPinsInput),
      m_previous_dip_state(0U),
      m_last_press_ms{0U, 0U},
      m_ready(false)
{
}

bool RadioCoreTCA6408A::begin(int reset_pin, int interrupt_pin)
{
    m_ready = false;
    m_input_state = kAllPinsInput;
    m_previous_dip_state = 0U;
    m_last_press_ms[0] = 0U;
    m_last_press_ms[1] = 0U;

    if (reset_pin >= 0)
    {
        pinMode(reset_pin, OUTPUT);
        digitalWrite(reset_pin, LOW);
        delay(kResetPulseMs);
        digitalWrite(reset_pin, HIGH);
        delay(kResetRecoveryMs);
    }
    if (interrupt_pin >= 0)
    {
        pinMode(interrupt_pin, INPUT_PULLUP);
    }

    if (!writeRegister(kPolarityRegister, 0x00U) ||
        !writeRegister(kConfigurationRegister, kAllPinsInput))
    {
        return false;
    }

    m_ready = readRegister(kInputRegister, m_input_state);
    if (m_ready)
    {
        m_previous_dip_state = getDipSwitchState();
    }
    return m_ready;
}

bool RadioCoreTCA6408A::refresh()
{
    uint8_t input_state = kAllPinsInput;
    if (!readRegister(kInputRegister, input_state))
    {
        m_ready = false;
        return false;
    }

    m_input_state = input_state;
    m_ready = true;
    return true;
}

uint8_t RadioCoreTCA6408A::pollPressedEdges()
{
    if (!refresh())
    {
        return 0U;
    }

    const uint8_t current_state = getDipSwitchState();
    const uint8_t pressed_edges =
        static_cast<uint8_t>(current_state & static_cast<uint8_t>(~m_previous_dip_state));
    const uint32_t now_ms = millis();
    uint8_t accepted_edges = 0U;

    for (uint8_t index = 0U; index < kDipSwitchCount; ++index)
    {
        const uint8_t mask = static_cast<uint8_t>(1U << index);
        if ((pressed_edges & mask) == 0U)
        {
            continue;
        }
        if (m_last_press_ms[index] == 0U ||
            (now_ms - m_last_press_ms[index]) >= kInputDebounceMs)
        {
            accepted_edges = static_cast<uint8_t>(accepted_edges | mask);
            m_last_press_ms[index] = now_ms;
        }
    }

    m_previous_dip_state = current_state;
    return accepted_edges;
}

bool RadioCoreTCA6408A::isReady() const
{
    return m_ready;
}

uint8_t RadioCoreTCA6408A::getDipSwitchState() const
{
    return static_cast<uint8_t>((~m_input_state) & kDipSwitchMask);
}

bool RadioCoreTCA6408A::isDipSwitchOn(uint8_t index) const
{
    if (index >= kDipSwitchCount)
    {
        return false;
    }
    return (getDipSwitchState() & static_cast<uint8_t>(1U << index)) != 0U;
}

uint8_t RadioCoreTCA6408A::getRawInputState() const
{
    return m_input_state;
}

bool RadioCoreTCA6408A::writeRegister(uint8_t reg, uint8_t value)
{
    m_wire.beginTransmission(kDeviceAddress);
    m_wire.write(reg);
    m_wire.write(value);
    return m_wire.endTransmission() == 0U;
}

bool RadioCoreTCA6408A::readRegister(uint8_t reg, uint8_t &value)
{
    m_wire.beginTransmission(kDeviceAddress);
    m_wire.write(reg);
    if (m_wire.endTransmission(false) != 0U)
    {
        return false;
    }
    if (m_wire.requestFrom(kDeviceAddress, static_cast<uint8_t>(1U)) != 1U)
    {
        return false;
    }

    value = static_cast<uint8_t>(m_wire.read());
    return true;
}
