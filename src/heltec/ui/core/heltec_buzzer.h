#pragma once

#include <Arduino.h>
#ifdef PIN_BUZZER
#include <NonBlockingRtttl.h>

class HeltecBuzzer {
 public:
  void begin();
  void play(const char* melody);
  void loop();
  void startup();
  void shutdown();
  bool isPlaying();
  void quiet(bool buzzer_state);
  bool isQuiet();
#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
  void setVolumeLevel(uint8_t level);
#endif

 private:
  const char* startup_song = "Startup:d=4,o=5,b=160:16c6,16e6,8g6";
  const char* shutdown_song = "Shutdown:d=4,o=5,b=100:8g5,16e5,16c5";
  bool _is_quiet = true;
#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
  uint8_t _volume_level = 3;
  bool _pwm_initialized = false;
  void applyPwmVolume();
  void stopPwmOutput();
#endif
};
#endif
