#pragma once

#include <Arduino.h>

class GpsNulFilteringStream final : public Stream {
 public:
  explicit GpsNulFilteringStream(Stream& source) : _source(source) {}

  int available() override {
    if (_pending >= 0) return 1;
    size_t discarded = 0;
    while (_source.available() && discarded < 256) {
      const int value = _source.read();
      if (value < 0) break;
      if (value == 0) { ++discarded; continue; }
      _pending = value;
      return 1;
    }
#if defined(GPS_NMEA_DEBUG)
    if (discarded) Serial.printf("DEBUG: [gps] ignored %u NUL RX bytes\n", (unsigned)discarded);
#endif
    return 0;
  }

  int read() override {
    if (available() == 0) return -1;
    const int value = _pending;
    _pending = -1;
    return value;
  }

  int peek() override {
    if (available() == 0) return -1;
    return _pending;
  }

  void flush() override { _source.flush(); }
  size_t write(uint8_t value) override { return _source.write(value); }
  using Print::write;

 private:
  Stream& _source;
  int _pending = -1;
};
