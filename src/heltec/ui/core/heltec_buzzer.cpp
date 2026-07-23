#include "heltec_buzzer.h"

#ifdef PIN_BUZZER

#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
#include <esp32-hal-ledc.h>
namespace {
constexpr uint8_t kBuzzerLedcChannel = 0;
constexpr uint16_t kBuzzerDutyByLevel[] = {0, 128, 288, 511};
}
#endif

void HeltecBuzzer::begin() {
#ifdef PIN_BUZZER_EN
  pinMode(PIN_BUZZER_EN, OUTPUT);
  digitalWrite(PIN_BUZZER_EN, HIGH);
#endif
  quiet(false);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

void HeltecBuzzer::play(const char* melody) {
  if (isPlaying()) rtttl::stop();
  if (_is_quiet) return;
  rtttl::begin(PIN_BUZZER, melody);
#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
  _pwm_initialized = true;
#endif
}

bool HeltecBuzzer::isPlaying() { return rtttl::isPlaying(); }

void HeltecBuzzer::loop() {
  if (!rtttl::done()) {
    rtttl::play();
#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
    if (rtttl::done()) stopPwmOutput();
    else applyPwmVolume();
#endif
  }
}

void HeltecBuzzer::startup() { play(startup_song); }
void HeltecBuzzer::shutdown() { play(shutdown_song); }

void HeltecBuzzer::quiet(bool buzzer_state) {
  _is_quiet = buzzer_state;
#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
  if (_is_quiet) {
    if (rtttl::isPlaying()) rtttl::stop();
    stopPwmOutput();
  }
#endif
#ifdef PIN_BUZZER_EN
  digitalWrite(PIN_BUZZER_EN, _is_quiet ? LOW : HIGH);
#endif
}

bool HeltecBuzzer::isQuiet() { return _is_quiet; }

#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
void HeltecBuzzer::setVolumeLevel(uint8_t level) {
  _volume_level = level > 3 ? 3 : level;
  if (_volume_level == 0) stopPwmOutput();
}

void HeltecBuzzer::applyPwmVolume() {
  if (!_pwm_initialized || _volume_level == 0 || !rtttl::isPlaying()) return;
  if (ledcReadFreq(kBuzzerLedcChannel) == 0) return;
  ledcWrite(kBuzzerLedcChannel, kBuzzerDutyByLevel[_volume_level]);
}

void HeltecBuzzer::stopPwmOutput() {
  if (_pwm_initialized) ledcWrite(kBuzzerLedcChannel, 0);
}
#endif

#endif
