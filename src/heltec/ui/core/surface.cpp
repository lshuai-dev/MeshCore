#include "Surface.hpp"

namespace Heltec::MeshCore::Ui {

UiSurface::UiSurface(Biz::IBizFacade &biz, const lv_obj_class_t &root_class)
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
    lv_obj_set_user_data(_root, this);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    _gridNav.bindRoot(_root);

    if (!installEventHandlers() || !buildSurface()) {
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

bool UiSurface::installEventHandlers() {
    if(nullptr == lv_obj_add_event_cb(
                       _root, handleKeyEvent,
                       static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this) ||
        nullptr == lv_obj_add_event_cb(_root, handleFocusedEvent, LV_EVENT_FOCUSED, this) ||
        nullptr == lv_obj_add_event_cb(_root, handleUIEvent, HT_EVENT_USER, this)) return false;

    return true;
}

void UiSurface::handleFocusedEvent(lv_event_t *event) {
    if (!event || lv_event_get_code(event) != LV_EVENT_FOCUSED) return;
    auto *const self = static_cast<UiSurface *>(lv_event_get_user_data(event));
    lv_obj_t *const target = lv_event_get_target(event);
    if (!self || target == self->_root || !self->_gridNav.contains(target) ||
        self->_gridNav.focused() == target) return;
    (void)self->_gridNav.focus(target);
}

void UiSurface::handleKeyEvent(lv_event_t *event) {
    if (!event || lv_event_get_code(event) != LV_EVENT_KEY) return;
    auto *const self = static_cast<UiSurface *>(lv_event_get_user_data(event));
    if (!self || lv_event_get_target(event) != self->_root) return;
    const uint32_t key = lv_event_get_key(event);
    if (!self->onKey(key) && !self->_gridNav.handleKey(key)) return;
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

bool UiSurface::showAlert(const char *text, uint16_t duration_ms) const {
    UiAlertRequest request{};
    request.text = text;
    request.duration_ms = duration_ms;
    return emitEvent(EventType::AlertOpen, &request);
}

} // namespace Heltec::MeshCore::Ui
