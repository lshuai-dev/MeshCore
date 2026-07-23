#include "T114Board.h"

#include <Arduino.h>
#include <Wire.h>

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include <lvgl.h>
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

#ifdef NRF52_POWER_MANAGEMENT
// Static configuration for power management
// Values come from variant.h defines
const PowerMgtConfig power_config = {
  .lpcomp_ain_channel = PWRMGT_LPCOMP_AIN,
  .lpcomp_refsel = PWRMGT_LPCOMP_REFSEL,
  .voltage_bootlock = PWRMGT_VOLTAGE_BOOTLOCK
};

void T114Board::initiateShutdown(uint8_t reason) {
#if ENV_INCLUDE_GPS == 1
  pinMode(GPS_EN, OUTPUT);
  digitalWrite(GPS_EN, LOW);
#endif
  digitalWrite(SX126X_POWER_EN, LOW);

  bool enable_lpcomp = (reason == SHUTDOWN_REASON_LOW_VOLTAGE ||
                        reason == SHUTDOWN_REASON_BOOT_PROTECT);
  pinMode(PIN_BAT_CTL, OUTPUT);
  digitalWrite(PIN_BAT_CTL, enable_lpcomp ? HIGH : LOW);

  if (enable_lpcomp) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }

  enterSystemOff(reason);
}
#endif // NRF52_POWER_MANAGEMENT

void T114Board::begin() {
  NRF52Board::begin();

  pinMode(PIN_VBAT_READ, INPUT);

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
#endif

  Wire.begin();

#ifdef P_LORA_TX_LED
  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, HIGH);
#endif

  pinMode(SX126X_POWER_EN, OUTPUT);
#ifdef NRF52_POWER_MANAGEMENT
  // Boot voltage protection check (may not return if voltage too low)
  // We need to call this after we configure SX126X_POWER_EN as output but before we pull high
  checkBootVoltage(&power_config);
#endif
  digitalWrite(SX126X_POWER_EN, HIGH);
  delay(10); // give sx1262 some time to power up

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
  delay(50);  // let TFT rail settle before SPI init
  (void)heltec::meshcore::dal::display_port::init();

  using heltec::meshcore::dal::momentary_button::Config;
  using heltec::meshcore::dal::momentary_button::KeyMap;

  Config cnf{};
  cnf.debounce_ms = 50;
  cnf.multi_click_window_ms = 350;
  cnf.long_press_ms = 1000;
  cnf.buttons[0].pin = PIN_USER_BTN;
  cnf.buttons[0].pin_mode = INPUT;
  cnf.buttons[0].active_level = LOW;
  cnf.buttons[0].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_PREV, LV_KEY_ENTER);
  heltec::meshcore::dal::momentary_button::configure(cnf);
  heltec::meshcore::dal::momentary_button::initialize();
#endif
}
