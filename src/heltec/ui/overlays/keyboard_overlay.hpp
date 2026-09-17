#pragma once
#include "../core/abstract_overlay.hpp"

#include <stdint.h>

namespace heltec::meshcore::biz
{
class IBizFacade;
}
struct _lv_event_t;
struct _lv_obj_t;
namespace heltec::meshcore::ui
{

class KeyboardOverlay : public AbstractOverlay
{
  public:
    explicit KeyboardOverlay(biz::IBizFacade &biz);

    bool prepareMessageInput(const UiMessageKeyboardRequest &request);
    bool prepareWaypointInput();
    bool isWaypointCompose() const
    {
        return _compose_mode == ComposeMode::Waypoint;
    }

    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;

  private:
    _lv_obj_t *focusTarget() const override
    {
        return _keyboard;
    }
    bool onKey(uint32_t key) override;

    enum class ComposeMode : uint8_t
    {
        Message,
        Waypoint
    };

    static void on_keyboard_events(_lv_event_t *e);
    static void on_keyboard_value_pre(_lv_event_t *e);
    static void on_keyboard_key_pre(_lv_event_t *e);
    static void on_keyboard_key_post(_lv_event_t *e);
    static void on_waypoint_editor_clicked(_lv_event_t *e);
    bool ensureContent();
    void selectWaypointEditor(_lv_obj_t *editor);
    void setWaypointHint(const char *text);
    void fillWaypointFromGps();
    void submitMessageInput();
    void submitWaypointInput();
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    void applyTouchCoordinateLayout(bool waypoint);
#endif

    _lv_obj_t *_hint = nullptr;
    _lv_obj_t *_textarea = nullptr;
    _lv_obj_t *_longitude_textarea = nullptr;
    _lv_obj_t *_active_waypoint_editor = nullptr;
    _lv_obj_t *_keyboard = nullptr;
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    _lv_obj_t *_latitude_row = nullptr;
    _lv_obj_t *_longitude_row = nullptr;
    _lv_obj_t *_latitude_label = nullptr;
    _lv_obj_t *_longitude_label = nullptr;
#endif

    ComposeMode _compose_mode = ComposeMode::Message;
    UiMessageKeyboardPurpose _message_purpose = UiMessageKeyboardPurpose::SendCustom;
    uint8_t _preset_index = 0;
    uint8_t _message_max_bytes = kMaxText;
    bool _skip_next_ok_value = false;
    uint32_t _skip_next_ok_value_ms = 0;

    static constexpr int kMaxText = 80;
    char _title_text[40]{};
    char _text_buffer[kMaxText + 1]{};
    char _longitude_buffer[20]{};
    char _submitted_text[kMaxText + 1]{};
};

} // namespace heltec::meshcore::ui
