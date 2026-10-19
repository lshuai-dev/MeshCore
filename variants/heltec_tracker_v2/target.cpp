#include <Arduino.h>
#include "target.h"

HeltecTrackerV2Board board;

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

namespace heltec::meshcore::board {
bool lnaCanControl() { return ::board.isLnaCanControl(); }
bool setLnaEnable(bool enabled) { return ::board.setLNAEnable(enabled); }
}

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

#if ENV_INCLUDE_GPS
  #include <helpers/sensors/MicroNMEALocationProvider.h>
  #include <heltec/sensors/GpsRmcSpeedStream.h>

  namespace {
  class TrackerV2LocationProvider final : public MicroNMEALocationProvider {
   public:
    TrackerV2LocationProvider(Stream& serial, mesh::RTCClock* clock, int reset_pin,
                              RefCountedDigitalPin* peripheral_power)
        : MicroNMEALocationProvider(serial, clock, reset_pin, -1, peripheral_power) {}

    void begin() override {
      MicroNMEALocationProvider::begin();
      _running = true;
    }

    void stop() override {
      MicroNMEALocationProvider::stop();
      _running = false;
    }

    bool isEnabled() override {
      const bool vext_on = digitalRead(PIN_VEXT_EN) == PIN_VEXT_EN_ACTIVE;
      return _running && vext_on;
    }

   private:
    bool _running = false;
  };
  }  // namespace

  heltec::gps::GpsRmcSpeedStream gps_serial(Serial1);
  TrackerV2LocationProvider nmea(gps_serial, &rtc_clock, GPS_RESET,
                                 &board.periph_power);
  #if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
  HeltecEnvironmentSensorManager sensors(nmea);
  #else
  EnvironmentSensorManager sensors = EnvironmentSensorManager(nmea);
  #endif
#else
  #if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
  HeltecEnvironmentSensorManager sensors;
  #else
  EnvironmentSensorManager sensors;
  #endif
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display(&board.periph_power);   // peripheral power pin is shared
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);
  
#if defined(P_LORA_SCLK)
  return radio.std_init(&spi);
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
  return mesh::LocalIdentity(&rng);  // create new random identity
}
