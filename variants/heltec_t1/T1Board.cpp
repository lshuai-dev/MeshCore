#include "T1Board.h"

#include <Arduino.h>
#include <Wire.h>

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include <lvgl.h>
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

#ifdef NRF52_POWER_MANAGEMENT
const PowerMgtConfig power_config = {
  .lpcomp_ain_channel = PWRMGT_LPCOMP_AIN,
  .lpcomp_refsel = PWRMGT_LPCOMP_REFSEL,
  .voltage_bootlock = PWRMGT_VOLTAGE_BOOTLOCK
};

void T1Board::initiateShutdown(uint8_t reason) {
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
  _sensorPower.release();
  bool enable_lpcomp = (reason == SHUTDOWN_REASON_LOW_VOLTAGE ||
                        reason == SHUTDOWN_REASON_BOOT_PROTECT);
  pinMode(PIN_BAT_CTL, OUTPUT);
  digitalWrite(PIN_BAT_CTL, enable_lpcomp ? ADC_CTRL_ENABLED : !ADC_CTRL_ENABLED);
  if (enable_lpcomp) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }
  enterSystemOff(reason);
#else
  variant_shutdown();

  bool enable_lpcomp = (reason == SHUTDOWN_REASON_LOW_VOLTAGE ||
                        reason == SHUTDOWN_REASON_BOOT_PROTECT);
  pinMode(PIN_BAT_CTL, OUTPUT);
  digitalWrite(PIN_BAT_CTL, enable_lpcomp ? ADC_CTRL_ENABLED : !ADC_CTRL_ENABLED);

  if (enable_lpcomp) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }

  enterSystemOff(reason);
#endif
}
#endif

void T1Board::begin() {
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
  NRF52Board::begin();
  _sensorPower.begin();
  _sensorPower.claim();
  pinMode(PIN_VBAT_READ, INPUT);
#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
  Wire.setClock(100000);
  Wire.begin();
#endif

  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, !LED_STATE_ON);

#ifdef NRF52_POWER_MANAGEMENT
  checkBootVoltage(&power_config);
#endif

  delay(50);  // let TFT rail settle before SPI init
  (void)heltec::meshcore::dal::display_port::init();

  using heltec::meshcore::dal::momentary_button::Config;
  using heltec::meshcore::dal::momentary_button::KeyMap;

  Config cnf{};
  cnf.multi_click_window_ms = 350;
  cnf.long_press_ms = 1000;
  cnf.buttons[0].pin = BUTTON_PIN;
  cnf.buttons[0].pin_mode = INPUT_PULLUP;
  cnf.buttons[0].active_level = LOW;
  cnf.buttons[0].map = KeyMap(LV_KEY_PREV, LV_KEY_ESC, LV_KEY_LEFT, LV_KEY_ENTER);
  cnf.buttons[1].pin = PIN_USER_BTN;
  cnf.buttons[1].pin_mode = INPUT_PULLUP;
  cnf.buttons[1].active_level = LOW;
  cnf.buttons[1].map = KeyMap(LV_KEY_NEXT, LV_KEY_ESC, LV_KEY_RIGHT, LV_KEY_ENTER);
  heltec::meshcore::dal::momentary_button::configure(cnf);
  heltec::meshcore::dal::momentary_button::initialize();

#ifdef PIN_BUZZER
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
#ifdef PIN_BUZZER_VOLTAGE_MULTIPLIER_1
  pinMode(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, OUTPUT);
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, HIGH);
#endif
#ifdef PIN_BUZZER_VOLTAGE_MULTIPLIER_2
  pinMode(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, OUTPUT);
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, HIGH);
#endif
#endif
#else
  NRF52Board::begin();

#ifdef NRF52_POWER_MANAGEMENT
  checkBootVoltage(&power_config);
#endif

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
#endif
  Wire.begin();

  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, !LED_STATE_ON);

  pinMode(PIN_GPS_EN, OUTPUT);
  digitalWrite(PIN_GPS_EN, !PIN_GPS_EN_ACTIVE);

  pinMode(PIN_SENSOR_EN, OUTPUT);
  digitalWrite(PIN_SENSOR_EN, PIN_SENSOR_EN_ACTIVE);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, OUTPUT);
  pinMode(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, OUTPUT);
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, HIGH);
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, HIGH);

  periph_power.begin();
  delay(1);
#endif
}

void T1Board::onBeforeTransmit() {
  digitalWrite(P_LORA_TX_LED, LED_STATE_ON);
}

void T1Board::onAfterTransmit() {
  digitalWrite(P_LORA_TX_LED, !LED_STATE_ON);
}

uint16_t T1Board::getBattMilliVolts() {
  analogReadResolution(12);
  analogReference(VBAT_AR_INTERNAL);
  pinMode(PIN_VBAT_READ, INPUT);
  pinMode(PIN_BAT_CTL, OUTPUT);
  digitalWrite(PIN_BAT_CTL, ADC_CTRL_ENABLED);

  delay(10);
  int adcvalue = analogRead(PIN_VBAT_READ);
  digitalWrite(PIN_BAT_CTL, !ADC_CTRL_ENABLED);

  return (uint16_t)((float)adcvalue * MV_LSB * ADC_MULTIPLIER);
}

void T1Board::powerOff() {
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
  _sensorPower.release();
  sd_power_system_off();
#else
  variant_shutdown();
  sd_power_system_off();
#endif
}

const char* T1Board::getManufacturerName() const {
  return "Heltec T1";
}

#if !(defined(HELTEC_MESH_UI) && HELTEC_MESH_UI)
void T1Board::variant_shutdown() {
  nrf_gpio_cfg_default(PIN_TFT_CS);
  nrf_gpio_cfg_default(PIN_TFT_DC);
  nrf_gpio_cfg_default(PIN_TFT_SDA);
  nrf_gpio_cfg_default(PIN_TFT_SCL);
  nrf_gpio_cfg_default(PIN_TFT_RST);
  nrf_gpio_cfg_default(PIN_TFT_LEDA_CTL);
  nrf_gpio_cfg_default(PIN_TFT_VDD_CTL);

  nrf_gpio_cfg_default(PIN_WIRE_SDA);
  nrf_gpio_cfg_default(PIN_WIRE_SCL);

  nrf_gpio_cfg_default(LORA_CS);
  nrf_gpio_cfg_default(SX126X_DIO1);
  nrf_gpio_cfg_default(SX126X_BUSY);
  nrf_gpio_cfg_default(SX126X_RESET);
  nrf_gpio_cfg_default(PIN_SPI_MISO);
  nrf_gpio_cfg_default(PIN_SPI_MOSI);
  nrf_gpio_cfg_default(PIN_SPI_SCK);

  nrf_gpio_cfg_default(PIN_SPI1_MOSI);
  nrf_gpio_cfg_default(PIN_SPI1_SCK);

  nrf_gpio_cfg_default(PIN_GPS_RESET);
  nrf_gpio_cfg_default(PIN_GPS_EN);
  nrf_gpio_cfg_default(PIN_GPS_PPS);
  nrf_gpio_cfg_default(PIN_GPS_RX);
  nrf_gpio_cfg_default(PIN_GPS_TX);

  nrf_gpio_cfg_default(PIN_BUZZER_VOLTAGE_MULTIPLIER_1);
  nrf_gpio_cfg_default(PIN_BUZZER_VOLTAGE_MULTIPLIER_2);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_SENSOR_EN, OUTPUT);
  digitalWrite(PIN_SENSOR_EN, !PIN_SENSOR_EN_ACTIVE);

  pinMode(PIN_LED1, OUTPUT);
  digitalWrite(PIN_LED1, !LED_STATE_ON);

  pinMode(PIN_BAT_CTL, OUTPUT);
  digitalWrite(PIN_BAT_CTL, !ADC_CTRL_ENABLED);
}
#endif
