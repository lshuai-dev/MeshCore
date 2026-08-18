/*
 * Standalone Arduino power fixture for the ESP32-P4 board shown in the
 * schematic.  This sketch is intentionally independent of MeshCore.
 *
 * Serial commands (115200 baud):
 *   min     - switch off board peripherals and enter indefinite deep sleep
 *   max     - switch on board peripherals and run all available CPU cores
 *   idle    - leave board rails in their safe/off state
 *   status  - print CPU/FreeRTOS and TCA9555 status
 *
 * The schematic shows GPIO53/54 connected to TCA9555 U26 at address 0x20.
 * TCA output polarities are macros below; verify them against the power-tree
 * sheet before connecting a current analyser.
 */

#include <Arduino.h>
#include <driver/gpio.h>
#include <SPI.h>
#include <Wire.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <soc/soc_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// -------------------------- board configuration --------------------------
#define BOARD_I2C_SDA 53       // schematic: GPIO53 -> I2C_SDA
#define BOARD_I2C_SCL 54       // schematic: GPIO54 -> I2C_SCL
#define TCA9555_ADDRESS 0x20  // U26, A0/A1/A2 strapped low in the schematic

// Direct MCU controls found in the power/C6 sheets.
#define BOARD_VEXT_EN_GPIO 8   // VE_CTRL -> U8 VEXT regulator EN
#define BOARD_C6_EN_GPIO 45    // CHIP_EN -> external ESP32-C6 EN
#define BOARD_STATUS_LED_GPIO 32

// SX1262 connections from the LoRa sheet.
#define LORA_NSS_GPIO  11
#define LORA_SCK_GPIO  12
#define LORA_MOSI_GPIO 13
#define LORA_MISO_GPIO 14
#define LORA_DIO1_GPIO 15
#define LORA_BUSY_GPIO 16
#define LORA_RST_GPIO  17

#ifndef POWER_TEST_MAX_CPU_MHZ
#define POWER_TEST_MAX_CPU_MHZ 360
#endif

// 0 = wait for serial command, 1 = MIN on boot, 2 = MAX on boot.
#ifndef POWER_TEST_BOOT_MODE
#define POWER_TEST_BOOT_MODE 0
#endif

#ifndef POWER_TEST_STARTUP_DELAY_MS
#define POWER_TEST_STARTUP_DELAY_MS 1000
#endif

// TCA9555 P0/P1 bit mapping. P10 means TCA port P1.0, not MCU GPIO10.
constexpr uint16_t TCA_AU_CTRL   = 1u << 4;   // P4
constexpr uint16_t TCA_VFEM_EN   = 1u << 5;   // P5
constexpr uint16_t TCA_TFT_LEDK  = 1u << 6;   // P6
constexpr uint16_t TCA_TFT_CTRL  = 1u << 7;   // P7
constexpr uint16_t TCA_PA_EN     = 1u << 8;   // P10
constexpr uint16_t TCA_GPS_CTRL  = 1u << 9;   // P11
constexpr uint16_t TCA_TP_RST    = 1u << 10;  // P12
constexpr uint16_t TCA_SD_PWRN   = 1u << 14;  // P16
constexpr uint16_t TCA_SDA_EN    = 1u << 15;  // P17

// Set each signal's active level according to the complete schematic.
// Most controls are active-high. SD_PWRN is checked against the transistor
// polarity in the SD power sheet below rather than inferred from its name.
#define AU_CTRL_ACTIVE  HIGH
#define VFEM_EN_ACTIVE  HIGH
#define TFT_LEDK_ACTIVE HIGH
#define TFT_CTRL_ACTIVE HIGH
#define PA_EN_ACTIVE    HIGH
#define GPS_CTRL_ACTIVE HIGH
#define TP_RST_ACTIVE   LOW
#define SD_PWRN_ACTIVE  HIGH    // verify Q5 polarity on the SD power sheet
#define SDA_EN_ACTIVE   HIGH
#define VEXT_EN_ACTIVE  HIGH
#define C6_EN_ACTIVE    HIGH
#define STATUS_LED_ACTIVE HIGH

enum class PowerMode : uint8_t { Idle, Minimum, Maximum };
static PowerMode currentMode = PowerMode::Idle;
static volatile uint32_t loadCounter = 0;
static uint16_t tcaOutput = 0;
static bool tcaPresent = false;

class Sx1262Power {
 public:
  void begin() {
    pinMode(LORA_NSS_GPIO, OUTPUT);
    pinMode(LORA_BUSY_GPIO, INPUT);
    pinMode(LORA_RST_GPIO, OUTPUT);
    digitalWrite(LORA_NSS_GPIO, HIGH);
    digitalWrite(LORA_RST_GPIO, HIGH);
    SPI.begin(LORA_SCK_GPIO, LORA_MISO_GPIO, LORA_MOSI_GPIO, LORA_NSS_GPIO);
  }

  void sleep() {
    if (!waitBusyLow(100)) return;
    // SX126x SetSleep opcode. 0x00 selects sleep without an RTC wake source.
    command(0x84, 0x00);
  }

  void wakeToStandby() {
    // SX126x wakes when NSS is asserted. A reset pulse makes this deterministic
    // after a previous deep-sleep measurement or an interrupted transaction.
    digitalWrite(LORA_NSS_GPIO, LOW);
    delayMicroseconds(20);
    digitalWrite(LORA_NSS_GPIO, HIGH);
    delay(2);
    digitalWrite(LORA_RST_GPIO, LOW);
    delay(2);
    digitalWrite(LORA_RST_GPIO, HIGH);
    delay(5);
    if (!waitBusyLow(100)) return;
    // SetStandby, RC oscillator (0x00).
    command(0x80, 0x00);
  }

 private:
  bool waitBusyLow(uint32_t timeoutMs) {
    const uint32_t start = millis();
    while (digitalRead(LORA_BUSY_GPIO) == HIGH) {
      if (millis() - start >= timeoutMs) return false;
      delayMicroseconds(50);
    }
    return true;
  }

  void command(uint8_t opcode, uint8_t argument) {
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(LORA_NSS_GPIO, LOW);
    SPI.transfer(opcode);
    SPI.transfer(argument);
    digitalWrite(LORA_NSS_GPIO, HIGH);
    SPI.endTransaction();
  }
};

static Sx1262Power loraPower;

class Tca9555 {
 public:
  explicit Tca9555(TwoWire& wire) : bus(wire) {}

  bool begin() {
    bus.begin(BOARD_I2C_SDA, BOARD_I2C_SCL, 400000);
    bus.beginTransmission(TCA9555_ADDRESS);
    if (bus.endTransmission() != 0) return false;

    // Write the safe latch before changing pins to outputs, then configure
    // only the nine board-control pins as outputs. All other TCA pins remain
    // inputs (battery/interrupt signals).
    write16(0x02, 0);
    write16(0x06, static_cast<uint16_t>(~kOutputMask));
    tcaOutput = 0;
    return true;
  }

  void set(uint16_t mask, bool on) {
    if (on) tcaOutput |= mask;
    else tcaOutput &= static_cast<uint16_t>(~mask);
    write16(0x02, tcaOutput);
  }

  uint16_t readInputs() {
    uint16_t value = 0;
    if (!read16(0x00, value)) return 0xffff;
    return value;
  }

  static constexpr uint16_t kOutputMask =
      TCA_AU_CTRL | TCA_VFEM_EN | TCA_TFT_LEDK | TCA_TFT_CTRL |
      TCA_PA_EN | TCA_GPS_CTRL | TCA_TP_RST | TCA_SD_PWRN | TCA_SDA_EN;

 private:
  TwoWire& bus;

  bool write16(uint8_t reg, uint16_t value) {
    bus.beginTransmission(TCA9555_ADDRESS);
    bus.write(reg);
    bus.write(static_cast<uint8_t>(value));
    bus.write(static_cast<uint8_t>(value >> 8));
    return bus.endTransmission() == 0;
  }

  bool read16(uint8_t reg, uint16_t& value) {
    bus.beginTransmission(TCA9555_ADDRESS);
    bus.write(reg);
    if (bus.endTransmission(false) != 0) return false;
    if (bus.requestFrom(TCA9555_ADDRESS, 2u) != 2u) return false;
    value = static_cast<uint16_t>(bus.read()) |
            (static_cast<uint16_t>(bus.read()) << 8);
    return true;
  }
};

static Tca9555 boardExpander(Wire);

static void configureDirectControls() {
  pinMode(BOARD_VEXT_EN_GPIO, OUTPUT);
  pinMode(BOARD_C6_EN_GPIO, OUTPUT);
  pinMode(BOARD_STATUS_LED_GPIO, OUTPUT);

  // VEXT must be available before U26 can be accessed over I2C. The rails are
  // shut down again by enterIdle()/enterMinimumPower() when requested.
  digitalWrite(BOARD_VEXT_EN_GPIO, VEXT_EN_ACTIVE);
  digitalWrite(BOARD_C6_EN_GPIO, C6_EN_ACTIVE == HIGH ? LOW : HIGH);
  digitalWrite(BOARD_STATUS_LED_GPIO, STATUS_LED_ACTIVE == HIGH ? LOW : HIGH);
  delay(5);
}

static void setBoardRails(bool enabled) {
  // U8 is the upstream VEXT rail. Turn it on before talking to U26. When
  // disabling, keep it on until all downstream controls have been written.
  if (enabled) {
    digitalWrite(BOARD_VEXT_EN_GPIO, VEXT_EN_ACTIVE);
    delay(5);
  }
  digitalWrite(BOARD_C6_EN_GPIO,
               enabled ? C6_EN_ACTIVE
                       : (C6_EN_ACTIVE == HIGH ? LOW : HIGH));
  digitalWrite(BOARD_STATUS_LED_GPIO,
               enabled ? STATUS_LED_ACTIVE
                       : (STATUS_LED_ACTIVE == HIGH ? LOW : HIGH));
  if (!tcaPresent) {
    if (!enabled) {
      digitalWrite(BOARD_VEXT_EN_GPIO,
                   VEXT_EN_ACTIVE == HIGH ? LOW : HIGH);
    }
    return;
  }
  // For a signal with active level HIGH, enabled=true writes 1 and enabled=
  // false writes 0. For active-low signals the values are inverted.
  auto setSignal = [](uint16_t mask, bool functionEnabled, uint8_t active) {
    const bool rawLevel = functionEnabled ? (active == HIGH) : (active == LOW);
    boardExpander.set(mask, rawLevel);
  };

  setSignal(TCA_AU_CTRL,  enabled, AU_CTRL_ACTIVE);
  setSignal(TCA_VFEM_EN,  enabled, VFEM_EN_ACTIVE);
  setSignal(TCA_TFT_LEDK, enabled, TFT_LEDK_ACTIVE);
  setSignal(TCA_TFT_CTRL, enabled, TFT_CTRL_ACTIVE);
  setSignal(TCA_PA_EN,    enabled, PA_EN_ACTIVE);
  setSignal(TCA_GPS_CTRL, enabled, GPS_CTRL_ACTIVE);

  // TP_RST is asserted in MIN and released in MAX.
  setSignal(TCA_TP_RST, !enabled, TP_RST_ACTIVE);
  // SD_PWRN is a power-down input, so it is asserted in MIN.
  setSignal(TCA_SD_PWRN, !enabled, SD_PWRN_ACTIVE);
  setSignal(TCA_SDA_EN, enabled, SDA_EN_ACTIVE);

  if (!enabled) {
    // VEXT also powers U26, so no I2C transaction must occur after this point.
    digitalWrite(BOARD_VEXT_EN_GPIO,
                 VEXT_EN_ACTIVE == HIGH ? LOW : HIGH);
    delay(2);
  }
}

static void setCpuMaximum() {
  setCpuFrequencyMhz(POWER_TEST_MAX_CPU_MHZ);
}

static void setMinimumPowerGpios() {
  // Board power-control pins must retain a defined OFF level. All other
  // valid GPIOs are put in Arduino-ESP32 ANALOG mode, which maps to
  // GPIO_MODE_DISABLE with pull-ups and pull-downs disabled.
  for (int pin = 0; pin < GPIO_NUM_MAX; ++pin) {
    if (!GPIO_IS_VALID_GPIO(pin)) continue;

    if (pin == BOARD_VEXT_EN_GPIO ||
        pin == BOARD_C6_EN_GPIO ||
        pin == BOARD_STATUS_LED_GPIO) {
      continue;
    }

    // Keep SX1262 NSS and RESET at their inactive levels; leaving either
    // floating can select or reset the radio while its VDD remains present.
    if (pin == LORA_NSS_GPIO) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      continue;
    }
    if (pin == LORA_RST_GPIO) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      continue;
    }

    gpio_hold_dis(static_cast<gpio_num_t>(pin));
    pinMode(pin, ANALOG);
  }

  // Re-assert the direct board controls after configuring the other GPIOs.
  pinMode(BOARD_VEXT_EN_GPIO, OUTPUT);
  digitalWrite(BOARD_VEXT_EN_GPIO,
               VEXT_EN_ACTIVE == HIGH ? LOW : HIGH);
  pinMode(BOARD_C6_EN_GPIO, OUTPUT);
  digitalWrite(BOARD_C6_EN_GPIO,
               C6_EN_ACTIVE == HIGH ? LOW : HIGH);
  pinMode(BOARD_STATUS_LED_GPIO, OUTPUT);
  digitalWrite(BOARD_STATUS_LED_GPIO,
               STATUS_LED_ACTIVE == HIGH ? LOW : HIGH);
}

static void maximumWorker(void*) {
  for (;;) loadCounter = loadCounter * 1664525UL + 1013904223UL;
}

static void runMaximumLoad() {
#if defined(CONFIG_FREERTOS_NUMBER_OF_CORES) && CONFIG_FREERTOS_NUMBER_OF_CORES > 1
  xTaskCreatePinnedToCore(maximumWorker, "power_max_cpu1", 2048, nullptr,
                          configMAX_PRIORITIES - 1, nullptr, 1);
#endif
  uint32_t lastWdt = millis();
  for (;;) {
    loadCounter = loadCounter * 1664525UL + 1013904223UL;
    if (millis() - lastWdt >= 1000) {
      esp_task_wdt_reset();
      lastWdt = millis();
    }
  }
}

static void enterMinimumPower() {
  // VDD_3V3 remains present on the schematic, so explicitly put SX1262 into
  // its own sleep state before disabling the board-level peripheral rails.
  loraPower.sleep();
  setBoardRails(false);
  Serial.println("POWER_MODE=MIN; entering indefinite deep sleep");
  Serial.flush();
  Serial.end();
  SPI.end();
  Wire.end();
  setMinimumPowerGpios();

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
#if SOC_PM_SUPPORT_RTC_PERIPH_PD
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
#endif

  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

#if SOC_PM_SUPPORT_RC_FAST_PD
  esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_OFF);
#endif

#if SOC_PM_SUPPORT_CPU_PD && \
    (!defined(CONFIG_ESP32P4_SELECTS_REV_LESS_V3) || \
     !CONFIG_ESP32P4_SELECTS_REV_LESS_V3)
  esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);
#endif

#if SOC_PM_SUPPORT_VDDSDIO_PD
  // This turns off the Flash/PSRAM-related VDD_SDIO domain for the current
  // test. Deep-sleep wake is a reboot, so the memory can be powered down.
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
#endif

#if SOC_PM_SUPPORT_TOP_PD
  esp_sleep_pd_config(ESP_PD_DOMAIN_TOP, ESP_PD_OPTION_OFF);
#endif

#if SOC_PM_SUPPORT_CNNT_PD
  // USB/high-speed connectivity power domain.
  esp_sleep_pd_config(ESP_PD_DOMAIN_CNNT, ESP_PD_OPTION_OFF);
#endif

  esp_deep_sleep_start();
  for (;;) {}
}

static void enterIdle() {
  currentMode = PowerMode::Idle;
  setBoardRails(false);
  Serial.println("POWER_MODE=IDLE; commands: min, max, idle, status");
}

static void printStatus() {
  Serial.printf("mode=%s cpu=%uMHz cores=%d reset=%d tca=%s inputs=0x%04X\n",
                currentMode == PowerMode::Maximum ? "MAX" :
                currentMode == PowerMode::Minimum ? "MIN" : "IDLE",
                getCpuFrequencyMhz(),
#ifdef CONFIG_FREERTOS_NUMBER_OF_CORES
                CONFIG_FREERTOS_NUMBER_OF_CORES,
#else
                -1,
#endif
                static_cast<int>(esp_reset_reason()),
                tcaPresent ? "OK" : "MISSING",
                tcaPresent ? boardExpander.readInputs() : 0xffff);
}

static void enterMaximum() {
  currentMode = PowerMode::Maximum;
  loraPower.wakeToStandby();
  setCpuMaximum();
  setBoardRails(true);
  Serial.printf("POWER_MODE=MAX; cpu=%uMHz\n", getCpuFrequencyMhz());
  Serial.println("MAX mode active; reset the board to leave it");
  runMaximumLoad();
}

static void handleCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command == "min" || command == "sleep" || command == "1") enterMinimumPower();
  else if (command == "max" || command == "busy" || command == "2") enterMaximum();
  else if (command == "idle" || command == "0") enterIdle();
  else if (command == "status" || command == "?") printStatus();
  else if (command.length()) Serial.println("Use: min, max, idle, status");
}

void setup() {
  Serial.begin(115200);
  // Do not wait for `Serial` to become true. On ESP32-P4 Arduino Core builds
  // with CDCOnBoot disabled (or with no host attached), its bool state can
  // remain false and block the power test. A fixed delay is sufficient for
  // USB enumeration and never prevents unattended MIN/MAX measurements.
  delay(POWER_TEST_STARTUP_DELAY_MS);
  Serial.println("BOOT: serial online");
  configureDirectControls();
  loraPower.begin();
  tcaPresent = boardExpander.begin();
  Serial.println("\nESP32-P4 board power test fixture");
  Serial.printf("TCA9555: %s, CPU target: %uMHz\n",
                tcaPresent ? "found" : "not found", POWER_TEST_MAX_CPU_MHZ);
#if POWER_TEST_BOOT_MODE == 1
  enterMinimumPower();
#elif POWER_TEST_BOOT_MODE == 2
  enterMaximum();
#else
  enterIdle();
#endif
}

void loop() {
  static String command;
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (command.length()) { handleCommand(command); command = ""; }
    } else if (command.length() < 32) command += c;
  }
  delay(2);
}
