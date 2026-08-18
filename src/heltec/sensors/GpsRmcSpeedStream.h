#pragma once

#include <Arduino.h>
#include <MicroNMEA.h>
#include <limits.h>
#include <string.h>

namespace heltec::gps
{

struct RmcSpeedState
{
    bool valid = false;
    long milli_knots = LONG_MIN;
    uint32_t updated_ms = 0;
};

inline RmcSpeedState &rmcSpeedState()
{
    static RmcSpeedState state;
    return state;
}

inline bool rmcSpeedKph(float &speed_kph, uint32_t max_age_ms = 3000)
{
    const RmcSpeedState &state = rmcSpeedState();
    if (!state.valid || state.milli_knots == LONG_MIN || millis() - state.updated_ms > max_age_ms)
    {
        return false;
    }
    speed_kph = static_cast<float>(state.milli_knots) * 0.001f * 1.852f;
    return true;
}

/** Pass-through GPS stream that observes RMC SOG without changing helpers. */
class GpsRmcSpeedStream final : public Stream
{
  public:
    explicit GpsRmcSpeedStream(Stream &source) : _source(source), _parser(_buffer, sizeof(_buffer))
    {
    }

    int available() override
    {
        return _source.available();
    }

    int read() override
    {
        const int value = _source.read();
        if (value >= 0 && _parser.process(static_cast<char>(value)) &&
            strcmp(_parser.getMessageID(), "RMC") == 0)
        {
            RmcSpeedState &state = rmcSpeedState();
            state.valid = _parser.isValid() && _parser.getSpeed() != LONG_MIN;
            state.milli_knots = state.valid ? _parser.getSpeed() : LONG_MIN;
            state.updated_ms = millis();
        }
        return value;
    }

    int peek() override
    {
        return _source.peek();
    }
    void flush() override
    {
        _source.flush();
    }
    size_t write(uint8_t value) override
    {
        return _source.write(value);
    }
    using Print::write;

  private:
    Stream &_source;
    char _buffer[100];
    MicroNMEA _parser;
};

} // namespace heltec::gps
