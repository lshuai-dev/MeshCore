#pragma once

#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"
#include "../core/ui_feedback.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId SystemActionRow = ht_meta_id(MetaIdScope::Screen, 0xA0);
constexpr MetaId SystemActionLabel = ht_meta_id(MetaIdScope::Screen, 0xA1);
constexpr MetaId SystemSwitchRow = ht_meta_id(MetaIdScope::Screen, 0xA2);
constexpr MetaId SystemSwitchLabel = ht_meta_id(MetaIdScope::Screen, 0xA3);
constexpr MetaId SystemSwitch = ht_meta_id(MetaIdScope::Screen, 0xA4);
constexpr MetaId SystemDropdownRow = ht_meta_id(MetaIdScope::Screen, 0xA5);
constexpr MetaId SystemDropdownLabel = ht_meta_id(MetaIdScope::Screen, 0xA6);
constexpr MetaId SystemDropdown = ht_meta_id(MetaIdScope::Screen, 0xA7);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
constexpr MetaId SystemVolumeRow = ht_meta_id(MetaIdScope::Screen, 0xA8);
constexpr MetaId SystemVolumeLabel = ht_meta_id(MetaIdScope::Screen, 0xA9);
constexpr MetaId SystemVolumeControls = ht_meta_id(MetaIdScope::Screen, 0xAA);
constexpr MetaId SystemVolumeButton = ht_meta_id(MetaIdScope::Screen, 0xAB);
constexpr MetaId SystemVolumeButtonLabel = ht_meta_id(MetaIdScope::Screen, 0xAC);
constexpr MetaId SystemVolumeSlider = ht_meta_id(MetaIdScope::Screen, 0xAD);
#endif
}

class SystemScreen : public AbstractScreen {
 public:
  SystemScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon, false), _feedback(biz) {}
  _lv_obj_t* create(_lv_obj_t* parent) override;
  lv_obj_t* focusedObject() const override;
  eScreenId screenId() const override { return eScreenId::System; }
  void onEnter() override;
  void onExit() override;
  void onWaypointKeyboardClosed() override;
  void onWaypointKeyboardSubmit(double lat, double lon) override;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  bool hitScrollableContent(lv_coord_t x, lv_coord_t y) const;
#endif

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  enum class SysAction : uint8_t { None, WpGps, WpManual, FactoryReset, ClearData };
  struct ChoiceRow {
    _lv_obj_t* row = nullptr;
    _lv_obj_t* label = nullptr;
    _lv_obj_t* dropdown = nullptr;
    const char* title = nullptr;
  };

  void bindWidget(_lv_obj_t* obj);
  bool onKey(uint32_t key) override;
  _lv_obj_t* addActionRow(_lv_obj_t* scroll, const char* title, _lv_obj_t** out_row);
  _lv_obj_t* addSwitchRow(_lv_obj_t* scroll, const char* title, _lv_obj_t** out_sw);
  _lv_obj_t* addDropdownRow(_lv_obj_t* scroll, ChoiceRow& choice, const char* title);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  _lv_obj_t* addBuzzerVolumeRow(_lv_obj_t* scroll);
#endif

  static void onWidgetKeyPreprocess(lv_event_t* e);
  static void onActionRowEvent(lv_event_t* e);
  static void onRowFocus(lv_event_t* e);
  static void onSwitchValueChanged(lv_event_t* e);
  static void onDropdownValueChanged(lv_event_t* e);
  static void onDropdownStateEvent(lv_event_t* e);
  static void onDropdownReleasedPre(lv_event_t* e);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  static void onBuzzerVolumeButtonClicked(lv_event_t* e);
#endif

  void handleAction(SysAction action);
  void executeAction(SysAction action);
  bool createActionConfirmation();
  void openActionConfirmation(SysAction action);
  void closeActionConfirmation();
  void acceptActionConfirmation();
  bool handleConfirmationKey(uint32_t key);
  static void onActionConfirmationEvent(lv_event_t* e);
  SysAction actionForRow(_lv_obj_t* obj) const;
  void syncDropdownLayout(_lv_obj_t* dd) const;
  void clearFocusRowHighlight();
  void highlightFocusRow(_lv_obj_t* row);
  bool isActionRow(_lv_obj_t* obj) const;
  void clearGroupFocusVisual();
  void scrollFocusedIntoView(_lv_obj_t* focused) const;
  void closeOpenDropdowns();
  void applyGroupFocus(_lv_obj_t* focused);
  bool focusKeypadWidget(_lv_obj_t* obj);
  void syncDropdownsFromApp(const biz::IBizFacade& app);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  void syncFriendDropdownFromApp(const biz::IBizFacade& app, bool force = false);
#endif
  void updateConditionalVisibility(const biz::IBizFacade& app);
  void rebuildKeypadGroup(const biz::IBizFacade& app);
  void syncSwitchesFromApp(const biz::IBizFacade& app);
  void syncControlsFromApp(const biz::IBizFacade& app);
  void refreshControls();
  void ensureKeypadFocus();
  void applyActionRowThemes();

  bool anyDropdownOpen() const;
  void setDropdownIndex(_lv_obj_t* dd, uint16_t index, bool fire_changed, bool force = false);
  void setDropdownOptions(_lv_obj_t* dd, const char* options);
  ChoiceRow* dropdownChoice(_lv_obj_t* dd);
  const ChoiceRow* dropdownChoice(_lv_obj_t* dd) const;
  bool isDropdownRow(_lv_obj_t* obj) const;
  void setSwitchState(_lv_obj_t* sw, bool on);
  void addKeypadWidget(_lv_obj_t* obj);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  void setBuzzerVolumeSlider(uint8_t level);
  void setBuzzerVolumeLevel(uint8_t level, bool show_feedback);
#endif

  IFeedback& _feedback;

  _lv_obj_t* _swBle = nullptr;
  _lv_obj_t* _swGps = nullptr;
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  _lv_obj_t* _swLna = nullptr;
#endif
#ifdef PIN_BUZZER
  _lv_obj_t* _swBuzzer = nullptr;
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  _lv_obj_t* _btnBuzzerVolumeDown = nullptr;
  _lv_obj_t* _sliderBuzzerVolume = nullptr;
  _lv_obj_t* _btnBuzzerVolumeUp = nullptr;
#endif
  _lv_obj_t* _swLocShare = nullptr;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _lv_obj_t* _swGpsTrack = nullptr;
#endif

  ChoiceRow _choice_region;
  ChoiceRow _choice_screen_off;
  ChoiceRow _choice_adv;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  ChoiceRow _choice_ff_mode;
  ChoiceRow _choice_friend;
#endif
  _lv_obj_t* _dd_region = nullptr;
  _lv_obj_t* _dd_screen_off = nullptr;
  _lv_obj_t* _row_adv = nullptr;
  _lv_obj_t* _dd_adv = nullptr;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _lv_obj_t* _dd_ff_mode = nullptr;
  _lv_obj_t* _row_friend = nullptr;
  _lv_obj_t* _dd_friend = nullptr;
  _lv_obj_t* _row_wp_gps = nullptr;
  _lv_obj_t* _row_wp_manual = nullptr;
#endif
  _lv_obj_t* _row_factory_reset = nullptr;
  _lv_obj_t* _row_clear_data = nullptr;
  _lv_obj_t* _action_confirm_root = nullptr;
  _lv_obj_t* _action_confirm_box = nullptr;
  _lv_obj_t* _action_confirm_body = nullptr;
  _lv_obj_t* _action_confirm_cancel = nullptr;
  _lv_obj_t* _action_confirm_accept = nullptr;
  SysAction _pending_action = SysAction::None;
  uint32_t _suppress_action_click_until_ms = 0;

  _lv_obj_t* _focus_row = nullptr;
  _lv_obj_t* _open_dropdown = nullptr;
  uint16_t _open_dropdown_original_index = 0;
  _lv_obj_t* _waypoint_keyboard_return_focus = nullptr;
  bool _syncing_dropdown = false;
  bool _syncing_switch = false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  uint32_t _friend_dd_options_hash_applied = 0;
  int16_t _friend_mesh_map[100] = {};
  int _friend_mesh_map_count = 0;
  int _friend_mesh_map_count_applied = -1;
#endif
  uint8_t _keypad_group_mask = 0xFF;
};

}  // namespace heltec::meshcore::ui
