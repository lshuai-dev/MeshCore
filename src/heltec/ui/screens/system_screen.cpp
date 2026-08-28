#include "system_screen.hpp"

#include "../core/biz_facade.hpp"
#include "config/LoRaBandPresets.h"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ht_event.h"

#include <Arduino.h>
#include <lvgl.h>
#include <string.h>

namespace heltec::meshcore::ui
{

namespace
{

char s_region_options[kRadioParamPresetUiScratchSize + 32];
char s_screen_off_options[64];

int append_options(char *buf, size_t cap, const biz::IBizFacade &app, int count,
                   bool (*label_fn)(const biz::IBizFacade &, int, char *, size_t))
{
    char *p = buf;
    size_t rem = cap;
    for (int i = 0; i < count; ++i)
    {
        char lab[32];
        if (!label_fn(app, i, lab, sizeof(lab)))
            continue;
        const int n = lv_snprintf(p, rem, "%s%s", lab, (i + 1 < count) ? "\n" : "");
        if (n < 0 || (size_t)n >= rem)
            break;
        p += n;
        rem -= (size_t)n;
    }
    return (int)(p - buf);
}

bool screen_off_label(const biz::IBizFacade &app, int i, char *lab, size_t cap)
{
    const char *s = app.displayAutoOffOptionLabel(i);
    lv_snprintf(lab, cap, "%s", s ? s : "?");
    return true;
}

void build_region_options()
{
    char *const presets = radioParamPresetUiScratch();
    radioParamPresetDropdownOptions(presets, kRadioParamPresetUiScratchSize);
    lv_snprintf(s_region_options, sizeof(s_region_options), "Custom/fwd\n%s", presets);
}

} // namespace

_lv_obj_t *SystemScreen::create(_lv_obj_t *parent)
{
    if (!AbstractScreen::create(parent))
        return nullptr;
    addDropdownRow(_root, _choice_region, "Region");
    if (_choice_region.dropdown)
    {
        build_region_options();
        setDropdownOptions(_choice_region.dropdown, s_region_options);
    }

    addSwitchRow(_root, "Repeat Mode", &_swForwarding);

    addDropdownRow(_root, _choice_screen_off, "Screen Off");
    if (_choice_screen_off.dropdown)
    {
        append_options(s_screen_off_options, sizeof(s_screen_off_options), _biz,
                       _biz.displayAutoOffOptionCount(), screen_off_label);
        setDropdownOptions(_choice_screen_off.dropdown, s_screen_off_options);
    }

    addSwitchRow(_root, "Bluetooth", &_swBle);
#ifdef PIN_BUZZER
    addSwitchRow(_root, "Buzzer", &_swBuzzer);
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    addBuzzerVolumeRow(_root);
#endif
    addActionRow(_root, "> Factory Reset", &_row_factory_reset);
    addActionRow(_root, "> Clear Data", &_row_clear_data);

    addFocusItem(_choice_region.dropdown, _choice_region.row);
    addFocusItem(_swForwarding, _swForwarding ? lv_obj_get_parent(_swForwarding) : nullptr);
    addFocusItem(_choice_screen_off.dropdown, _choice_screen_off.row);
    addFocusItem(_swBle, _swBle ? lv_obj_get_parent(_swBle) : nullptr);
#ifdef PIN_BUZZER
    addFocusItem(_swBuzzer, _swBuzzer ? lv_obj_get_parent(_swBuzzer) : nullptr);
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    addFocusItem(_btnBuzzerVolumeDown, nullptr, false);
    addFocusItem(_btnBuzzerVolumeUp, nullptr, false);
#endif
    addFocusItem(_row_factory_reset);
    addFocusItem(_row_clear_data);

    ht_set_user_data(_root, this);

    lv_group_set_focus_cb(
        group(),
        +[](lv_group_t *g)
        {
            lv_obj_t *foc = lv_group_get_focused(g);
            if (!foc)
                return;
            for (lv_obj_t *p = foc; p; p = lv_obj_get_parent(p))
            {
                void *ud = ht_user_data(p);
                if (!ud)
                    continue;
                auto *self = static_cast<SystemScreen *>(ud);
                if (self->root() == p)
                {
                    if (!self->isDropdownRow(foc))
                        self->closeOpenDropdown();
                    return;
                }
            }
        });

    return _root;
}

void SystemScreen::onEnter()
{
    AbstractScreen::onEnter();
    closeOpenDropdown();
}

} // namespace heltec::meshcore::ui
