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
VolatileRTCClock rtc_clock;

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
