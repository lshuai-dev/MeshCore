#include "SystemScreen.hpp"

#include "ui/theme/UiTheme.hpp"
#include "ui/widgets/HtContainer.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include "ui/theme/WidgetTheme.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/DisplayPort.hpp"
#endif

#include <Arduino.h>

namespace Heltec::MeshCore::Ui
{
namespace
{
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
void request_dropdown_refresh()
{
    Heltec::MeshCore::Dal::DisplayPort::requestSelectionRefresh();
}
#else
void request_dropdown_refresh()
{
}
#endif

#if HELTEC_UI_HAS_TOUCH
constexpr lv_coord_t kSystemDropdownHeight = 28;
#else
constexpr lv_coord_t kSystemDropdownHeight = 16;
#endif
constexpr lv_coord_t kSystemSwitchHeight = 16;
void configure_system_row(_lv_obj_t *row, lv_flex_flow_t flow, lv_flex_align_t main,
                          lv_flex_align_t cross, bool single_line = true)
{
    if (!row)
        return;
    lv_obj_set_flex_flow(row, flow);
    lv_obj_set_flex_align(row, main, cross, cross);
    if (!single_line)
    {
        lv_obj_set_height(row, LV_SIZE_CONTENT);
    }
}
void configure_system_label(_lv_obj_t *label, lv_coord_t width)
{
    if (!label)
        return;
    lv_obj_set_width(label, width);
    lv_obj_clear_flag(label,
                      LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
}
} // namespace

_lv_obj_t *SystemScreen::addActionRow(_lv_obj_t *scroll, const char *title, _lv_obj_t **out_row)
{
    _lv_obj_t *row = ht_container_create(scroll);
    if (!row)
        return nullptr;
    configure_system_row(row, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    _lv_obj_t *label = lv_label_create(row);
    configure_system_label(label, lv_pct(100));
    if (label)
        lv_label_set_text_static(label, title ? title : "");
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, onActionRowEvent, LV_EVENT_CLICKED, this);
    if (out_row)
        *out_row = row;
    return row;
}

_lv_obj_t *SystemScreen::addSwitchRow(_lv_obj_t *scroll, const char *title, _lv_obj_t **out_sw)
{
    _lv_obj_t *row = ht_container_create(scroll);
    if (!row)
        return nullptr;
    configure_system_row(row, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);
    _lv_obj_t *label = lv_label_create(row);
    configure_system_label(label, LV_SIZE_CONTENT);
    if (label)
        lv_label_set_text_static(label, title ? title : "");
    *out_sw = lv_switch_create(row);
    if (!*out_sw)
        return row;
    lv_obj_set_height(*out_sw, kSystemSwitchHeight);
    lv_obj_clear_flag(*out_sw, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(*out_sw, onSwitchValueChanged, LV_EVENT_VALUE_CHANGED, this);
    return row;
}

_lv_obj_t *SystemScreen::addDropdownRow(_lv_obj_t *scroll, ChoiceRow &choice, const char *title)
{
    _lv_obj_t *row = ht_container_create(scroll);
    if (!row)
        return nullptr;
    configure_system_row(row, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    _lv_obj_t *label = lv_label_create(row);
    configure_system_label(label, LV_SIZE_CONTENT);
    choice.row = row;
    choice.dropdown = nullptr;
    if (label)
    {
        lv_label_set_text_static(label, title ? title : "");
    }

    _lv_obj_t *dd = lv_dropdown_create(row);
    choice.dropdown = dd;
    if (!dd)
        return row;
    // Flex owns the horizontal layout. A zero base width ensures the dropdown
    // does not request space from its option text; it simply fills the row area
    // left after the label.
    lv_obj_set_width(dd, 0);
    lv_obj_set_flex_grow(dd, 1);
    lv_obj_set_height(dd, kSystemDropdownHeight);
    lv_obj_clear_flag(dd, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(dd, LV_SCROLLBAR_MODE_OFF);
    lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);

    lv_obj_add_event_cb(dd, onDropdownValueChanged, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(dd, onDropdownStateEvent, LV_EVENT_READY, this);
    lv_obj_add_event_cb(dd, onDropdownStateEvent, LV_EVENT_CANCEL, this);
    return dd;
}

void SystemScreen::closeOpenDropdown()
{
    if (!_open_dropdown)
        return;
    _lv_obj_t *const closing = _open_dropdown;
    _open_dropdown = nullptr;
#if LV_USE_DROPDOWN != 0
    if (lv_obj_is_valid(closing) && lv_obj_has_class(closing, &lv_dropdown_class) &&
        lv_dropdown_is_open(closing))
    {
        lv_dropdown_set_selected(closing, _open_dropdown_original_index);
        lv_dropdown_close(closing);
    }
#endif
    if (lv_obj_is_valid(closing))
        lv_obj_clear_state(closing, LV_STATE_EDITED);
    _gridNav.setEditing(false);
    request_dropdown_refresh();
}

void SystemScreen::onActionRowEvent(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    auto *self = static_cast<SystemScreen *>(lv_event_get_user_data(e));
    if (!self)
        return;

    const SysAction action = self->actionForRow(lv_event_get_target(e));
    if (action == SysAction::None)
        return;

    self->handleAction(action);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL

_lv_obj_t *SystemScreen::addBuzzerVolumeRow(_lv_obj_t *scroll)
{
    _lv_obj_t *row = ht_container_create(scroll);
    if (!row)
        return nullptr;
    configure_system_row(row, LV_FLEX_FLOW_COLUMN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                         false);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                            LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    _lv_obj_t *title = lv_label_create(row);
    if (title)
        lv_label_set_text_static(title, "Volume");
    configure_system_label(title, lv_pct(100));

    _lv_obj_t *controls = ht_class_create(&ht_volume_controls_class, row);
    if (!controls)
        return row;
    lv_obj_set_size(controls, lv_pct(100), 26);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE |
                               LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    addBuzzerVolumeButton(controls, "-", &_btnBuzzerVolumeDown);

#if LV_USE_SLIDER != 0
    _sliderBuzzerVolume = lv_slider_create(controls);
    if (_sliderBuzzerVolume)
    {
        lv_obj_set_size(_sliderBuzzerVolume, 60, 10);
        lv_obj_set_flex_grow(_sliderBuzzerVolume, 1);
        lv_slider_set_range(_sliderBuzzerVolume, 0, 3);
        lv_obj_clear_flag(_sliderBuzzerVolume, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE |
                                                   LV_OBJ_FLAG_CLICK_FOCUSABLE);
    }
#endif

    addBuzzerVolumeButton(controls, "+", &_btnBuzzerVolumeUp);
    return row;
}

void SystemScreen::addBuzzerVolumeButton(_lv_obj_t *parent, const char *text,
                                         _lv_obj_t **out_button)
{
    if (!parent || !out_button)
        return;
    _lv_obj_t *const button = ht_btn_create(&ht_volume_button_class, parent);
    if (!button)
        return;
    lv_obj_set_size(button, 26, 24);
#if HELTEC_UI_HAS_TOUCH
    lv_obj_set_ext_click_area(button, 5);
#endif
    _lv_obj_t *const label = lv_label_create(button);
    if (label)
    {
        lv_label_set_text_static(label, text ? text : "");
        lv_obj_center(label);
    }
    lv_obj_add_event_cb(button, onBuzzerVolumeButtonClicked, LV_EVENT_CLICKED, this);
    *out_button = button;
}

#endif

} // namespace Heltec::MeshCore::Ui
