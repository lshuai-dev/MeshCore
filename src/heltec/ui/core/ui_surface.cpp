#include "ui_surface.hpp"

#include "ht_meta_data.hpp"
#include "ht_event.h"

#include <lvgl.h>

namespace heltec::meshcore::ui
{
namespace
{

void stop_focus_event_bubbling(lv_event_t *event)
{
    if (!event)
    {
        return;
    }
    lv_event_stop_bubbling(event);
}

} // namespace

bool UiSurface::init(_lv_obj_t *parent)
{
    if (_root)
        return true;
    return create(parent) != nullptr;
}

_lv_obj_t *UiSurface::createRoot(_lv_obj_t *parent)
{
    return ht_obj_create(parent);
}

_lv_obj_t *UiSurface::create(_lv_obj_t *parent)
{
    if (!parent) return nullptr;
    if (_root) return _root;

    _root = createRoot(parent);
    if (!_root) return nullptr;

    _focus_group = lv_group_create();
    if (!_focus_group)
    {
        lv_obj_del(_root);
        _root = nullptr;
        return _root;
    }

    lv_group_set_wrap(_focus_group, true);
    lv_group_set_editing(_focus_group, false);

    lv_obj_add_event_cb(_root, [](lv_event_t *e) {
        if (LV_EVENT_KEY != lv_event_get_code(e)) return;
        auto *self = static_cast<UiSurface *>(lv_event_get_user_data(e));
        if (!self || !self->onKey(lv_event_get_key(e))) return;
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
    }, LV_EVENT_KEY, this);

    lv_obj_add_event_cb(_root, [](lv_event_t *e) {
        const UiEvent *event = ht_event_get(e);
        auto *self = static_cast<UiSurface *>(lv_event_get_user_data(e));
        if (!self || !event) return;
        if (event->type == EventType::APP_STATE_CHANGED) {
            const auto *state = static_cast<const AppStateEvent *>(event->payload);
            if (state) self->onAppStateChanged(*state);
        }
        else if (event->type == EventType::SURFACE_REFRESH) {
                self->onSurfaceRefresh();
            }
            else
            {
                self->onUiEvent(*event);
            }
        },
        HT_EVENT_USER, this);

    return _root;
}

_lv_obj_t *UiSurface::defaultFocus() const
{
    return _default_focus && lv_obj_is_valid(_default_focus) ? _default_focus : nullptr;
}

_lv_obj_t *UiSurface::focusedObject() const
{
    if (_focus_group)
    {
        if (lv_obj_t *foc = lv_group_get_focused(_focus_group))
            return foc;
    }
    return defaultFocus();
}

void UiSurface::addFocusObject(_lv_obj_t *obj)
{
    if (!_focus_group || !obj)
        return;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    while (lv_obj_remove_event_cb(obj, stop_focus_event_bubbling))
    {
    }
    lv_obj_add_event_cb(obj, stop_focus_event_bubbling,
                        static_cast<lv_event_code_t>(LV_EVENT_FOCUSED | LV_EVENT_PREPROCESS),
                        nullptr);
    lv_obj_add_event_cb(obj, stop_focus_event_bubbling,
                        static_cast<lv_event_code_t>(LV_EVENT_DEFOCUSED | LV_EVENT_PREPROCESS),
                        nullptr);
    lv_group_add_obj(_focus_group, obj);
    if (!_default_focus || !lv_obj_is_valid(_default_focus))
        _default_focus = obj;
    for (_lv_obj_t *p = lv_obj_get_parent(obj); p && p != _root; p = lv_obj_get_parent(p))
    {
        lv_obj_add_flag(p, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
}

void UiSurface::clearFocusObjects()
{
    if (!_focus_group)
        return;
    lv_group_remove_all_objs(_focus_group);
    _default_focus = nullptr;
}

bool UiSurface::emit(EventType type, const void *payload) const
{
    return ht_event_send(_event_target, type, payload);
}

} // namespace heltec::meshcore::ui
