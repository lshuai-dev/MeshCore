#include "DialogOverlay.hpp"
#include "ui/widgets/WidgetClasses.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

namespace
{
void apply_variant_state(lv_obj_t *object, DialogOverlay::Variant variant)
{
    if (!object)
    {
        return;
    }
    if (variant == DialogOverlay::Variant::Alert)
    {
        lv_obj_add_state(object, LV_STATE_USER_1);
    }
    else if (variant == DialogOverlay::Variant::Fullscreen)
    {
        lv_obj_add_state(object, LV_STATE_USER_2);
    }
}

} // namespace

DialogOverlay::DialogOverlay(biz::IBizFacade &biz, Options options)
    : AbstractOverlay(biz, ht_dialog_overlay_class), _options(options)
{
}

bool DialogOverlay::buildSurface()
{
    if (!AbstractOverlay::buildSurface())
    {
        return false;
    }

    apply_variant_state(_root, _options.variant);
    lv_obj_set_layout(_root, 0);
    if (_options.dismiss_on_backdrop)
    {
        lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(_root, backdropEventCb, LV_EVENT_CLICKED, this);
    }

    _panel = ht_class_create(&ht_dialog_panel_class, _root);
    if (!_panel)
    {
        return false;
    }
    apply_variant_state(_panel, _options.variant);
    lv_obj_set_flex_flow(_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);

    if (_options.has_title)
    {
        _title = ht_class_create(&ht_overlay_header_class, _panel);
        if (!_title)
        {
            return false;
        }
        lv_obj_set_width(_title, lv_pct(100));
        lv_label_set_long_mode(_title, LV_LABEL_LONG_CLIP);
    }

    _content = ht_class_create(&ht_dialog_content_class, _panel);
    if (!_content)
    {
        return false;
    }
    apply_variant_state(_content, _options.variant);
    lv_obj_set_width(_content, lv_pct(100));
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (_options.has_actions)
    {
        _actions = ht_class_create(&ht_dialog_actions_class, _panel);
        if (!_actions)
        {
            return false;
        }
        apply_variant_state(_actions, _options.variant);
        lv_obj_set_width(_actions, lv_pct(100));
        lv_obj_clear_flag(_actions, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
    return true;
}

_lv_obj_t *DialogOverlay::panel() const
{
    return _panel;
}

_lv_obj_t *DialogOverlay::contentHost() const
{
    return _content;
}

_lv_obj_t *DialogOverlay::actionsHost() const
{
    return _actions;
}

void DialogOverlay::setTitle(const char *text)
{
    if (_title)
    {
        /* Dialog titles are owned by the overlay or supplied as static text. */
        lv_label_set_text_static(_title, text ? text : "");
    }
}

void DialogOverlay::onBackdropPressed()
{
}

void DialogOverlay::backdropEventCb(lv_event_t *event)
{
    auto *self = static_cast<DialogOverlay *>(lv_event_get_user_data(event));
    if (!self || lv_event_get_target(event) != self->_root)
    {
        return;
    }
    self->onBackdropPressed();
    lv_event_stop_processing(event);
    lv_event_stop_bubbling(event);
}

} // namespace heltec::meshcore::ui
