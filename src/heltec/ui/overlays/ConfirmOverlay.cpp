#include "ConfirmOverlay.hpp"
#include "ui/core/OperationHints.hpp"
#include "ui/widgets/WidgetClasses.hpp"

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

namespace
{

constexpr uint32_t kInputGuardMs = 500;

const char *confirmation_body(UiConfirmAction action)
{
    switch (action)
    {
    case UiConfirmAction::FactoryReset:
        return "Erase all settings and data?";
    case UiConfirmAction::ClearData:
        return "Clear contacts and user data?";
    case UiConfirmAction::TeamLeave:
        return "Leave Team and erase its key?";
    case UiConfirmAction::TeamJoin:
        return "Approve this Team member?";
    default:
        return "Confirm this action?";
    }
}

} // namespace

void ConfirmOverlay::onEnter()
{
    if (!panel())
    {
        return;
    }

    _entered_ms = lv_tick_get();
    lv_obj_update_layout(_root);
    lv_obj_center(panel());
    lv_obj_invalidate(_root);
}

bool ConfirmOverlay::build()
{
    if (!DialogOverlay::build())
    {
        return false;
    }

    setTitle("Confirm");

    const char *const body_text = _action == UiConfirmAction::TeamJoin
                                      ? _body_text
                                      : confirmation_body(_action);
    _body = ht_label_create(&ht_confirm_text_class, contentHost(), body_text);
    if (!_body)
    {
        return false;
    }
    lv_obj_set_width(_body, lv_pct(100));
    lv_label_set_long_mode(_body, LV_LABEL_LONG_WRAP);

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    _lv_obj_t *const button_row = actionsHost();
    if (!button_row)
    {
        return false;
    }
    lv_obj_set_height(button_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    const auto create_button = [this, button_row](const char *label_text) -> _lv_obj_t *
    {
        _lv_obj_t *const button = ht_btn_create(&ht_confirm_button_class, button_row);
        if (!button)
        {
            return nullptr;
        }
        lv_obj_set_height(button, LV_SIZE_CONTENT);
        lv_obj_set_width(button, lv_pct(46));
        lv_obj_add_event_cb(button, onButtonClicked, LV_EVENT_CLICKED, this);
        _lv_obj_t *const label = ht_label_create(&ht_confirm_text_class, button, label_text);
        if (label)
        {
            lv_obj_center(label);
        }
        return button;
    };

    _cancel = create_button("Cancel");
    _accept = create_button("Confirm");
    if (!_cancel || !_accept)
    {
        return false;
    }
#else
    _lv_obj_t *const key_hint = ht_label_create(
        &ht_overlay_footer_class, actionsHost(), OperationHint::kDestructiveConfirm);
    if (key_hint)
    {
        lv_obj_set_width(key_hint, lv_pct(100));
        lv_label_set_long_mode(key_hint, LV_LABEL_LONG_CLIP);
    }
#endif

    return true;
}

void ConfirmOverlay::setRequest(UiConfirmAction action)
{
    _action = action;
    _team_session = 0;
    if (_action == UiConfirmAction::TeamJoin)
    {
        char name[32] = {};
        uint16_t code = 0;
        (void)_biz.teamPendingDetails(name, sizeof(name), _team_session, code);
        lv_snprintf(_body_text, sizeof(_body_text), "%s\nPairing code %04u\nApprove?",
                    name[0] ? name : "Team member",
                    static_cast<unsigned>(code));
    }
    if (_body)
    {
        lv_label_set_text_static(_body, _action == UiConfirmAction::TeamJoin ?
                                _body_text : confirmation_body(_action));
    }
}

bool ConfirmOverlay::onKey(uint32_t key)
{
    if (lv_tick_elaps(_entered_ms) < kInputGuardMs)
    {
        return true;
    }
    if (key == LV_KEY_ESC)
    {
        return notifyApp(EventType::ConfirmCancelled);
    }
    if (key == LV_KEY_ENTER)
    {
        return notifyApp(EventType::ConfirmAccepted, &_action);
    }
    return false;
}

void ConfirmOverlay::onBackdropPressed()
{
    (void)notifyApp(EventType::ConfirmCancelled);
}

void ConfirmOverlay::onButtonClicked(lv_event_t *e)
{
    auto *self = static_cast<ConfirmOverlay *>(lv_event_get_user_data(e));
    if (!self)
    {
        return;
    }
    _lv_obj_t *const target = lv_event_get_target(e);
    const uint32_t age_ms = lv_tick_elaps(self->_entered_ms);
    if (age_ms < kInputGuardMs)
    {
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    if (target == self->_cancel)
    {
        (void)self->notifyApp(EventType::ConfirmCancelled);
    }
    else if (target == self->_accept)
    {
        (void)self->notifyApp(EventType::ConfirmAccepted, &self->_action);
    }
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

} // namespace Heltec::MeshCore::Ui
