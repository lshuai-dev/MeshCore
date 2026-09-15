#include "confirm_overlay.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/widget_classes.hpp"

#include <lvgl.h>

#if defined(MESH_DEBUG) && MESH_DEBUG && defined(ARDUINO)
#include <Arduino.h>
#define CONFIRM_LOG(format, ...)                                      \
    do                                                                \
    {                                                                 \
        Serial.printf("[confirm:ovl] " format "\n", ##__VA_ARGS__); \
        Serial.flush();                                               \
    } while (0)
#else
#define CONFIRM_LOG(...) do { } while (0)
#endif

namespace heltec::meshcore::ui
{

namespace
{

constexpr uint32_t kInputGuardMs = 500;

const char *confirmation_body(UiConfirmAction action)
{
    if (action == UiConfirmAction::FactoryReset)
        return "Erase all settings and data?";
    if (action == UiConfirmAction::ClearData)
        return "Clear contacts and user data?";
    if (action == UiConfirmAction::TeamLeave)
        return "Leave Team and erase its key?";
    return "Approve this Team member?";
}

} // namespace

void ConfirmOverlay::onEnter()
{
    DialogOverlay::onEnter();
    if (!_root || !panel())
        return;

    _entered_ms = lv_tick_get();
    _lv_obj_t *const parent = lv_obj_get_parent(_root);
    if (parent)
    {
        lv_obj_update_layout(parent);
        const lv_coord_t width = lv_obj_get_width(parent);
        const lv_coord_t height = lv_obj_get_height(parent);
        if (width > 0 && height > 0)
        {
            lv_obj_set_size(_root, width, height);
            lv_obj_set_pos(_root, 0, 0);
        }
    }
    lv_obj_update_layout(panel());
    lv_obj_center(panel());
    lv_obj_update_layout(_root);
    lv_obj_invalidate(_root);

    lv_area_t panel_area{};
    lv_obj_get_coords(panel(), &panel_area);
    CONFIRM_LOG("enter action=%u session=%08lX root=%p hidden=%u size=%dx%d "
                "panel=%p area=(%d,%d)-(%d,%d) body=%dx%d",
                static_cast<unsigned>(_action),
                static_cast<unsigned long>(_team_session), _root,
                lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN) ? 1U : 0U,
                static_cast<int>(lv_obj_get_width(_root)),
                static_cast<int>(lv_obj_get_height(_root)), panel(),
                static_cast<int>(panel_area.x1), static_cast<int>(panel_area.y1),
                static_cast<int>(panel_area.x2), static_cast<int>(panel_area.y2),
                _body ? static_cast<int>(lv_obj_get_width(_body)) : -1,
                _body ? static_cast<int>(lv_obj_get_height(_body)) : -1);
}

bool ConfirmOverlay::buildSurface()
{
    if (!DialogOverlay::buildSurface())
        return false;


    lv_disp_t *const display = lv_disp_get_default();
    const bool compact =
        display && (lv_disp_get_hor_res(display) <= 160 || lv_disp_get_ver_res(display) <= 128);
    lv_obj_set_width(panel(), compact ? lv_pct(94) : lv_pct(86));
    setTitle("Confirm");

    _body = ht_label_create(&ht_confirm_text_class, contentHost(),
                                     _action == UiConfirmAction::TeamJoin ? _body_text :
                                     confirmation_body(_action));
    if (!_body)
        return false;
    lv_obj_set_width(_body, lv_pct(100));
    lv_label_set_long_mode(_body, LV_LABEL_LONG_WRAP);

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    _lv_obj_t *const button_row = actionsHost();
    if (!button_row)
        return false;
    lv_obj_set_height(button_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    const auto create_button = [this, button_row](const char *label_text, _lv_obj_t **out_button)
    {
        _lv_obj_t *const button =
            ht_btn_create(&ht_confirm_button_class, button_row);
        if (!button)
            return;
        lv_obj_set_height(button, LV_SIZE_CONTENT);
        lv_obj_set_width(button, lv_pct(46));
        lv_obj_add_event_cb(button, onButtonClicked, LV_EVENT_CLICKED, this);
        _lv_obj_t *const label =
            ht_label_create(&ht_confirm_text_class, button, label_text);
        if (label)
            lv_obj_center(label);
        *out_button = button;
    };

    create_button("Cancel", &_cancel);
    create_button("Confirm", &_accept);
    if (!_cancel || !_accept)
        return false;
#else
    _lv_obj_t *const key_hint = ht_label_create(
        &ht_overlay_bar_class, actionsHost(), operation_hint::kDestructiveConfirm);
    if (key_hint)
    {
        lv_obj_set_width(key_hint, lv_pct(100));
        lv_label_set_long_mode(key_hint, LV_LABEL_LONG_CLIP);
    }
#endif

    lv_obj_update_layout(_root);
    lv_obj_center(panel());
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
        lv_label_set_text_static(_body, _action == UiConfirmAction::TeamJoin ?
                                _body_text : confirmation_body(_action));
}

bool ConfirmOverlay::onKey(uint32_t key)
{
    CONFIRM_LOG("key=%lu age=%lu", static_cast<unsigned long>(key),
                static_cast<unsigned long>(lv_tick_elaps(_entered_ms)));
    if (lv_tick_elaps(_entered_ms) < kInputGuardMs)
        return true;
    if (key == LV_KEY_ESC)
        return emitEvent(EventType::ConfirmCancelled);
    if (key == LV_KEY_ENTER)
        return emitEvent(EventType::ConfirmAccepted, &_action);
    return false;
}

void ConfirmOverlay::onBackdropPressed()
{
    (void)emitEvent(EventType::ConfirmCancelled);
}

void ConfirmOverlay::onButtonClicked(lv_event_t *e)
{
    auto *self = static_cast<ConfirmOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *const target = lv_event_get_target(e);
    const uint32_t age_ms = lv_tick_elaps(self->_entered_ms);
    CONFIRM_LOG("button target=%p accept=%u age=%lu", target,
                target == self->_accept ? 1U : 0U, static_cast<unsigned long>(age_ms));
    if (age_ms < kInputGuardMs)
    {
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    if (target == self->_cancel)
    {
        (void)self->emitEvent(EventType::ConfirmCancelled);
    }
    else if (target == self->_accept)
    {
        (void)self->emitEvent(EventType::ConfirmAccepted, &self->_action);
    }
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

} // namespace heltec::meshcore::ui
