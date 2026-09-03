#include "confirm_overlay.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ui_semantic_classes.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

namespace
{

const char *confirmation_body(UiConfirmAction action)
{
    return action == UiConfirmAction::FactoryReset ? "Erase all settings and data?"
                                                   : "Clear contacts and user data?";
}

} // namespace

_lv_obj_t *ConfirmOverlay::create(_lv_obj_t *parent)
{
    if (!DialogOverlay::create(parent))
        return nullptr;

    lv_disp_t *const display = lv_disp_get_default();
    const bool compact =
        display && (lv_disp_get_hor_res(display) <= 160 || lv_disp_get_ver_res(display) <= 128);
    lv_obj_set_width(panel(), compact ? lv_pct(94) : lv_pct(86));
    setDialogTitle("Confirm");

    _body = ht_semantic_label_create(&ht_confirm_text_class, contentHost(),
                                     confirmation_body(_action));
    if (!_body)
        return nullptr;
    lv_obj_set_width(_body, lv_pct(100));
    lv_label_set_long_mode(_body, LV_LABEL_LONG_WRAP);

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    _lv_obj_t *const button_row = actionsHost();
    if (!button_row)
        return nullptr;
    lv_obj_set_height(button_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    const auto create_button = [this, button_row](const char *label_text, _lv_obj_t **out_button)
    {
        _lv_obj_t *const button =
            ht_semantic_button_create(&ht_confirm_button_class, button_row);
        if (!button)
            return;
        lv_obj_set_height(button, LV_SIZE_CONTENT);
        lv_obj_set_width(button, lv_pct(46));
        lv_obj_add_event_cb(button, onButtonClicked, LV_EVENT_CLICKED, this);
        _lv_obj_t *const label =
            ht_semantic_label_create(&ht_confirm_text_class, button, label_text);
        if (label)
            lv_obj_center(label);
        *out_button = button;
    };

    create_button("Cancel", &_cancel);
    create_button("Confirm", &_accept);
    if (!_cancel || !_accept)
        return nullptr;
#else
    _lv_obj_t *const key_hint = ht_semantic_label_create(
        &ht_confirm_key_hint_class, actionsHost(), operation_hint::kDestructiveConfirm);
    if (key_hint)
    {
        lv_obj_set_width(key_hint, lv_pct(100));
        lv_label_set_long_mode(key_hint, LV_LABEL_LONG_CLIP);
    }
#endif

    lv_obj_update_layout(_root);
    lv_obj_center(panel());
    return _root;
}

_lv_obj_t *ConfirmOverlay::focusTarget() const
{
    return _root;
}

void ConfirmOverlay::setRequest(UiConfirmAction action)
{
    _action = action;
    if (_body)
        lv_label_set_text_static(_body, confirmation_body(_action));
}

bool ConfirmOverlay::onKey(uint32_t key)
{
    if (key == LV_KEY_ESC)
        return emit(EventType::CONFIRM_CANCELLED);
    if (key == LV_KEY_ENTER)
        return emit(EventType::CONFIRM_ACCEPTED, &_action);
    return false;
}

void ConfirmOverlay::onBackdropPressed()
{
    (void)emit(EventType::CONFIRM_CANCELLED);
}

void ConfirmOverlay::onButtonClicked(lv_event_t *e)
{
    auto *self = static_cast<ConfirmOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *const target = lv_event_get_target(e);
    if (target == self->_cancel)
    {
        (void)self->emit(EventType::CONFIRM_CANCELLED);
    }
    else if (target == self->_accept)
    {
        (void)self->emit(EventType::CONFIRM_ACCEPTED, &self->_action);
    }
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

} // namespace heltec::meshcore::ui
