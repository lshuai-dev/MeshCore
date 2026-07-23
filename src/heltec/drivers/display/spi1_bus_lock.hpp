#pragma once

#if defined(SPI_INTERFACES_COUNT) && (SPI_INTERFACES_COUNT >= 2)

#include <Arduino.h>
#include <SPI.h>

namespace heltec::meshcore::dal::spi1 {

static constexpr uint32_t kDisplaySpiHz = 40000000;

/** SD CS idle + TFT SPI clock before panel transactions. */
inline void prepareDisplayBus(SPIClass& spi) {
#if defined(PIN_MAP_SD_CS)
  pinMode(PIN_MAP_SD_CS, OUTPUT);
  digitalWrite(PIN_MAP_SD_CS, HIGH);
#endif
#if defined(PIN_TFT_CS)
  pinMode(PIN_TFT_CS, OUTPUT);
#endif
#if defined(ESP_PLATFORM)
  spi.setFrequency(kDisplaySpiHz);
#else
  (void)spi;
#endif
}

inline void prepareSdBus(SPIClass& spi) {
#if defined(PIN_TFT_CS)
  pinMode(PIN_TFT_CS, OUTPUT);
  digitalWrite(PIN_TFT_CS, HIGH);
#endif
#if defined(PIN_MAP_SD_CS)
  pinMode(PIN_MAP_SD_CS, OUTPUT);
#endif
  (void)spi;
}

}  // namespace heltec::meshcore::dal::spi1

#endif
