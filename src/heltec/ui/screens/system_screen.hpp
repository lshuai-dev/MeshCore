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
constexpr MetaId SystemDropdownList = ht_meta_id(MetaIdScope::Screen, 0xAE);
}

class SystemScreen : public AbstractScreen {
 public:
  SystemScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon), _feedback(biz) {}
  eScreenId screenId() const override { return eScreenId::System; }
  void onEnter() override;
  void onExit() override;
 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  void onUiEvent(const UiEvent& event) override;
  enum class SysAction : uint8_t { None, FactoryReset, ClearData };
  struct ChoiceRow {
    _lv_obj_t* row = nullptr;
    _lv_obj_t* label = nullptr;
    _lv_obj_t* dropdown = nullptr;
  };

  void bindWidget(_lv_obj_t* obj);
  _lv_obj_t* addActionRow(_lv_obj_t* scroll, const char* title, _lv_obj_t** out_row);
  _lv_obj_t* addSwitchRow(_lv_obj_t* scroll, const char* title, _lv_obj_t** out_sw);
  _lv_obj_t* addDropdownRow(_lv_obj_t* scroll, ChoiceRow& choice, const char* title);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  _lv_obj_t* addBuzzerVolumeRow(_lv_obj_t* scroll);
#endif

  static void onWidgetKeyPreprocess(lv_event_t* e);
  static void onActionRowEvent(lv_event_t* e);
  static void onSwitchValueChanged(lv_event_t* e);
  static void onDropdownValueChanged(lv_event_t* e);
  static void onDropdownStateEvent(lv_event_t* e);
  static void onDropdownReleasedPre(lv_event_t* e);
  static void realignDropdownListAsync(void* user_data);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  static void onBuzzerVolumeButtonClicked(lv_event_t* e);
#endif

  void handleAction(SysAction action);
  void executeAction(SysAction action);
  SysAction actionForRow(_lv_obj_t* obj) const;
  void syncDropdownLayout(_lv_obj_t* dropdown) const;
  void realignDropdownList(_lv_obj_t* dropdown);
  void clearGroupFocusVisual();
  void scrollFocusedIntoView(_lv_obj_t* focused) const;
  void closeOpenDropdown();
  void applyGroupFocus(_lv_obj_t* focused);
  void syncDropdownsFromApp(const biz::IBizFacade& app);
  void syncSwitchesFromApp(const biz::IBizFacade& app);
  void syncControlsFromApp(const biz::IBizFacade& app);
  void refreshControls();
  uint16_t regionDropdownIndex(const biz::IBizFacade& app) const;

  bool anyDropdownOpen() const;
  void setDropdownIndex(_lv_obj_t* dd, uint16_t index, bool fire_changed, bool force = false);
  void setDropdownOptions(_lv_obj_t* dd, const char* options);
  ChoiceRow* dropdownChoice(_lv_obj_t* dd);
  const ChoiceRow* dropdownChoice(_lv_obj_t* dd) const;
  bool isDropdownRow(_lv_obj_t* obj) const;
  void setSwitchState(_lv_obj_t* sw, bool on);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  void setBuzzerVolumeSlider(uint8_t level);
  void setBuzzerVolumeLevel(uint8_t level, bool show_feedback);
#endif

  IFeedback& _feedback;

  _lv_obj_t* _swForwarding = nullptr;
  _lv_obj_t* _swBle = nullptr;
#ifdef PIN_BUZZER
  _lv_obj_t* _swBuzzer = nullptr;
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  _lv_obj_t* _btnBuzzerVolumeDown = nullptr;
  _lv_obj_t* _sliderBuzzerVolume = nullptr;
  _lv_obj_t* _btnBuzzerVolumeUp = nullptr;
#endif
  ChoiceRow _choice_region;
  ChoiceRow _choice_screen_off;
  _lv_obj_t* _dd_region = nullptr;
  _lv_obj_t* _dd_screen_off = nullptr;
  _lv_obj_t* _row_factory_reset = nullptr;
  _lv_obj_t* _row_clear_data = nullptr;

  _lv_obj_t* _open_dropdown = nullptr;
  uint16_t _open_dropdown_original_index = 0;
  bool _syncing_dropdown = false;
  bool _syncing_switch = false;
};

}  // namespace heltec::meshcore::ui
