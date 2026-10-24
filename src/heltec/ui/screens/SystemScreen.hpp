#pragma once

#include "../core/AbstractScreen.hpp"
#include "../core/AppStateEvent.hpp"

namespace Heltec::MeshCore::Biz {
class IBizFacade;
}

namespace Heltec::MeshCore::Ui {

extern const lv_obj_class_t ht_system_screen_root_class;

class SystemScreen final : public AbstractScreen {
  public:
    SystemScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon) {
    }

  protected:
    void onEnter() override;
    void onExit() override;
    void onActivated() override;
    bool build() override;
    bool onKey(uint32_t key) override;
    bool acceptsAppEvents() const override {
        return true;
    }
    void onAppEvent(const UiMessage &event) override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    enum class SysAction : uint8_t { None, FactoryReset, ClearData };
    struct ChoiceRow {
        _lv_obj_t *row = nullptr;
        _lv_obj_t *dropdown = nullptr;
    };

    _lv_obj_t *addActionRow(_lv_obj_t *scroll, const char *title, _lv_obj_t **out_row);
    _lv_obj_t *addSwitchRow(_lv_obj_t *scroll, const char *title, _lv_obj_t **out_sw);
    _lv_obj_t *addDropdownRow(_lv_obj_t *scroll, ChoiceRow &choice, const char *title);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    _lv_obj_t *addBuzzerVolumeRow(_lv_obj_t *scroll);
    void addBuzzerVolumeButton(_lv_obj_t *parent, const char *text, _lv_obj_t **out_button);
#endif

    static void onActionRowEvent(lv_event_t *e);
    static void onSwitchValueChanged(lv_event_t *e);
    static void onDropdownValueChanged(lv_event_t *e);
    static void onDropdownStateEvent(lv_event_t *e);
    static void onFocusChanged(lv_obj_t *focused, void *user_data);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    static void onBuzzerVolumeButtonClicked(lv_event_t *e);
#endif

    void handleAction(SysAction action);
    void executeAction(SysAction action);
    SysAction actionForRow(_lv_obj_t *obj) const;
    void closeOpenDropdown();
    void syncDropdownsFromApp(const Biz::IBizFacade &app);
    void syncSwitchesFromApp(const Biz::IBizFacade &app);
    void syncControlsFromApp(const Biz::IBizFacade &app);
    void refreshControls();
    void openMessagePresetEditor(uint8_t index);
    uint16_t regionDropdownIndex(const Biz::IBizFacade &app) const;

    void setDropdownIndex(_lv_obj_t *dd, uint16_t index, bool fire_changed, bool force = false);
    void setDropdownOptions(_lv_obj_t *dd, const char *options);
    ChoiceRow *dropdownChoice(_lv_obj_t *dd);
    bool isDropdownRow(_lv_obj_t *obj);
    void setSwitchState(_lv_obj_t *sw, bool on);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    void setBuzzerVolumeSlider(uint8_t level);
    void setBuzzerVolumeLevel(uint8_t level, bool show_feedback);
#endif

    _lv_obj_t *_swForwarding = nullptr;
    _lv_obj_t *_swBle = nullptr;
#ifdef PIN_BUZZER
    _lv_obj_t *_swBuzzer = nullptr;
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    _lv_obj_t *_btnBuzzerVolumeDown = nullptr;
    _lv_obj_t *_sliderBuzzerVolume = nullptr;
    _lv_obj_t *_btnBuzzerVolumeUp = nullptr;
#endif
    ChoiceRow _choice_region;
    ChoiceRow _choice_screen_off;
    ChoiceRow _choice_message_preset;
    _lv_obj_t *_row_factory_reset = nullptr;
    _lv_obj_t *_row_clear_data = nullptr;

    _lv_obj_t *_open_dropdown = nullptr;
    uint16_t _open_dropdown_original_index = 0;
    bool _syncing_dropdown = false;
    bool _syncing_switch = false;
};

} // namespace Heltec::MeshCore::Ui
