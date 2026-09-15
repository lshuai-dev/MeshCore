#include "surface.hpp"

namespace heltec::meshcore::ui {

UiSurface::UiSurface(biz::IBizFacade &biz, const lv_obj_class_t &root_class)
    : _biz(biz), _root_class(root_class) {
}

bool UiSurface::init(lv_obj_t *parent, lv_obj_t *event_target) {
    if (_inited) return true;
    if (!parent) {
        return false;
    }

    _event_target = event_target;
    _root = lv_obj_class_create_obj(&_root_class, parent);
    if (!_root) {
        return false;
    }
    lv_obj_class_init_obj(_root);

    if (!createFocusGroup() || !installEventHandlers() || !buildSurface()) {
        if (_focus_group) {
            lv_group_del(_focus_group);
            _focus_group = nullptr;
        }
        if (_root) {
            lv_obj_del(_root);
            _root = nullptr;
        }
        _event_target = nullptr;
        return false;
    }

    _inited = true;
    return _inited;
}

bool UiSurface::createFocusGroup() {
    _focus_group = lv_group_create();
    if (!_focus_group) return false;

    lv_group_set_wrap(_focus_group, true);
    lv_group_set_editing(_focus_group, false);
    return true;
}

bool UiSurface::installEventHandlers() {
    if(nullptr == lv_obj_add_event_cb(_root, handleKeyEvent, LV_EVENT_KEY, this) ||
        nullptr == lv_obj_add_event_cb(_root, handleUIEvent, HT_EVENT_USER, this)) return false;

    return true;
}

void UiSurface::handleKeyEvent(lv_event_t *event) {
    if (!event || lv_event_get_code(event) != LV_EVENT_KEY) return;
    auto *const self = static_cast<UiSurface *>(lv_event_get_user_data(event));
    if (!self || !self->onKey(lv_event_get_key(event))) return;
    lv_event_stop_processing(event);
    lv_event_stop_bubbling(event);
}

void UiSurface::handleUIEvent(lv_event_t *event) {
    if (!event) return;
    auto *const self = static_cast<UiSurface *>(lv_event_get_user_data(event));
    const UiEvent *const ui_event = ht_event_get(event);
    if (!self || !ui_event) return;
    self->onUIEvent(*ui_event);
}

bool UiSurface::emitEvent(EventType type, const void *payload) const {
    return _event_target && ht_event_send(_event_target, type, payload);
}

} // namespace heltec::meshcore::ui
