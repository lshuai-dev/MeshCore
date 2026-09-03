#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>
#include "target.h"
#if defined(RADIOCORE_BOARD_RC52) && defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include <heltec/sensors/ICMCompassProvider.h>
ICMCompassProvider compassProvider;
#endif

#if defined(RADIOCORE_BOARD_RCC6)
#include <hal/Arduino/ArduinoHal.h>
#endif

HeltecRadioCoreTargetBoard board;

#if defined(RADIOCORE_BOARD_RCC6)
namespace
{
class Rcc6SoftwareSpiHal final : public ArduinoHal
{
public:
    Rcc6SoftwareSpiHal(uint8_t sck, uint8_t miso, uint8_t mosi)
        : ArduinoHal(SPI), m_sck(sck), m_miso(miso), m_mosi(mosi)
    {
    }

    void init() override
    {
        spiBegin();
    }

    void term() override
    {
        spiEnd();
    }

    void spiBegin() override
    {
        ::pinMode(m_sck, OUTPUT);
        ::pinMode(m_miso, INPUT);
        ::pinMode(m_mosi, OUTPUT);
        ::digitalWrite(m_sck, LOW);
        ::digitalWrite(m_mosi, LOW);
    }

    void spiBeginTransaction() override
    {
    }

    void spiTransfer(uint8_t *out, size_t len, uint8_t *in) override
    {
        for (size_t index = 0; index < len; ++index)
        {
            const uint8_t output = out != nullptr ? out[index] : 0U;
            uint8_t input = 0U;
            for (uint8_t mask = 0x80U; mask != 0U; mask >>= 1U)
            {
                ::digitalWrite(m_mosi, (output & mask) != 0U ? HIGH : LOW);
                ::digitalWrite(m_sck, HIGH);
                input = static_cast<uint8_t>(input << 1U);
                if (::digitalRead(m_miso) == HIGH)
                {
                    input |= 1U;
                }
                ::digitalWrite(m_sck, LOW);
            }
            if (in != nullptr)
            {
                in[index] = input;
            }
        }
    }

    void spiEndTransaction() override
    {
    }

    void spiEnd() override
    {
        ::digitalWrite(m_sck, LOW);
        ::digitalWrite(m_mosi, LOW);
    }

private:
    const uint8_t m_sck;
    const uint8_t m_miso;
    const uint8_t m_mosi;
};

Rcc6SoftwareSpiHal s_radio_spi(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
} // namespace
#endif

#if defined(RADIOCORE_BOARD_RC52)
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#elif defined(RADIOCORE_BOARD_RCC6)
  RADIO_CLASS radio = new Module(&s_radio_spi, P_LORA_NSS, P_LORA_DIO_1,
                                 P_LORA_RESET, P_LORA_BUSY);
#elif defined(RADIOCORE_BOARD_RC32)
  static SPIClass spi(HSPI);
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#elif defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

namespace heltec::meshcore::board {
bool lnaCanControl() { return false; }
bool setLnaEnable(bool enabled) { (void)enabled; return false; }
}

#if defined(RADIOCORE_BOARD_RC52)
VolatileRTCClock fallback_clock;
#else
ESP32RTCClock fallback_clock;
#endif
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
  #if defined(RADIOCORE_BOARD_RC52) && defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  HeltecEnvironmentSensorManager sensors(compassProvider);
  #else
  HeltecEnvironmentSensorManager sensors;
  #endif
  #else
  EnvironmentSensorManager sensors;
  #endif
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display(&board.periph_power);   // peripheral power pin is shared
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

bool radio_init() {
#if !defined(RADIOCORE_BOARD_RC52)
  MESH_DEBUG_PRINTLN("[radio] fallback clock begin");
  fallback_clock.begin();
#endif
#if !defined(RADIOCORE_BOARD_RCC6)
  rtc_clock.begin(Wire);
#endif
  
#if defined(RADIOCORE_BOARD_RC52)
  SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
  return radio.std_init(&SPI);
#elif defined(P_LORA_SCLK)
  MESH_DEBUG_PRINTLN("[radio] SX1262 begin");
#if defined(RADIOCORE_BOARD_RCC6)
  return radio.std_init();
#else
  return radio.std_init(&spi);
#endif
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
