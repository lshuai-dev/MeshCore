#pragma once
#include "../core/AbstractOverlay.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
}
struct _lv_event_t;
struct _lv_obj_t;
namespace Heltec::MeshCore::Ui
{

class KeyboardOverlay : public AbstractOverlay
{
  public:
    explicit KeyboardOverlay(Biz::IBizFacade &biz);

    bool prepareMessageInput(const UiMessageKeyboardRequest &request);
    bool prepareWaypointInput();
    bool isWaypointCompose() const
    {
        return _compose_mode == ComposeMode::Waypoint;
    }


  protected:
    void onEnter() override;
    void onExit() override;
    bool build() override;

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

    enum class CoordinateField : uint8_t
    {
        Latitude,
        Longitude,
    };

    static void on_keyboard_events(_lv_event_t *e);
    static void on_keyboard_value_pre(_lv_event_t *e);
    static void on_keyboard_key_pre(_lv_event_t *e);
    static void on_keyboard_key_post(_lv_event_t *e);
    static void on_waypoint_editor_clicked(_lv_event_t *e);
    static void closeAsync(void *user_data);
    static void submitWaypointAsync(void *user_data);
    void requestClose();
    bool ensureContent();
    void selectCoordinateField(CoordinateField field);
    void saveCoordinateField();
    void setWaypointHint(const char *text);
    void fillWaypointFromGps();
    void submitMessageInput();
    void submitWaypointInput();
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    void applyTouchCoordinateLayout(bool waypoint);
#endif

    _lv_obj_t *_hint = nullptr;
    _lv_obj_t *_textarea = nullptr;
    _lv_obj_t *_longitude_textarea = nullptr;
    _lv_obj_t *_keyboard = nullptr;
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    _lv_obj_t *_latitude_row = nullptr;
    _lv_obj_t *_longitude_row = nullptr;
    _lv_obj_t *_latitude_label = nullptr;
    _lv_obj_t *_longitude_label = nullptr;
#endif

    ComposeMode _compose_mode = ComposeMode::Message;
    CoordinateField _coordinate_field = CoordinateField::Latitude;
    UiMessageKeyboardPurpose _message_purpose = UiMessageKeyboardPurpose::SendCustom;
    uint8_t _preset_index = 0;
    uint8_t _message_max_bytes = kMaxText;
    bool _close_pending = false;
    bool _waypoint_submit_pending = false;
    double _pending_latitude = 0;
    double _pending_longitude = 0;

    static constexpr int kMaxText = 80;
    char _title_text[40]{};
    char _text_buffer[kMaxText + 1]{};
    char _latitude_buffer[20]{};
    char _longitude_buffer[20]{};
    char _submitted_text[kMaxText + 1]{};
};

} // namespace Heltec::MeshCore::Ui
