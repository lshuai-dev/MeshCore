#pragma once
#include <stdint.h>
#include "../../core/AbstractUITask.h"
#include "config/NodePrefs.h"
#ifdef PIN_BUZZER
#include "heltec_buzzer.h"
#endif


namespace heltec::meshcore::ui {
class IUiHost;

class UiTask final : public AbstractUITask {
 public:
  UiTask(mesh::MainBoard* board, BaseSerialInterface* serial) : AbstractUITask(board, serial) {}

  void attachHost(IUiHost* host);

  void msgRead(int msgcount) override;
  void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void setHasConnection(bool connected) override;
  void loop() override;
  /** Cached battery mV; hardware ADC polled at most every 10s in loop(). */
  uint16_t batteryMilliVolts() const { return _batt_mv; }
  /** Init buzzer from stored prefs (call once after mesh prefs are loaded). */
  void begin(NodePrefs* node_prefs);

  bool isBuzzerQuiet() const;
  void setBuzzerEnabled(bool enabled);
  uint8_t buzzerVolumeLevel() const;
  void setBuzzerVolumeLevel(uint8_t level);
  bool isLnaCanControl() const;
  bool lnaEnabled() const;
  bool setLnaEnabled(bool enabled);
  void playShutdownMelody();

  int msgCount() const { return _msg_count; }
  void setMessageCount(int msgcount);

  // Preview state mirrors the original MeshCore 32-entry volatile queue.
  bool isPreviewActive() const { return _preview_count > 0; }
  const char* previewOrigin() const { return _preview_entries[_preview_head].origin; }
  const char* previewText() const { return _preview_entries[_preview_head].text; }
  uint8_t previewUnread() const { return _preview_count; }
  uint32_t previewReceivedMillis() const {
    return _preview_entries[_preview_head].received_ms;
  }
  uint32_t previewAgeSeconds() const;
  bool advancePreview();
  void dismissPreview();

  // Alert state (compatible with legacy overlay behavior)
  void showAlert(const char* text, int duration_millis);
  bool isAlertActive() const;
  const char* getAlertText() const { return _alert; }
  void dismissAlert();

 private:
  IUiHost* _ui_host = nullptr;
  NodePrefs* _node_prefs = nullptr;
#ifdef PIN_BUZZER
  HeltecBuzzer _buzzer;
  bool _buzzer_startup_pending = false;
#endif
  int _msg_count = 0;
  char _alert[80] = {0};
  uint32_t _alert_expiry_ms = 0;

  static constexpr uint8_t kPreviewCapacity = 32;
  struct PreviewEntry {
    uint32_t received_ms = 0;
    char origin[62] = {};
    char text[78] = {};
  };
  PreviewEntry _preview_entries[kPreviewCapacity]{};
  uint8_t _preview_head = kPreviewCapacity - 1;
  uint8_t _preview_count = 0;

  uint16_t _batt_mv = 0;
  uint32_t _batt_last_read_ms = 0;
  void pollBattery();
#ifdef PIN_STATUS_LED
  void pollStatusLed();
  uint8_t _status_led_on = 0;
  uint32_t _status_led_next_ms = 0;
  uint16_t _status_led_on_ms = 0;
#endif
};

// Global singleton instance created in main.cpp for LVGL builds.
UiTask& ui_task();

}  // namespace heltec::meshcore::ui


