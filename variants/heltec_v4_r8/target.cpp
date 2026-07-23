#include <Arduino.h>
#include "target.h"
#include "SPI.h"

HeltecV4R8Board board;

#if defined(P_LORA_SCLK)
  // static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

namespace heltec::meshcore::board {
bool lnaCanControl() { return ::board.isLnaCanControl(); }
bool setLnaEnable(bool enabled) { return ::board.setLNAEnable(enabled); }
}

#if defined(SPI_INTERFACES_COUNT) && (SPI_INTERFACES_COUNT >= 2)
SPIClass SPI1(HSPI);
#endif

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);

#if defined(HELTEC_DISPLAY_ST7789)
#if ENV_INCLUDE_GPS
#include <helpers/sensors/MicroNMEALocationProvider.h>
#include <heltec/sensors/GpsNulFilteringStream.h>
GpsNulFilteringStream gps_serial(Serial1);
MicroNMEALocationProvider nmea(gps_serial, &rtc_clock, GPS_RESET, GPS_EN);
HeltecEnvironmentSensorManager sensors(nmea);
#else
HeltecEnvironmentSensorManager sensors;
#endif
#else
#if ENV_INCLUDE_GPS
  #include <helpers/sensors/MicroNMEALocationProvider.h>
  MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1, &rtc_clock, GPS_RESET, GPS_EN, &board.periph_power);
  EnvironmentSensorManager sensors = EnvironmentSensorManager(nmea);
#else
  EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display(&board.periph_power);
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif
#endif

bool radio_init() {
  fallback_clock.begin();
#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
#if !(defined(HELTEC_TOUCH_USE_CHSC6X) && HELTEC_TOUCH_USE_CHSC6X)
  rtc_clock.begin(Wire);
#endif
#endif

#if defined(P_LORA_SCLK)
  const bool ok = radio.std_init(&SPI);
#else
  const bool ok = radio.std_init();
#endif
  if (ok) board.ensureLoraRxMode();
  return ok;
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
