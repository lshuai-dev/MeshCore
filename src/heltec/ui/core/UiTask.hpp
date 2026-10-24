#pragma once
#include "../../core/AbstractUiTask.h"
#include "config/NodePrefs.h"
#include <helpers/BaseChatMesh.h>

#include <stdint.h>
#ifdef PIN_BUZZER
#include "HeltecBuzzer.h"
#endif

namespace Heltec::MeshCore::Ui
{
class UiTask final : public AbstractUITask
{
  public:
    explicit UiTask(BaseSerialInterface *serial) : AbstractUITask(serial)
    {
    }

    void msgRead(int msgcount) override;
    void newMsg(uint8_t path_len, const char *from_name, const char *text, int msgcount) override;
    void notify(UIEventType t = UIEventType::none) override;
    void setHasConnection(bool connected) override;
    void loop() override;
    /** Init buzzer from stored prefs (call once after mesh prefs are loaded). */
    void begin(NodePrefs *node_prefs);

    bool isBuzzerPlaying();
    void setBuzzerEnabled(bool enabled);
    uint8_t buzzerVolumeLevel() const;
    void setBuzzerVolumeLevel(uint8_t level);
    bool isLnaCanControl() const;
    bool lnaEnabled() const;
    bool setLnaEnabled(bool enabled);
    void startShutdownFeedback();
    bool shutdownFeedbackDone();

    int msgCount() const
    {
        return _msg_count;
    }
    void setMessageCount(int msgcount);

    // Preview state mirrors the original MeshCore 32-entry volatile queue.
    bool isPreviewActive() const
    {
        return _preview_count > 0;
    }
    const char *previewOrigin() const
    {
        return _preview_entries[_preview_head].origin;
    }
    const char *previewText() const
    {
        return _preview_entries[_preview_head].text;
    }
    uint8_t previewUnread() const
    {
        return _preview_count;
    }
    uint32_t previewReceivedMillis() const
    {
        return _preview_entries[_preview_head].received_ms;
    }
    bool advancePreview();
    void dismissPreview();

    // Alert state (compatible with legacy overlay behavior)
    void showAlert(const char *text, int duration_millis);
    void playAlertFeedback();
    bool isAlertActive() const;
    const char *getAlertText() const
    {
        return _alert;
    }
    void dismissAlert();

  private:
    NodePrefs *_node_prefs = nullptr;
#ifdef PIN_BUZZER
    HeltecBuzzer _buzzer;
    bool _buzzer_startup_pending = false;
#endif
    int _msg_count = 0;
    char _alert[80] = { 0 };
    uint32_t _alert_expiry_ms = 0;

    static constexpr uint8_t kPreviewCapacity = 32;
    struct PreviewEntry
    {
        uint32_t received_ms = 0;
        char origin[62] = {};
        char text[MAX_TEXT_LEN + 1] = {};
    };
    PreviewEntry _preview_entries[kPreviewCapacity]{};
    uint8_t _preview_head = kPreviewCapacity - 1;
    uint8_t _preview_count = 0;

#ifdef PIN_STATUS_LED
    void pollStatusLed();
    uint8_t _status_led_on = 0;
    uint32_t _status_led_next_ms = 0;
    uint16_t _status_led_on_ms = 0;
#endif
    bool _shutdown_feedback_active = false;
    void finishShutdownFeedback();
};

// Global singleton instance created in Main.cpp for LVGL builds.
UiTask &ui_task();

} // namespace Heltec::MeshCore::Ui
