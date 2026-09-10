#include "dialog_overlay.hpp"
#include "ui_semantic_classes.hpp"

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
    : AbstractOverlay(biz), _options(options)
{
}

_lv_obj_t *DialogOverlay::createRoot(_lv_obj_t *parent)
{
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_dialog_overlay_class, parent);
    if (!obj) {
        return nullptr;
    }

    lv_obj_class_init_obj(obj);

    return obj;
}

_lv_obj_t *DialogOverlay::create(_lv_obj_t *parent)
{
    if (!AbstractOverlay::create(parent)) return nullptr;
    if (_panel) return _root;

    apply_variant_state(_root, _options.variant);
    lv_obj_set_layout(_root, 0);
    if (_options.dismiss_on_backdrop)
    {
        lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(_root, backdropEventCb, LV_EVENT_CLICKED, this);
    }

    auto create_object = [](const lv_obj_class_t * class_p, lv_obj_t * parent) -> lv_obj_t * {
        lv_obj_t *const obj = lv_obj_class_create_obj(class_p, parent);
        if (!obj) {
            return nullptr;
        }

        lv_obj_class_init_obj(obj);

        return obj;
    };
    

    _panel = create_object(&ht_dialog_panel_class, _root);
    if (!_panel)
    {
        return nullptr;
    }
    apply_variant_state(_panel, _options.variant);
    lv_obj_set_flex_flow(_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);

    if (_options.has_title)
    {
        _title = create_object(&ht_dialog_title_class, _panel);
        if (!_title)
        {
            return nullptr;
        }
        lv_obj_set_width(_title, lv_pct(100));
        lv_label_set_long_mode(_title, LV_LABEL_LONG_CLIP);
    }

    _content = create_object(&ht_dialog_content_class, _panel);
    if (!_content)
    {
        return nullptr;
    }
    apply_variant_state(_content, _options.variant);
    lv_obj_set_width(_content, lv_pct(100));
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (_options.has_actions)
    {
        _actions = create_object(&ht_dialog_actions_class, _panel);
        if (!_actions)
        {
            return nullptr;
        }
        apply_variant_state(_actions, _options.variant);
        lv_obj_set_width(_actions, lv_pct(100));
        lv_obj_clear_flag(_actions, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }
    return _root;
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

void DialogOverlay::setDialogTitle(const char *text)
{
    if (_title)
    {
        lv_label_set_text(_title, text ? text : "");
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
