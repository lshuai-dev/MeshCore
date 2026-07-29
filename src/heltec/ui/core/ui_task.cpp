#include "ui_task.hpp"

#include "heltec/app/HeltecMesh.h"
#include "heltec/core/board_features.hpp"
#include "ui/core/ui_host.hpp"
#include "ui/core/app_state_notifier.hpp"
#include <Arduino.h>
#include <lvgl.h>
#include "heltec/drivers/input/touch_input.hpp"

extern HeltecMesh the_mesh;

namespace heltec::meshcore::ui {

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
#if !defined(PIN_BUZZER)
#error "HAS_BUZZER_VOLUME_CONTROL requires PIN_BUZZER"
#endif
#if !(defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL) && \
    (!defined(PIN_BUZZER_VOLTAGE_MULTIPLIER_1) || \
     !defined(PIN_BUZZER_VOLTAGE_MULTIPLIER_2))
#error "HAS_BUZZER_VOLUME_CONTROL requires PIN_BUZZER and both voltage multiplier pins"
#endif

static void apply_buzzer_volume_level(HeltecBuzzer& buzzer, uint8_t level) {
  if (level > 3) level = 3;
#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
  buzzer.setVolumeLevel(level);
#else
  (void)buzzer;
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, (level & 0x02U) ? HIGH : LOW);
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, (level & 0x01U) ? HIGH : LOW);
#endif
}
#endif

#ifdef PIN_BUZZER
static void ensure_buzzer_hw() {
#if defined(PIN_BUZZER_VOLTAGE_MULTIPLIER_1)
  pinMode(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, OUTPUT);
#if !(defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL)
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_1, HIGH);
#endif
#endif
#if defined(PIN_BUZZER_VOLTAGE_MULTIPLIER_2)
  pinMode(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, OUTPUT);
#if !(defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL)
  digitalWrite(PIN_BUZZER_VOLTAGE_MULTIPLIER_2, HIGH);
#endif
#endif
}
#endif

#ifndef PREVIEW_AUTO_DISMISS_MS
#define PREVIEW_AUTO_DISMISS_MS 15000
#endif

#ifndef UI_BATT_POLL_MS
#define UI_BATT_POLL_MS 10000
#endif

#if defined(MESH_DEBUG) && MESH_DEBUG
#define UI_TASK_ALERT_LOG(fmt, ...) \
  do { \
    Serial.printf("[alert:task] " fmt "\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)
#else
#define UI_TASK_ALERT_LOG(fmt, ...) ((void)0)
#endif

#ifdef PIN_STATUS_LED
#ifndef LED_ON_MILLIS
#define LED_ON_MILLIS 20
#endif
#ifndef LED_ON_MSG_MILLIS
#define LED_ON_MSG_MILLIS 200
#endif
#ifndef LED_CYCLE_MILLIS
#define LED_CYCLE_MILLIS 4000
#endif
#ifndef LED_STATE_ON
#define LED_STATE_ON 1
#endif
#endif

void UiTask::attachHost(IUiHost* host) {
  // UiTask and IUiHost run on the same cooperative Arduino loop. A future
  // concurrent producer must marshal at its own task or ISR boundary.
  _ui_host = host;
}

void UiTask::pollBattery() {
  const uint32_t now = millis();
  if (_batt_last_read_ms != 0 && (uint32_t)(now - _batt_last_read_ms) < UI_BATT_POLL_MS) {
    return;
  }
  const uint16_t prev = _batt_mv;
  _batt_mv = _board ? _board->getBattMilliVolts() : 0;
  _batt_last_read_ms = now;
  if (_batt_mv != prev) {
    AppStateEvent ev{};
    ev.type = AppStateEventType::BatteryChanged;
    ev.battery.millivolts = _batt_mv;
    ev.battery.percent = 0;
    ev.battery.charging = false;
    app_state_notifier().notify(ev);
  }
}

#ifdef PIN_STATUS_LED
void UiTask::pollStatusLed() {
  const uint32_t now = millis();
  if ((int32_t)(now - _status_led_next_ms) < 0) return;

  if (!_status_led_on) {
    _status_led_on = 1;
    _status_led_on_ms = (_msg_count > 0) ? (uint16_t)LED_ON_MSG_MILLIS : (uint16_t)LED_ON_MILLIS;
    _status_led_next_ms = now + _status_led_on_ms;
  } else {
    _status_led_on = 0;
    _status_led_next_ms = now + (uint32_t)(LED_CYCLE_MILLIS - _status_led_on_ms);
  }
  digitalWrite(PIN_STATUS_LED, _status_led_on ? LED_STATE_ON : !LED_STATE_ON);
}
#endif

void UiTask::begin(NodePrefs* node_prefs) {
  _node_prefs = node_prefs;
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  if (_node_prefs && (!isLnaCanControl() ||
      !heltec::meshcore::board::setLnaEnable(_node_prefs->lna_enabled != 0))) {
    _node_prefs->lna_enabled = 0;
  }
#endif
  AppStateEvent config_ev{};
  config_ev.type = AppStateEventType::ConfigChanged;
  app_state_notifier().notify(config_ev);
  pollBattery();
#if HELTEC_TOUCH_INPUT
  if (lv_disp_t* d = lv_disp_get_default()) {
    heltec::meshcore::dal::touch_input::armInit((uint16_t)lv_disp_get_hor_res(d),
                                                (uint16_t)lv_disp_get_ver_res(d));
  }
#endif
#ifdef PIN_BUZZER
  ensure_buzzer_hw();
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  apply_buzzer_volume_level(_buzzer,
      (_node_prefs && _node_prefs->buzzer_quiet == 0) ? _node_prefs->buzzer_volume_level : 0);
#endif
  _buzzer.begin();
  if (_node_prefs) {
    _buzzer.quiet(_node_prefs->buzzer_quiet != 0);
  }
  _buzzer_startup_pending = !_buzzer.isQuiet();
#if defined(MESH_DEBUG) && MESH_DEBUG
  Serial.printf("[buzzer] begin quiet=%d pending_startup=%d t=%lu\n",
                _buzzer.isQuiet() ? 1 : 0, _buzzer_startup_pending ? 1 : 0,
                (unsigned long)millis());
#endif
#endif
#ifdef PIN_STATUS_LED
  pinMode(PIN_STATUS_LED, OUTPUT);
  _status_led_on = 0;
  _status_led_next_ms = 0;
  _status_led_on_ms = 0;
  digitalWrite(PIN_STATUS_LED, 0 == LED_STATE_ON);
#endif
}

void UiTask::msgRead(int msgcount) {
  // This callback tracks the companion app's volatile offline queue. Device
  // unread state is maintained separately by the persistent message history.
  (void)msgcount;
}

void UiTask::setMessageCount(int msgcount) {
  _msg_count = msgcount;
  AppStateEvent ev{};
  ev.type = AppStateEventType::UnreadMessageCountChanged;
  ev.unread.count = (msgcount < 0) ? 0 : (msgcount > 255 ? 255 : (uint8_t)msgcount);
  app_state_notifier().notify(ev);
}

void UiTask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
  _msg_count = msgcount;
  // Minimal preview capture for overlay. Keep short and ASCII-safe.
  _preview_timestamp_ms = millis();
  _preview_dismiss_at_ms = _preview_timestamp_ms + PREVIEW_AUTO_DISMISS_MS;
  if (_preview_unread < 255) _preview_unread++;
  if (!from_name) from_name = "";
  if (!text) text = "";
  if (path_len == 0xFF) {
    snprintf(_preview_origin, sizeof(_preview_origin), "(D) %s:", from_name);
  } else {
    snprintf(_preview_origin, sizeof(_preview_origin), "(%u) %s:", (unsigned)path_len, from_name);
  }
  snprintf(_preview_text, sizeof(_preview_text), "%s", text);

  AppStateEvent ev{};
  ev.type = AppStateEventType::UnreadMessageCountChanged;
  ev.unread.count = (msgcount < 0) ? 0 : (msgcount > 255 ? 255 : (uint8_t)msgcount);
  app_state_notifier().notify(ev);
  if (_ui_host && _ui_host->isReady()) {
    _ui_host->openPreviewOverlay(_preview_unread, previewAgeSeconds(), _preview_origin, _preview_text);
  }
}

void UiTask::notify(UIEventType t) {
#if defined(PIN_BUZZER) &&  PIN_BUZZER
  switch (t) {
    case UIEventType::contactMessage:
      _buzzer.play("MsgRcv3:d=4,o=6,b=200:32e,32g,32b,16c7");
      break;
    case UIEventType::channelMessage:
      _buzzer.play("kerplop:d=16,o=6,b=120:32g#,32c#");
      break;
    case UIEventType::ack:
      _buzzer.play("ack:d=32,o=8,b=120:c");
      break;
    case UIEventType::roomMessage:
    case UIEventType::newContactMessage:
    case UIEventType::none:
    default:
      break;
  }
#endif
}

void UiTask::setHasConnection(bool connected) {
  if (_connected == connected) return;
  AbstractUITask::setHasConnection(connected);

  AppStateEvent ev{};
  ev.type = AppStateEventType::CompanionChanged;
  ev.companion.connected = connected;
  ev.companion.pairing_pin = the_mesh.getBLEPin();
  app_state_notifier().notify(ev);
}

void UiTask::loop() {
  pollBattery();
#ifdef PIN_BUZZER
  if (_buzzer_startup_pending) {
    _buzzer_startup_pending = false;
    _buzzer.startup();
  }
  if (!rtttl::done()) {
    _buzzer.loop();
  } else if (rtttl::isPlaying()) {
    rtttl::stop();
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
  }
#endif
  if (_preview_unread > 0 && _preview_dismiss_at_ms != 0 &&
      (int32_t)(millis() - _preview_dismiss_at_ms) >= 0) {
    if (_ui_host) _ui_host->closePreviewOverlay();
  }
  const uint32_t alert_now = millis();
  if (_alert_expiry_ms != 0 && (int32_t)(alert_now - _alert_expiry_ms) >= 0) {
    UI_TASK_ALERT_LOG("expiry now=%lu expiry=%lu text=%s host=%p",
                      (unsigned long)alert_now,
                      (unsigned long)_alert_expiry_ms,
                      _alert,
                      _ui_host);
    if (_ui_host) _ui_host->closeAlertOverlay();
  }
#if HELTEC_TOUCH_INPUT
  heltec::meshcore::dal::touch_input::poll();
#endif
#ifdef PIN_STATUS_LED
  pollStatusLed();
#endif
}

bool UiTask::isBuzzerQuiet() const {
#ifdef PIN_BUZZER
  return _buzzer.isQuiet();
#else
  return true;
#endif
}

void UiTask::setBuzzerEnabled(bool enabled) {
#ifdef PIN_BUZZER
  const bool cur = !_buzzer.isQuiet();
  if (cur == enabled) return;
  if (enabled) {
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    uint8_t level = _node_prefs ? _node_prefs->buzzer_volume_level : 3;
    if (level < 1 || level > 3) level = 3;
    apply_buzzer_volume_level(_buzzer, level);
#endif
    _buzzer.quiet(false);
    notify(UIEventType::ack);
  } else {
    _buzzer.quiet(true);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    apply_buzzer_volume_level(_buzzer, 0);
#endif
  }
  if (_node_prefs) {
    _node_prefs->buzzer_quiet = _buzzer.isQuiet() ? 1 : 0;
    the_mesh.savePrefs();
  }
#else
  (void)enabled;
#endif
}

uint8_t UiTask::buzzerVolumeLevel() const {
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  if (!_node_prefs || _node_prefs->buzzer_quiet != 0) return 0;
  const uint8_t level = _node_prefs->buzzer_volume_level;
  return (level >= 1 && level <= 3) ? level : 3;
#else
  return 3;
#endif
}

void UiTask::setBuzzerVolumeLevel(uint8_t level) {
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  if (level > 3) level = 3;
  if (buzzerVolumeLevel() == level) return;

  if (level == 0) {
    _buzzer.quiet(true);
    apply_buzzer_volume_level(_buzzer, 0);
    if (_node_prefs) {
      _node_prefs->buzzer_quiet = 1;
      the_mesh.savePrefs();
    }
    return;
  }

  apply_buzzer_volume_level(_buzzer, level);
  _buzzer.quiet(false);
  if (_node_prefs) {
    _node_prefs->buzzer_volume_level = level;
    _node_prefs->buzzer_quiet = 0;
    the_mesh.savePrefs();
  }
  notify(UIEventType::ack);
#else
  (void)level;
#endif
}

bool UiTask::isLnaCanControl() const {
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  return heltec::meshcore::board::lnaCanControl();
#else
  return false;
#endif
}

bool UiTask::lnaEnabled() const {
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  return isLnaCanControl() && _node_prefs && _node_prefs->lna_enabled != 0;
#else
  return false;
#endif
}

bool UiTask::setLnaEnabled(bool enabled) {
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  if (!isLnaCanControl()) return false;
  if (lnaEnabled() == enabled) return true;
  if (!heltec::meshcore::board::setLnaEnable(enabled)) return false;
  if (_node_prefs) {
    _node_prefs->lna_enabled = enabled ? 1 : 0;
    the_mesh.savePrefs();
  }
  return true;
#else
  (void)enabled;
  return false;
#endif
}

void UiTask::playShutdownMelody() {
#ifdef PIN_BUZZER
  _buzzer.shutdown();
  const uint32_t started = millis();
  while (_buzzer.isPlaying() && (millis() - started) < 2500U) {
    _buzzer.loop();
  }
#endif
#ifdef PIN_STATUS_LED
  digitalWrite(PIN_STATUS_LED, 0 == LED_STATE_ON);
#endif
}

void UiTask::showAlert(const char* text, int duration_millis) {
  if (!text) {
    _alert[0] = '\0';
  } else {
    snprintf(_alert, sizeof(_alert), "%s", text);
  }
  const uint32_t now = millis();
  _alert_expiry_ms = (duration_millis > 0) ? (now + (uint32_t)duration_millis) : 0;
  UI_TASK_ALERT_LOG("show text=%s duration=%d now=%lu expiry=%lu host=%p ready=%d",
                    _alert,
                    duration_millis,
                    (unsigned long)now,
                    (unsigned long)_alert_expiry_ms,
                    _ui_host,
                    (_ui_host && _ui_host->isReady()) ? 1 : 0);

  if (_ui_host && _ui_host->isReady() && _alert[0] != '\0') {
    _ui_host->openAlertOverlay(_alert);
  }
}

bool UiTask::isAlertActive() const {
  return _alert_expiry_ms != 0 && (int32_t)(millis() - _alert_expiry_ms) < 0 &&
         _alert[0] != '\0';
}

void UiTask::dismissAlert() {
  UI_TASK_ALERT_LOG("dismiss text=%s expiry=%lu",
                    _alert,
                    (unsigned long)_alert_expiry_ms);
  _alert_expiry_ms = 0;
  _alert[0] = '\0';
}

uint32_t UiTask::previewAgeSeconds() const {
  if (_preview_timestamp_ms == 0) return 0;
  const uint32_t age_ms = millis() - _preview_timestamp_ms;
  return age_ms / 1000U;
}

void UiTask::dismissPreview() {
  _preview_unread = 0;
  _preview_timestamp_ms = 0;
  _preview_dismiss_at_ms = 0;
  _preview_origin[0] = '\0';
  _preview_text[0] = '\0';
}

}  // namespace heltec::meshcore::ui
