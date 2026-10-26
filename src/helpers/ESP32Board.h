#pragma once

#include <MeshCore.h>
#include <Arduino.h>

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

#if defined(ESP_PLATFORM)

#include <rom/rtc.h>
#include <sys/time.h>
#include <Wire.h>
#include "soc/rtc.h"
#include "esp_system.h"
#include <driver/rtc_io.h>

class ESP32Board : public mesh::MainBoard {
protected:
  uint8_t startup_reason;
  bool inhibit_sleep = false;
  static inline portMUX_TYPE sleepMux = portMUX_INITIALIZER_UNLOCKED;

public:
  void begin() {
    // for future use, sub-classes SHOULD call this from their begin()
    startup_reason = BD_STARTUP_NORMAL;    

  #ifdef ESP32_CPU_FREQ
    setCpuFrequencyMhz(ESP32_CPU_FREQ);
  #endif

  #if defined(PIN_VBAT_READ) && PIN_VBAT_READ >= 0
    // battery read support
    pinMode(PIN_VBAT_READ, INPUT);
#if ESP_ARDUINO_VERSION_MAJOR < 3
    adcAttachPin(PIN_VBAT_READ);
#endif
  #endif

  #ifdef P_LORA_TX_LED
    pinMode(P_LORA_TX_LED, OUTPUT);
    digitalWrite(P_LORA_TX_LED, LOW);
  #endif

  #if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
   #if PIN_BOARD_SDA >= 0 && PIN_BOARD_SCL >= 0
    Wire.begin(PIN_BOARD_SDA, PIN_BOARD_SCL);
   #endif
  #else
    Wire.begin();
  #endif    
  }

  // Temperature from ESP32 MCU
  float getMCUTemperature() override {
    uint32_t raw = 0;

    // To get and average the temperature so it is more accurate, especially in low temperature
    for (int i = 0; i < 4; i++) {
      raw += temperatureRead();
    }

    return raw / 4;
  }

  virtual void powerOff() override;
  void enterDeepSleep(uint32_t secs);

  uint32_t getIRQGpio() override {
    return P_LORA_DIO_1; // default for SX1262
  }

  // Boards may add input wake sources such as buttons or a touch IRQ.
  // Each level is the asserted wake level for the corresponding GPIO.
  virtual uint8_t getAdditionalWakeupPins(uint8_t *pins, uint8_t *levels,
                                           uint8_t capacity) const {
    (void)pins;
    (void)levels;
    (void)capacity;
    return 0;
  }

  void sleep(uint32_t secs) override {
    // Skip if not allow to sleep
    if (inhibit_sleep) {
      delay(1); // Give MCU to OTA to run
      return;
    }

    // Set GPIO wakeup
    const gpio_num_t loraWakePin = (gpio_num_t)getIRQGpio();
    uint8_t additionalPins[4] = {};
    uint8_t additionalLevels[4] = {};
    const uint8_t additionalCount = getAdditionalWakeupPins(
        additionalPins, additionalLevels, sizeof(additionalPins));
    uint8_t additionalPins[4] = {};
    uint8_t additionalLevels[4] = {};
    const uint8_t additionalCount = getAdditionalWakeupPins(
        additionalPins, additionalLevels, sizeof(additionalPins));

    // A non-zero sleep interval is a safety net for software deadlines such as
    // Team heartbeats and delayed Mesh transmissions. GPIO interrupts remain
    // the normal fast wake path.
    if (secs > 0) {
      esp_sleep_enable_timer_wakeup(secs * 1000000ULL); // Wake up periodically to do scheduled jobs
    }

    // Disable CPU interrupt servicing
    portENTER_CRITICAL(&sleepMux);

    // Skip sleep if there is a LoRa packet
    if (gpio_get_level(loraWakePin) == HIGH) {
      portEXIT_CRITICAL(&sleepMux);
      delay(1);
      return;
    }

    // Configure GPIO wakeup
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable(loraWakePin, GPIO_INTR_HIGH_LEVEL);
    for (uint8_t index = 0; index < additionalCount; ++index) {
      const gpio_num_t pin = (gpio_num_t)additionalPins[index];
      const int level = additionalLevels[index] ? HIGH : LOW;
      if (gpio_get_level(pin) == level) {
        gpio_wakeup_disable(loraWakePin);
        portEXIT_CRITICAL(&sleepMux);
        delay(1);
        return;
      }
      gpio_wakeup_enable(pin, additionalLevels[index] ? GPIO_INTR_HIGH_LEVEL
                                                       : GPIO_INTR_LOW_LEVEL);
    }

    // MCU enters light sleep
    esp_light_sleep_start();

    // Avoid ISR flood during wakeup due to HIGH LEVEL interrupt
    gpio_wakeup_disable(loraWakePin);
    gpio_set_intr_type(loraWakePin, GPIO_INTR_POSEDGE);
    for (uint8_t index = 0; index < additionalCount; ++index) {
      const gpio_num_t pin = (gpio_num_t)additionalPins[index];
      gpio_wakeup_disable(pin);
      // MomentaryButton uses CHANGE; restoring only the wake edge loses the
      // release transition and leaves the button logically held.
      gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
    }

    // Enable CPU interrupt servicing
    portEXIT_CRITICAL(&sleepMux);
  }

  void sleepMillis(uint32_t millis) override {
    if (inhibit_sleep || millis == 0U) {
      delay(1);
      return;
    }
    const gpio_num_t loraWakePin = (gpio_num_t)getIRQGpio();
    if (gpio_get_level(loraWakePin) == HIGH) {
      delay(1);
      return;
    }
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(millis) * 1000ULL);
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable(loraWakePin, GPIO_INTR_HIGH_LEVEL);
    for (uint8_t index = 0; index < additionalCount; ++index) {
      const gpio_num_t pin = (gpio_num_t)additionalPins[index];
      gpio_wakeup_enable(pin, additionalLevels[index] ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
    }
    esp_light_sleep_start();
    gpio_wakeup_disable(loraWakePin);
    for (uint8_t index = 0; index < additionalCount; ++index)
      gpio_wakeup_disable((gpio_num_t)additionalPins[index]);
  }

  uint8_t getStartupReason() const override { return startup_reason; }

#if defined(P_LORA_TX_LED)
  void onBeforeTransmit() override {
    digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED on
  }
  void onAfterTransmit() override {
    digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED off
  }
#elif defined(P_LORA_TX_NEOPIXEL_LED)
  #define NEOPIXEL_BRIGHTNESS    64  // white brightness (max 255)

  void onBeforeTransmit() override {
    neopixelWrite(P_LORA_TX_NEOPIXEL_LED, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS, NEOPIXEL_BRIGHTNESS);   // turn TX neopixel on (White)
  }
  void onAfterTransmit() override {
    neopixelWrite(P_LORA_TX_NEOPIXEL_LED, 0, 0, 0);   // turn TX neopixel off
  }
#endif

  uint16_t getBattMilliVolts() override {
    #ifdef PIN_VBAT_READ
    analogReadResolution(12);

    uint32_t raw = 0;
    for (int i = 0; i < 4; i++) {
      raw += analogReadMilliVolts(PIN_VBAT_READ);
    }
    raw = raw / 4;

    return (2 * raw);
  #else
    return 0;  // not supported
  #endif
  }

  const char* getManufacturerName() const override {
    return "Generic ESP32";
  }

  void reboot() override {
    esp_restart();
  }

  bool startOTAUpdate(const char* id, char reply[]) override;

  void setInhibitSleep(bool inhibit) {
    inhibit_sleep = inhibit;
  }

  uint32_t getResetReason() const override {
    return esp_reset_reason();
  }

  // https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/system.html
  const char* getResetReasonString(uint32_t reason) {
    switch (reason) {
      case ESP_RST_UNKNOWN:
        return "Unknown or first boot";
      case ESP_RST_POWERON:
        return "Power-on reset";
      case ESP_RST_EXT:
        return "External reset";
      case ESP_RST_SW:
        return "Software reset";
      case ESP_RST_PANIC:
        return "Panic / exception reset";
      case ESP_RST_INT_WDT:
        return "Interrupt watchdog reset";
      case ESP_RST_TASK_WDT:
        return "Task watchdog reset";
      case ESP_RST_WDT:
        return "Other watchdog reset";
      case ESP_RST_DEEPSLEEP:
        return "Wake from deep sleep";
      case ESP_RST_BROWNOUT:
        return "Brownout (low voltage)";
      case ESP_RST_SDIO:
        return "SDIO reset";
      default:
        static char buf[40];
        snprintf(buf, sizeof(buf), "Unknown reset reason (%d)", reason);
        return buf;
    }
  }
};

class ESP32RTCClock : public mesh::RTCClock {
public:
  ESP32RTCClock() { }
  void begin() {
    esp_reset_reason_t reason = esp_reset_reason();
    if (reason == ESP_RST_POWERON) {
      // start with some date/time in the recent past
      struct timeval tv;
      tv.tv_sec = 1715770351;  // 15 May 2024, 8:50pm
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
  }
  uint32_t getCurrentTime() override {
    time_t _now;
    time(&_now);
    return _now;
  }
  void setCurrentTime(uint32_t time) override {
    struct timeval tv;
    tv.tv_sec = time;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
  }
};

#endif
