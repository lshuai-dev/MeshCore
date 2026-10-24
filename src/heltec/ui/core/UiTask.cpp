#include "UiTask.hpp"

#include "heltec/app/HeltecMesh.h"
#include "heltec/core/BoardFeatures.hpp"
#include "heltec/drivers/display/DisplayPort.hpp"
#include "heltec/drivers/input/MomentaryButton.hpp"
#include "heltec/drivers/input/TouchInput.hpp"
#include "ui/core/AppStateNotifier.hpp"

#include <Arduino.h>
#include <lvgl.h>

extern HeltecMesh the_mesh;

namespace Heltec::MeshCore::Ui {

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
#if !defined(PIN_BUZZER)
#error "HAS_BUZZER_VOLUME_CONTROL requires PIN_BUZZER"
#endif
#if !(defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL) &&      \
    (!defined(PIN_BUZZER_VOLTAGE_MULTIPLIER_1) || !defined(PIN_BUZZER_VOLTAGE_MULTIPLIER_2))
#error "HAS_BUZZER_VOLUME_CONTROL requires PIN_BUZZER and both voltage multiplier pins"
#endif

static void apply_buzzer_volume_level(HeltecBuzzer &buzzer, uint8_t level) {
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

#ifdef PIN_STATUS_LED
// Status LED feedback.
void UiTask::pollStatusLed() {
    const uint32_t now = millis();
    if ((int32_t)(now - _status_led_next_ms) < 0) return;

    if (!_status_led_on) {
        _status_led_on = 1;
        _status_led_on_ms =
            (_msg_count > 0) ? (uint16_t)LED_ON_MSG_MILLIS : (uint16_t)LED_ON_MILLIS;
        _status_led_next_ms = now + _status_led_on_ms;
    } else {
        _status_led_on = 0;
        _status_led_next_ms = now + (uint32_t)(LED_CYCLE_MILLIS - _status_led_on_ms);
    }
    digitalWrite(PIN_STATUS_LED, _status_led_on ? LED_STATE_ON : !LED_STATE_ON);
}
#endif

// Initialization.
void UiTask::begin(NodePrefs *node_prefs) {
    _node_prefs = node_prefs;
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
    if (_node_prefs && (!isLnaCanControl() ||
                        !Heltec::MeshCore::Board::setLnaEnable(_node_prefs->lna_enabled != 0))) {
        _node_prefs->lna_enabled = 0;
    }
#endif
    AppStateEvent config_ev{};
    config_ev.type = AppStateEventType::ConfigChanged;
    app_state_notifier().notify(config_ev);
#if HELTEC_TOUCH_INPUT
    if (lv_disp_t *d = lv_disp_get_default()) {
        Heltec::MeshCore::Dal::TouchInput::armInit((uint16_t)lv_disp_get_hor_res(d),
                                                    (uint16_t)lv_disp_get_ver_res(d));
    }
#endif
#ifdef PIN_BUZZER
    ensure_buzzer_hw();
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    apply_buzzer_volume_level(_buzzer, (_node_prefs && _node_prefs->buzzer_quiet == 0)
                                           ? _node_prefs->buzzer_volume_level
                                           : 0);
#endif
    _buzzer.begin();
    if (_node_prefs) {
        _buzzer.quiet(_node_prefs->buzzer_quiet != 0);
    }
    _buzzer_startup_pending = !_buzzer.isQuiet();
#if defined(MESH_DEBUG) && MESH_DEBUG
    Serial.printf("[buzzer] begin quiet=%d pending_startup=%d t=%lu\n", _buzzer.isQuiet() ? 1 : 0,
                  _buzzer_startup_pending ? 1 : 0, (unsigned long)millis());
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

// MeshCore message and connection callbacks.
void UiTask::msgRead(int msgcount) {
    setMessageCount(msgcount);
}

void UiTask::setMessageCount(int msgcount) {
    if (_msg_count != msgcount) {
        _msg_count = msgcount;
        AppStateEvent event{};
        event.type = AppStateEventType::UnreadMessageCountChanged;
        app_state_notifier().notify(event);
    }
    if (msgcount == 0) {
        dismissPreview();
    }
}

void UiTask::newMsg(uint8_t path_len, const char *from_name, const char *text, int msgcount) {
    setMessageCount(msgcount);
    _preview_head = static_cast<uint8_t>((_preview_head + 1U) % kPreviewCapacity);
    if (_preview_count < kPreviewCapacity) ++_preview_count;
    PreviewEntry &entry = _preview_entries[_preview_head];
    entry.received_ms = millis();
    if (!from_name) from_name = "";
    if (!text) text = "";
    if (path_len == 0xFF) {
        snprintf(entry.origin, sizeof(entry.origin), "(D) %s:", from_name);
    } else {
        snprintf(entry.origin, sizeof(entry.origin), "(%u) %s:", (unsigned)path_len, from_name);
    }
    snprintf(entry.text, sizeof(entry.text), "%s", text);
    AppStateEvent event{};
    event.type = AppStateEventType::PreviewChanged;
    app_state_notifier().notify(event);
}

// MeshCore notification feedback.
void UiTask::notify(UIEventType t) {
#if defined(PIN_BUZZER) && PIN_BUZZER
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
    app_state_notifier().notify(ev);
}

// Periodic hardware servicing.
void UiTask::loop() {
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
    const uint32_t alert_now = millis();
    if (_alert_expiry_ms != 0 && (int32_t)(alert_now - _alert_expiry_ms) >= 0) {
        dismissAlert();
    }
    if (!Heltec::MeshCore::Dal::DisplayPort::isBacklightOn())
    {
        // LVGL input polling is suspended with the display. Continue processing
        // GPIO edges so a completed button gesture can wake the display.
        Heltec::MeshCore::Dal::MomentaryButton::poll();
    }
#if HELTEC_TOUCH_INPUT
    if (Heltec::MeshCore::Dal::DisplayPort::isBacklightOn())
    {
        Heltec::MeshCore::Dal::TouchInput::poll();
    }
#endif
#ifdef PIN_STATUS_LED
    pollStatusLed();
#endif
}

// Buzzer settings and feedback.
bool UiTask::isBuzzerPlaying() {
#ifdef PIN_BUZZER
    return _buzzer.isPlaying();
#else
    return false;
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

// Board radio front-end control.
bool UiTask::isLnaCanControl() const {
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
    return Heltec::MeshCore::Board::lnaCanControl();
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
    if (!Heltec::MeshCore::Board::setLnaEnable(enabled)) return false;
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

// Shutdown feedback.
void UiTask::finishShutdownFeedback() {
    _shutdown_feedback_active = false;
#ifdef PIN_STATUS_LED
    digitalWrite(PIN_STATUS_LED, 0 == LED_STATE_ON);
#endif
}

void UiTask::startShutdownFeedback() {
    _shutdown_feedback_active = true;
#ifdef PIN_BUZZER
    _buzzer.shutdown();
    if (!_buzzer.isPlaying()) finishShutdownFeedback();
#else
    finishShutdownFeedback();
#endif
}

bool UiTask::shutdownFeedbackDone() {
#ifdef PIN_BUZZER
    if (_shutdown_feedback_active && _buzzer.isPlaying()) return false;
#endif
    if (_shutdown_feedback_active) finishShutdownFeedback();
    return true;
}

// Transient alert state consumed by UiApp.
void UiTask::showAlert(const char *text, int duration_millis) {
    if (!text) {
        _alert[0] = '\0';
    } else {
        snprintf(_alert, sizeof(_alert), "%s", text);
    }
    const uint32_t now = millis();
    _alert_expiry_ms = (duration_millis > 0) ? (now + (uint32_t)duration_millis) : 0;
    AppStateEvent event{};
    event.type = AppStateEventType::AlertChanged;
    app_state_notifier().notify(event);
}

void UiTask::playAlertFeedback() {
#ifdef PIN_BUZZER
    if (!_buzzer.isQuiet()) {
        _buzzer.play("alert:d=8,o=6,b=180: c,c");
    }
#endif
}

bool UiTask::isAlertActive() const {
    return _alert[0] != '\0' &&
           (_alert_expiry_ms == 0 || (int32_t)(millis() - _alert_expiry_ms) < 0);
}

void UiTask::dismissAlert() {
    if (_alert_expiry_ms == 0 && _alert[0] == '\0') return;
    _alert_expiry_ms = 0;
    _alert[0] = '\0';
    AppStateEvent event{};
    event.type = AppStateEventType::AlertChanged;
    app_state_notifier().notify(event);
}

// Volatile unread-message preview queue.
bool UiTask::advancePreview() {
    if (_preview_count == 0) return false;
    --_preview_count;
    if (_preview_count != 0) {
        _preview_head =
            static_cast<uint8_t>((_preview_head + kPreviewCapacity - 1U) % kPreviewCapacity);
    }
    AppStateEvent event{};
    event.type = AppStateEventType::PreviewChanged;
    app_state_notifier().notify(event);
    return _preview_count != 0;
}

void UiTask::dismissPreview() {
    if (_preview_count == 0) return;
    _preview_count = 0;
    _preview_head = kPreviewCapacity - 1;
    for (PreviewEntry &entry : _preview_entries)
        entry = PreviewEntry{};
    AppStateEvent event{};
    event.type = AppStateEventType::PreviewChanged;
    app_state_notifier().notify(event);
}

} // namespace Heltec::MeshCore::Ui
