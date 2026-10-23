#include "SystemScreen.hpp"
#include "../core/BizFacade.hpp"
#include "config/LoraBandPresets.h"
#include "ui/theme/UiTheme.hpp"
#include "ui/core/HtEvent.h"

namespace Heltec::MeshCore::Ui {

const lv_obj_class_t ht_system_screen_root_class = {
    &lv_obj_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_obj_t),
};

namespace {

char s_region_options[kRadioParamPresetUiScratchSize + 32];
char s_screen_off_options[64];
constexpr const char *kMessagePresetOptions =
    "Select\nPreset 1\nPreset 2\nPreset 3\nPreset 4\nPreset 5";

int append_options(char *buf, size_t cap, const Biz::IBizFacade &app, int count,
                   bool (*label_fn)(const Biz::IBizFacade &, int, char *, size_t)) {
    char *p = buf;
    size_t rem = cap;
    for (int i = 0; i < count; ++i) {
        char lab[32];
        if (!label_fn(app, i, lab, sizeof(lab))) continue;
        const int n = lv_snprintf(p, rem, "%s%s", lab, (i + 1 < count) ? "\n" : "");
        if (n < 0 || (size_t)n >= rem) break;
        p += n;
        rem -= (size_t)n;
    }
    return (int)(p - buf);
}

bool screen_off_label(const Biz::IBizFacade &app, int i, char *lab, size_t cap) {
    const char *s = app.displayAutoOffOptionLabel(i);
    lv_snprintf(lab, cap, "%s", s ? s : "?");
    return true;
}

void build_region_options() {
    char *const presets = radioParamPresetUiScratch();
    radioParamPresetDropdownOptions(presets, kRadioParamPresetUiScratchSize);
    lv_snprintf(s_region_options, sizeof(s_region_options), "Custom/fwd\n%s", presets);
}

} // namespace

bool SystemScreen::buildSurface() {
    if (!AbstractScreen::buildSurface()) return false;
    addDropdownRow(_root, _choice_region, "Region");
    if (_choice_region.dropdown) {
        build_region_options();
        setDropdownOptions(_choice_region.dropdown, s_region_options);
    }

    addSwitchRow(_root, "Repeat Mode", &_swForwarding);

    addDropdownRow(_root, _choice_screen_off, "Screen Off");
    if (_choice_screen_off.dropdown) {
        append_options(s_screen_off_options, sizeof(s_screen_off_options), _biz,
                       _biz.displayAutoOffOptionCount(), screen_off_label);
        setDropdownOptions(_choice_screen_off.dropdown, s_screen_off_options);
    }

#if HELTEC_UI_ENABLE_KEYBOARD
    addDropdownRow(_root, _choice_message_preset, "Message presets");
    setDropdownOptions(_choice_message_preset.dropdown, kMessagePresetOptions);
#endif

    addSwitchRow(_root, "Bluetooth", &_swBle);
#ifdef PIN_BUZZER
#if !defined(HAS_BUZZER_VOLUME_CONTROL) || !HAS_BUZZER_VOLUME_CONTROL
    addSwitchRow(_root, "Buzzer", &_swBuzzer);
#endif
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    addBuzzerVolumeRow(_root);
#endif
    addActionRow(_root, "> Factory Reset", &_row_factory_reset);
    addActionRow(_root, "> Clear Data", &_row_clear_data);

    addNavigableObject(_choice_region.dropdown, _choice_region.row);
    addNavigableObject(_swForwarding, _swForwarding ? lv_obj_get_parent(_swForwarding) : nullptr);
    addNavigableObject(_choice_screen_off.dropdown, _choice_screen_off.row);
#if HELTEC_UI_ENABLE_KEYBOARD
    addNavigableObject(_choice_message_preset.dropdown, _choice_message_preset.row);
#endif
    addNavigableObject(_swBle, _swBle ? lv_obj_get_parent(_swBle) : nullptr);
#ifdef PIN_BUZZER
#if !defined(HAS_BUZZER_VOLUME_CONTROL) || !HAS_BUZZER_VOLUME_CONTROL
    addNavigableObject(_swBuzzer, _swBuzzer ? lv_obj_get_parent(_swBuzzer) : nullptr);
#endif
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    addNavigableObject(_btnBuzzerVolumeDown, nullptr, false);
    addNavigableObject(_btnBuzzerVolumeUp, nullptr, false);
#endif
    addNavigableObject(_row_factory_reset);
    addNavigableObject(_row_clear_data);

    _gridNav.setFocusChangedCallback(onFocusChanged, this);

    return true;
}

void SystemScreen::onFocusChanged(lv_obj_t *focused, void *user_data) {
    auto *const self = static_cast<SystemScreen *>(user_data);
    if (self != nullptr && focused != nullptr && !self->isDropdownRow(focused)) {
        self->closeOpenDropdown();
    }
}

void SystemScreen::onEnter() {
    AbstractScreen::onEnter();
    closeOpenDropdown();
}

void SystemScreen::openMessagePresetEditor(uint8_t index)
{
    if (index >= _biz.messagePresetCount())
        return;
    UiMessageKeyboardRequest request{};
    request.title = "Edit preset";
    request.initial_text = _biz.messagePresetAt(index);
    request.purpose = UiMessageKeyboardPurpose::EditPreset;
    request.preset_index = index;
    request.max_bytes = 31;
    if (!emitEvent(EventType::MessageKeyboardOpen, &request))
        showAlert("Unable to open keyboard", 1600);
}

} // namespace Heltec::MeshCore::Ui
