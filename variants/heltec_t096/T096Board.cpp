#include "T096Board.h"

#include <Arduino.h>
#include <Wire.h>

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI && defined(HELTEC_DISPLAY_ST7735)
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

void T096Board::initiateShutdown(uint8_t reason) {
#if ENV_INCLUDE_GPS == 1
  pinMode(PIN_GPS_EN, OUTPUT);
  digitalWrite(PIN_GPS_EN, !PIN_GPS_EN_ACTIVE);
#endif
  variant_shutdown();

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

void T096Board::begin() {
  NRF52Board::begin();

#ifdef NRF52_POWER_MANAGEMENT
  // Boot voltage protection check (may not return if voltage too low)
  checkBootVoltage(&power_config);
#endif

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
#endif

  Wire.begin();

  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, LOW);

  periph_power.begin();
  loRaFEMControl.init();
  delay(1);

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI && defined(HELTEC_DISPLAY_ST7735)
  periph_power.claim();
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

void T096Board::onBeforeTransmit() {
    digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED on
    loRaFEMControl.setTxModeEnable();
}

void T096Board::onAfterTransmit() {
    digitalWrite(P_LORA_TX_LED, LOW);   //turn TX LED off
    loRaFEMControl.setRxModeEnable();
}

bool T096Board::setLNAEnable(bool enabled) {
    loRaFEMControl.setLNAEnable(enabled);
    loRaFEMControl.setRxModeEnable();
    return true;
}

uint16_t T096Board::getBattMilliVolts() {
    int adcvalue = 0;
    analogReadResolution(12);
    analogReference(AR_INTERNAL_3_0);
    pinMode(PIN_VBAT_READ, INPUT);
    pinMode(PIN_BAT_CTL, OUTPUT);
    digitalWrite(PIN_BAT_CTL, 1);

    delay(10);
    adcvalue = analogRead(PIN_VBAT_READ);
    digitalWrite(PIN_BAT_CTL, 0);

    return (uint16_t)((float)adcvalue * MV_LSB * 4.9);
}
void T096Board::variant_shutdown() {
 nrf_gpio_cfg_default(PIN_VEXT_EN);
    nrf_gpio_cfg_default(PIN_TFT_CS);
    nrf_gpio_cfg_default(PIN_TFT_DC);
    nrf_gpio_cfg_default(PIN_TFT_SDA);
    nrf_gpio_cfg_default(PIN_TFT_SCL);
    nrf_gpio_cfg_default(PIN_TFT_RST);
    nrf_gpio_cfg_default(PIN_TFT_LEDA_CTL);

    nrf_gpio_cfg_default(PIN_LED);

    nrf_gpio_cfg_default(P_LORA_KCT8103L_PA_CSD);
    nrf_gpio_cfg_default(P_LORA_KCT8103L_PA_CTX);
    pinMode(P_LORA_PA_POWER, OUTPUT);
    digitalWrite(P_LORA_PA_POWER, LOW);

    digitalWrite(PIN_BAT_CTL, LOW);
    nrf_gpio_cfg_default(LORA_CS);
    nrf_gpio_cfg_default(SX126X_DIO1);
    nrf_gpio_cfg_default(SX126X_BUSY);
    nrf_gpio_cfg_default(SX126X_RESET);

    nrf_gpio_cfg_default(PIN_SPI_MISO);
    nrf_gpio_cfg_default(PIN_SPI_MOSI);
    nrf_gpio_cfg_default(PIN_SPI_SCK);

    // nrf_gpio_cfg_default(PIN_GPS_PPS);
    nrf_gpio_cfg_default(PIN_GPS_RESET);
    nrf_gpio_cfg_default(PIN_GPS_EN);
    nrf_gpio_cfg_default(PIN_GPS_RX);
    nrf_gpio_cfg_default(PIN_GPS_TX);
}

void T096Board::powerOff() {
#if ENV_INCLUDE_GPS == 1
    pinMode(PIN_GPS_EN, OUTPUT);
    digitalWrite(PIN_GPS_EN, !PIN_GPS_EN_ACTIVE);
#endif
    loRaFEMControl.setSleepModeEnable();
    variant_shutdown();
    sd_power_system_off();
}

const char* T096Board::getManufacturerName() const {
  return "Heltec T096";
}
