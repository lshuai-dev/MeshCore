#include "system_screen.hpp"

#include "ui/core/ht_dropdown.hpp"
#include "ui/app/theme.hpp"
#include "ui/core/ht_container.hpp"
#include "ui/core/widget_classes.hpp"
#include "ui/theme/widget_theme.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/e213_port.hpp"
#endif

#include <Arduino.h>

namespace heltec::meshcore::ui
{
namespace
{
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
void request_dropdown_refresh()
{
    heltec::meshcore::dal::e213::requestSelectionRefresh();
}
#else
void request_dropdown_refresh()
{
}
#endif

#if HELTEC_HAS_TOUCH
constexpr lv_coord_t kSystemDropdownHeight =
    ui_settings_row_height() - 2 * ui_layout_metrics().row_padding_vertical;
#endif
void configure_system_row(_lv_obj_t *row, lv_flex_flow_t flow, lv_flex_align_t main,
                          lv_flex_align_t cross, bool single_line = true,
                          bool event_bubble = true)
{
    if (!row)
        return;
    lv_obj_set_flex_flow(row, flow);
    lv_obj_set_flex_align(row, main, cross, cross);
    if (event_bubble)
        lv_obj_add_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);
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
bool keypad_suppresses_bubble(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    if (lv_obj_check_type(obj, &ht_container_class) && lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE))
        return true;
#if LV_USE_SWITCH != 0
    if (lv_obj_check_type(obj, &lv_switch_class))
        return true;
#endif
#if LV_USE_DROPDOWN != 0
    if (lv_obj_has_class(obj, &lv_dropdown_class))
        return true;
#endif
#if LV_USE_SLIDER != 0
    if (lv_obj_check_type(obj, &lv_slider_class))
        return true;
#endif
    return false;
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
    bindWidget(row);
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
    lv_obj_clear_flag(*out_sw, LV_OBJ_FLAG_SCROLLABLE);
    bindWidget(*out_sw);
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

    _lv_obj_t *dd = ht_dropdown_create(row);
    choice.dropdown = dd;
    if (!dd)
        return row;
    ht_dropdown_set_viewport(dd, lv_obj_get_parent(scroll));
    // Flex owns the horizontal layout. A zero base width ensures the dropdown
    // does not request space from its option text; it simply fills the row area
    // left after the label.
    lv_obj_set_width(dd, 0);
    lv_obj_set_flex_grow(dd, 1);
#if LV_COLOR_DEPTH == 1
    lv_obj_set_height(dd, 12);
#else
#if HELTEC_HAS_TOUCH
    lv_obj_set_height(dd, kSystemDropdownHeight);
#else
    lv_obj_set_height(dd, LV_SIZE_CONTENT);
#endif
#endif
    lv_obj_clear_flag(dd, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(dd, LV_SCROLLBAR_MODE_OFF);
    lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);

    bindWidget(dd);
    lv_obj_add_event_cb(dd, onDropdownValueChanged, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(dd, onDropdownStateEvent, LV_EVENT_READY, this);
    lv_obj_add_event_cb(dd, onDropdownStateEvent, LV_EVENT_CANCEL, this);
    return dd;
}

void SystemScreen::bindWidget(_lv_obj_t *obj)
{
    if (!obj)
        return;
    if (keypad_suppresses_bubble(obj))
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(obj, onWidgetKeyPreprocess,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
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
    if (group())
        lv_group_set_editing(group(), false);
    request_dropdown_refresh();
}

void SystemScreen::onWidgetKeyPreprocess(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY)
        return;
    auto *self = static_cast<SystemScreen *>(lv_event_get_user_data(e));
    if (!self)
        return;

    const uint32_t key = lv_event_get_key(e);
    _lv_obj_t *const target = lv_event_get_target(e);

#if LV_USE_DROPDOWN != 0
    if (self->_open_dropdown == target && ht_dropdown_cycle(target, key))
    {
        request_dropdown_refresh();
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }

    // lv_dropdown_set_selected() also updates LVGL's internal original value.
    // Compare with the value captured at open time so Enter still commits a
    // keyboard change through the normal LV_EVENT_VALUE_CHANGED path.
    if (key == LV_KEY_ENTER && self->_open_dropdown == target && lv_dropdown_is_open(target))
    {
        bool changed = lv_dropdown_get_selected(target) != self->_open_dropdown_original_index;
        self->_open_dropdown = nullptr;
        lv_dropdown_close(target);
        lv_obj_clear_state(target, LV_STATE_EDITED);
        if (self->group())
            lv_group_set_editing(self->group(), false);
        if (changed)
            lv_event_send(target, LV_EVENT_VALUE_CHANGED, nullptr);
        request_dropdown_refresh();
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
#endif

    if (self->isDropdownRow(target))
    {
        if (key == LV_KEY_ESC && self->_open_dropdown == target)
        {
            self->closeOpenDropdown();
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
            return;
        }

        if (!self->_open_dropdown)
        {
            if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT)
            {
                if (self->group())
                    lv_group_focus_next(self->group());
                lv_event_stop_processing(e);
                lv_event_stop_bubbling(e);
                return;
            }
            if (key == LV_KEY_UP || key == LV_KEY_LEFT)
            {
                if (self->group())
                    lv_group_focus_prev(self->group());
                lv_event_stop_processing(e);
                lv_event_stop_bubbling(e);
                return;
            }
        }
    }

    if (key != LV_KEY_ESC)
        return;
    if (self->_open_dropdown)
    {
        self->closeOpenDropdown();
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    if (!keypad_suppresses_bubble(target))
        return;

    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    _lv_obj_t *const root = self->root();
    if (!root)
        return;
    uint32_t esc = LV_KEY_ESC;
    lv_event_send(root, LV_EVENT_KEY, &esc);
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
    configure_system_row(row, LV_FLEX_FLOW_COLUMN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, false,
                         false);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE |
                            LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    _lv_obj_t *title = lv_label_create(row);
    if(title) lv_label_set_text(title, "Volume");
    configure_system_label(title, lv_pct(100));

    _lv_obj_t *controls = ht_class_create(&ht_volume_controls_class, row);
    if (!controls)
        return row;
    lv_obj_set_size(controls, lv_pct(100), 26);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
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
#if HELTEC_HAS_TOUCH
    lv_obj_set_ext_click_area(button, 5);
#endif
    _lv_obj_t *const label = lv_label_create(button);
    if (label)
    {
        lv_label_set_text(label, text ? text : "");
        lv_obj_center(label);
    }
    bindWidget(button);
    lv_obj_add_event_cb(button, onBuzzerVolumeButtonClicked, LV_EVENT_CLICKED, this);
    *out_button = button;
}

#endif

} // namespace heltec::meshcore::ui
