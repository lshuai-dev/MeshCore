#include "target.h"

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>

#ifdef ENV_INCLUDE_GPS
#include <helpers/sensors/MicroNMEALocationProvider.h>
#endif

T1Board board;

#if defined(P_LORA_SCLK)
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#else
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

namespace {
constexpr uint32_t kMinSupportedUnixTime = 1704067200UL; // 2024-01-01 UTC
constexpr uint32_t kMaxSupportedUnixTime = 0x7FFFFFFFUL;

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
constexpr uint32_t kGpsUtcSampleIntervalMs = 1000UL;
constexpr uint32_t kGpsRtcResyncIntervalMs = 30UL * 60UL * 1000UL;
constexpr uint32_t kGpsUtcMaxStepSec = 2UL;
constexpr int32_t kRtcImmediateCorrectionSec = 5;
constexpr int32_t kRtcScheduledCorrectionSec = 1;
constexpr uint8_t kGpsUtcRequiredSamples = 3;
constexpr uint8_t kGpsUtcMaxStaleSamples = 2;

uint32_t s_gps_utc_next_sample_ms = 0;
uint32_t s_gps_utc_next_sync_ms = 0;
uint32_t s_gps_utc_previous = 0;
uint8_t s_gps_utc_consecutive = 0;
uint8_t s_gps_utc_stale = 0;

void resetGpsUtcSequence() {
  s_gps_utc_previous = 0;
  s_gps_utc_consecutive = 0;
  s_gps_utc_stale = 0;
}
#endif
}

void T1RTCClock::setCurrentTime(uint32_t time) {
  if (time < kMinSupportedUnixTime || time > kMaxSupportedUnixTime) return;
  VolatileRTCClock::setCurrentTime(time);
}

T1RTCClock rtc_clock;

#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "sensors/ICMCompassProvider.h"
ICMCompassProvider compassProvider;
#endif
#if ENV_INCLUDE_GPS
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1, &rtc_clock, GPS_RESET, GPS_EN);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
HeltecEnvironmentSensorManager sensors(nmea, compassProvider);
#else
HeltecEnvironmentSensorManager sensors(nmea);
#endif
#else
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
HeltecEnvironmentSensorManager sensors(compassProvider);
#else
HeltecEnvironmentSensorManager sensors;
#endif
#endif
#else
#if ENV_INCLUDE_GPS
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
MicroNMEALocationProvider nmea =
    MicroNMEALocationProvider(Serial1, &rtc_clock, GPS_RESET, GPS_EN, &board._sensorPower);
#else
MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1, &rtc_clock, GPS_RESET, GPS_EN, &board.periph_power);
#endif
EnvironmentSensorManager sensors = EnvironmentSensorManager(nmea);
#else
EnvironmentSensorManager sensors;
#endif
#endif

void T1RTCClock::tick() {
  VolatileRTCClock::tick();

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  const uint32_t now_ms = millis();
  if ((int32_t)(now_ms - s_gps_utc_next_sample_ms) < 0) return;
  s_gps_utc_next_sample_ms = now_ms + kGpsUtcSampleIntervalMs;

  if (!nmea.isEnabled()) {
    resetGpsUtcSequence();
    return;
  }

  const long raw_timestamp = nmea.getTimestamp();
  if (raw_timestamp < (long)kMinSupportedUnixTime ||
      raw_timestamp > (long)kMaxSupportedUnixTime) {
    resetGpsUtcSequence();
    return;
  }

  const uint32_t gps_timestamp = (uint32_t)raw_timestamp;
  if (s_gps_utc_previous == 0) {
    s_gps_utc_previous = gps_timestamp;
    s_gps_utc_consecutive = 1;
    return;
  }

  if (gps_timestamp == s_gps_utc_previous) {
    if (++s_gps_utc_stale > kGpsUtcMaxStaleSamples) {
      s_gps_utc_consecutive = 0;
    }
    return;
  }

  const bool continuous = gps_timestamp > s_gps_utc_previous &&
                          gps_timestamp - s_gps_utc_previous <= kGpsUtcMaxStepSec;
  if (!continuous) {
    s_gps_utc_previous = gps_timestamp;
    s_gps_utc_consecutive = 1;
    s_gps_utc_stale = 0;
    return;
  }

  s_gps_utc_previous = gps_timestamp;
  s_gps_utc_stale = 0;
  if (s_gps_utc_consecutive < kGpsUtcRequiredSamples) {
    ++s_gps_utc_consecutive;
  }
  if (s_gps_utc_consecutive < kGpsUtcRequiredSamples) return;

  const uint32_t local_before = getCurrentTime();
  const int32_t delta_sec = (int32_t)(gps_timestamp - local_before);
  const int32_t abs_delta_sec = delta_sec < 0 ? -delta_sec : delta_sec;
  const bool scheduled = (int32_t)(now_ms - s_gps_utc_next_sync_ms) >= 0;
  const bool clearly_wrong = abs_delta_sec > kRtcImmediateCorrectionSec;
  if (!scheduled && !clearly_wrong) return;

  const bool correction_needed = abs_delta_sec > kRtcScheduledCorrectionSec;
  if (correction_needed) {
    setCurrentTime(gps_timestamp);
  }
  s_gps_utc_next_sync_ms = now_ms + kGpsRtcResyncIntervalMs;
#endif
}

#ifdef DISPLAY_CLASS
#if !(defined(HELTEC_MESH_UI) && HELTEC_MESH_UI)
DISPLAY_CLASS display(&board.periph_power);
MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif
#endif

bool radio_init() {
#if defined(P_LORA_SCLK)
  return radio.std_init(&SPI);
#else
  return radio.std_init();
#endif
}

uint32_t radio_get_rng_seed() {
  return radio.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio.setFrequency(freq);
  radio.setSpreadingFactor(sf);
  radio.setBandwidth(bw);
  radio.setCodingRate(cr);
}

void radio_set_tx_power(int8_t dbm) {
  radio.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);
}
