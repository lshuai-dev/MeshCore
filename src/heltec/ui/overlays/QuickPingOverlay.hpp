#pragma once

#if HELTEC_UI_HAS_TOUCH

#include "heltec/ui/core/AbstractOverlay.hpp"
namespace Heltec::MeshCore::Ui
{
extern const lv_obj_class_t ht_quick_ping_overlay_class;
extern const lv_obj_class_t ht_quick_ping_backdrop_class;
extern const lv_obj_class_t ht_quick_ping_title_bar_class;
extern const lv_obj_class_t ht_quick_ping_content_class;
extern const lv_obj_class_t ht_quick_ping_message_controls_class;
extern const lv_obj_class_t ht_quick_ping_keyboard_editor_class;
extern const lv_obj_class_t ht_quick_ping_keyboard_counter_class;
extern const lv_obj_class_t ht_quick_ping_title_class;
extern const lv_obj_class_t ht_quick_ping_label_class;
extern const lv_obj_class_t ht_quick_ping_dropdown_class;
extern const lv_obj_class_t ht_quick_ping_message_dropdown_class;
extern const lv_obj_class_t ht_quick_ping_message_editor_class;
extern const lv_obj_class_t ht_quick_ping_keyboard_class;
} // namespace Heltec::MeshCore::Ui

#include <stddef.h>
#include <stdint.h>

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
}

namespace Heltec::MeshCore::Ui
{

struct UiSendMessageTarget;

class QuickPingOverlay final : public AbstractOverlay
{
  public:
    explicit QuickPingOverlay(Biz::IBizFacade &biz);
    void prepareTarget(const UiSendMessageTarget *target);
    bool preparePresetEdit(uint8_t index, const char *text);

    bool hitVisibleKeyboard(int16_t x, int16_t y) const;
    bool hitVerticalSwipeControl(int16_t x, int16_t y) const;
    bool hitSwipeDismissRegion(int16_t x, int16_t y) const;
    bool acceptsKeyboardCycle() const;
    /** Start the slide-out animation. Returns false when SurfaceManager should dismiss now. */
    bool requestCloseAnimation();

  protected:
    void onEnter() override;
    void onExit() override;
    bool build() override;
    bool onKey(uint32_t key) override;

  private:
    enum class TargetKind : uint8_t
    {
        Group = 0,
        Broadcast = 1,
        Personal = 2,
        Advert = 3,
    };

    struct CachedGroup
    {
        int channel_idx = -1;
        uint8_t source_index = 0;
    };

    struct CachedContact
    {
        uint8_t pub_key_prefix[6] = {};
    };

    static constexpr int kMaxGroups = 12;
    static constexpr int kMaxContacts = 10;
    static constexpr int kContactWindowStep = 5;
    static constexpr int kMessagePresetCount = 5;
    static constexpr uint32_t kMaxMessageLength = 120;
    static constexpr uint8_t kMaxPresetLength = 31;

    _lv_obj_t *createDropdownRow(_lv_obj_t *parent, const char *label_text,
                                 const lv_img_dsc_t *icon, _lv_obj_t **out_dropdown);
    _lv_obj_t *createMessageRow(_lv_obj_t *parent);
    _lv_obj_t *createKeyboard(_lv_obj_t *parent);
    void applyState(bool keep_focus = true);
    void restoreModeFocus(_lv_obj_t *preferred = nullptr);
    void syncLists();
    void rebuildMessagePresetOptions();
    void applyPendingTarget();
    void loadContactWindow(int start);
    void moveContactSelection(int delta);
    void recenterContactWindow(int global_index, int direction);
    void syncDropdownOptions();
    void syncRecipientDropdown();
    void setRowVisible(_lv_obj_t *row, bool visible);
    void setDropdownEnabled(_lv_obj_t *dropdown, bool enabled);
    void closeDropdowns();
    bool closeDropdown(_lv_obj_t *dropdown);
    void updateMessageTextPresentation(bool editing);
    void updateKeyboardCounter();
    void syncKeyboardEditorToMessage();
    void setKeyboardVisible(bool visible);
    bool keyboardVisible() const;
    bool dismissTransientControls();
    bool targetInside(_lv_obj_t *target, _lv_obj_t *ancestor) const;
    void closeKeyboardForOutsideTarget(_lv_obj_t *target);
    void handleDropdownRow(_lv_obj_t *target);
    void handleTargetChanged();
    void handleRecipientChanged();
    void handleMessageSelectionChanged();
    void handleMessageInput();
    void openPendingKeyboard();
    void submitMessageFromTextarea();
    bool sendMessageText(const char *text);
    void activateDropdown(_lv_obj_t *dropdown);
    void clearRepeatSelection();
    void armRepeatSelection(_lv_obj_t *dropdown);
    void finishRepeatSelection();
    bool currentTargetReady() const;
    TargetKind targetKind() const;
    static TargetKind targetKindFromDropdownIndex(uint16_t index);
    static uint16_t dropdownIndexForTargetKind(TargetKind kind);
    int currentGroupChannel() const;
    const uint8_t *currentContactKey() const;
    void showMissingTargetAlert() const;
    bool appendOption(char *buf, size_t buf_len, const char *option) const;

    static void onDropdownEvent(lv_event_t *e);
    static void onDropdownConfirmPreprocess(lv_event_t *e);
    static void finishRepeatSelectionAsync(void *user_data);
    static void openPendingKeyboardAsync(void *user_data);
    static void onMessageInputEvent(lv_event_t *e);
    static void onKeyboardEditorEvent(lv_event_t *e);
    static void onKeyboardEvent(lv_event_t *e);
    static void onKeyboardValuePre(lv_event_t *e);
    static void onOutsideEvent(lv_event_t *e);
    static void slideExec(void *var, int32_t value);
    static void closeAnimationReady(void *user_data);

    lv_coord_t paneHeight() const;
    void startOpenAnimation();

    _lv_obj_t *_title_bar = nullptr;
    _lv_obj_t *_backdrop = nullptr;
    _lv_obj_t *_content = nullptr;
    _lv_obj_t *_title = nullptr;
    _lv_obj_t *_row_target = nullptr;
    _lv_obj_t *_row_recipient = nullptr;
    _lv_obj_t *_row_message = nullptr;
    _lv_obj_t *_dd_target = nullptr;
    _lv_obj_t *_dd_recipient = nullptr;
    _lv_obj_t *_dd_message = nullptr;
    _lv_obj_t *_ta_message = nullptr;
    _lv_obj_t *_keyboard = nullptr;
    _lv_obj_t *_keyboard_editor = nullptr;
    _lv_obj_t *_keyboard_counter = nullptr;
    _lv_obj_t *_keyboard_return_focus = nullptr;

    CachedGroup _groups[kMaxGroups] = {};
    CachedContact _contacts[kMaxContacts] = {};
    int _group_count = 0;
    int _contact_count = 0;
    int _contact_total = 0;
    int _contact_window_start = 0;

    uint8_t _target_index = static_cast<uint8_t>(TargetKind::Broadcast);
    uint8_t _group_index = 0;
    int _contact_index = 0;
    uint8_t _message_index = 0;
    bool _syncing_dropdowns = false;
    bool _rebuilding_navigation = false;
    bool _keyboard_open_pending = false;
    // A touch on the keyboard OK button can produce more than one LVGL event
    // (for example VALUE_CHANGED followed by READY). Keep one successful
    // submission per keyboard session so the same message is not queued twice.
    bool _message_submit_handled = false;
    bool _preset_edit_active = false;
    uint8_t _preset_edit_index = 0;
    bool _close_animating = false;
    bool _repeat_pending = false;
    bool _pending_target_valid = false;
    bool _pending_target_direct = false;
    uint8_t _pending_target_key[6] = {};
    int _pending_target_channel = -1;
    uint16_t _repeat_index = 0;
    _lv_obj_t *_repeat_dropdown = nullptr;

    char _recipient_options[384] = {};
    char _message_preset_options[kMessagePresetCount * 32] = {};
    char _message_text[kMaxMessageLength + 1] = {};
    char _keyboard_count_text[8] = {};
};

} // namespace Heltec::MeshCore::Ui

#endif
