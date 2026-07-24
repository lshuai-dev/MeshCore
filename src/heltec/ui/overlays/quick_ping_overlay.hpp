#pragma once

#include "heltec/ui/core/abstract_overlay.hpp"
#include "heltec/ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {
constexpr MetaId QuickPingOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x08);
constexpr MetaId QuickPingTitleBar = ht_meta_id(MetaIdScope::Overlay, 0xB8);
constexpr MetaId QuickPingContent = ht_meta_id(MetaIdScope::Overlay, 0xB9);
constexpr MetaId QuickPingTitle = ht_meta_id(MetaIdScope::Overlay, 0xB0);
constexpr MetaId QuickPingRow = ht_meta_id(MetaIdScope::Overlay, 0xB1);
constexpr MetaId QuickPingLabel = ht_meta_id(MetaIdScope::Overlay, 0xB2);
constexpr MetaId QuickPingDropdown = ht_meta_id(MetaIdScope::Overlay, 0xB3);
constexpr MetaId QuickPingMessageInput = ht_meta_id(MetaIdScope::Overlay, 0xB4);
constexpr MetaId QuickPingMessageInputLabel = ht_meta_id(MetaIdScope::Overlay, 0xB5);
constexpr MetaId QuickPingKeyboard = ht_meta_id(MetaIdScope::Overlay, 0xB6);
constexpr MetaId QuickPingMessageDropdown = ht_meta_id(MetaIdScope::Overlay, 0xB7);
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

#include <stddef.h>
#include <stdint.h>

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

class QuickPingOverlay final : public AbstractOverlay {
 public:
  explicit QuickPingOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void onEnter() override;
  void onExit() override;
  _lv_obj_t* focusedObject() const override;
  bool hitVisibleKeyboard(int16_t x, int16_t y) const;
  bool hitVerticalSwipeControl(int16_t x, int16_t y) const;
  bool hitSwipeDismissRegion(int16_t x, int16_t y) const;
  /** Start the slide-out animation. Returns false when SurfaceManager should dismiss now. */
  bool requestCloseAnimation();

 protected:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  bool onKey(uint32_t key) override;

 private:
  enum class TargetKind : uint8_t {
    Group = 0,
    Broadcast = 1,
    Personal = 2,
    Advert = 3,
  };

  struct CachedGroup {
    int channel_idx = -1;
    char label[32] = {};
  };

  struct CachedContact {
    uint8_t pub_key_prefix[6] = {};
    char label[32] = {};
  };

  static constexpr int kMaxGroups = 12;
  static constexpr int kMaxContacts = 12;
  static constexpr int kMessagePresetCount = 5;
  static constexpr uint32_t kMaxMessageLength = 120;

  _lv_obj_t* createDropdownRow(_lv_obj_t* parent, const char* label_text,
                               _lv_obj_t** out_dropdown);
  _lv_obj_t* createMessageRow(_lv_obj_t* parent);
  _lv_obj_t* createKeyboard(_lv_obj_t* parent);
  void applyState(bool keep_focus = true);
  void rebuildFocusGroup(_lv_obj_t* preferred = nullptr);
  void syncLists();
  void syncDropdownOptions();
  void syncRecipientDropdown();
  void setRowVisible(_lv_obj_t* row, bool visible);
  void setDropdownEnabled(_lv_obj_t* dropdown, bool enabled);
  void closeDropdowns();
  bool closeDropdown(_lv_obj_t* dropdown);
  void syncDropdownListLayout(_lv_obj_t* dropdown);
  void setKeyboardVisible(bool visible);
  bool keyboardVisible() const;
  bool dismissTransientControls();
  bool targetInside(_lv_obj_t* target, _lv_obj_t* ancestor) const;
  void closeKeyboardForOutsideTarget(_lv_obj_t* target);
  void handleDropdownRow(_lv_obj_t* target);
  void handleTargetChanged();
  void handleRecipientChanged();
  void handleMessageSelectionChanged();
  void handleMessageInput();
  void openPendingKeyboard();
  void submitMessageFromTextarea();
  bool sendMessageText(const char* text);
  void activateDropdown(_lv_obj_t* dropdown);
  void clearRepeatSelection();
  void armRepeatSelection(_lv_obj_t* dropdown);
  void finishRepeatSelection();
  bool currentTargetReady() const;
  TargetKind targetKind() const;
  static TargetKind targetKindFromDropdownIndex(uint16_t index);
  static uint16_t dropdownIndexForTargetKind(TargetKind kind);
  int currentGroupChannel() const;
  const uint8_t* currentContactKey() const;
  void showMissingTargetAlert() const;
  bool appendOption(char* buf, size_t buf_len, const char* option) const;

  static void onDropdownEvent(lv_event_t* e);
  static void onDropdownConfirmPreprocess(lv_event_t* e);
  static void finishRepeatSelectionAsync(void* user_data);
  static void openPendingKeyboardAsync(void* user_data);
  static void onMessageInputEvent(lv_event_t* e);
  static void onKeyboardEvent(lv_event_t* e);
  static void onKeyboardValuePre(lv_event_t* e);
  static void onOutsideEvent(lv_event_t* e);
  static void slideYExec(void* var, int32_t value);
  static void closeAnimationReady(void* user_data);

  void startOpenAnimation();

  _lv_obj_t* _title_bar = nullptr;
  _lv_obj_t* _content = nullptr;
  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _row_target = nullptr;
  _lv_obj_t* _row_recipient = nullptr;
  _lv_obj_t* _row_message = nullptr;
  _lv_obj_t* _dd_target = nullptr;
  _lv_obj_t* _dd_recipient = nullptr;
  _lv_obj_t* _dd_message = nullptr;
  _lv_obj_t* _ta_message = nullptr;
  _lv_obj_t* _keyboard = nullptr;

  CachedGroup _groups[kMaxGroups] = {};
  CachedContact _contacts[kMaxContacts] = {};
  int _group_count = 0;
  int _contact_count = 0;

  uint8_t _target_index = static_cast<uint8_t>(TargetKind::Broadcast);
  uint8_t _group_index = 0;
  uint8_t _contact_index = 0;
  uint8_t _message_index = 0;
  bool _syncing_dropdowns = false;
  bool _rebuilding_focus_group = false;
  bool _keyboard_open_pending = false;
  // A touch on the keyboard OK button can produce more than one LVGL event
  // (for example VALUE_CHANGED followed by READY). Keep one successful
  // submission per keyboard session so the same message is not queued twice.
  bool _message_submit_handled = false;
  bool _close_animating = false;
  bool _close_animation_ready = false;
  bool _repeat_pending = false;
  uint16_t _repeat_index = 0;
  _lv_obj_t* _repeat_dropdown = nullptr;

  char _recipient_options[512] = {};
  char _message_text[kMaxMessageLength + 1] = {};
};

}  // namespace heltec::meshcore::ui

#endif
