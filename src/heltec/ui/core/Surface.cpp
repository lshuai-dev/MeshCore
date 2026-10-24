#include "Surface.hpp"

namespace Heltec::MeshCore::Ui {

UiSurface::UiSurface(Biz::IBizFacade &biz) : _biz(biz) {
}

bool UiSurface::init(lv_obj_t *parent, lv_obj_t *app_target,
                     const lv_obj_class_t &root_class) {
    if (_root) return true;
    if (!parent) {
        return false;
    }

    _app_target = app_target;
    _root = lv_obj_class_create_obj(&root_class, parent);
    if (!_root) {
        return false;
    }
    lv_obj_class_init_obj(_root);
    lv_obj_set_user_data(_root, this);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    _gridNav.bindRoot(_root);

    const bool handlers_ready =
        nullptr != lv_obj_add_event_cb(
                       _root, handleKeyEvent,
                       static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this) &&
        nullptr != lv_obj_add_event_cb(_root, handleFocusedEvent, LV_EVENT_FOCUSED, this) &&
        (!acceptsAppEvents() ||
         nullptr != lv_obj_add_event_cb(_root, handleAppEvent, UI_MESSAGE_EVENT, this));
    if (!handlers_ready || !build()) {
        if (_root) {
            lv_obj_del(_root);
            _root = nullptr;
        }
        _app_target = nullptr;
        return false;
    }

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

void UiSurface::handleAppEvent(lv_event_t *event) {
    if (!event) return;
    auto *const self = static_cast<UiSurface *>(lv_event_get_user_data(event));
    const UiMessage *const ui_event = getUiMessage(event);
    if (!self || !ui_event) return;
    self->onAppEvent(*ui_event);
}

bool UiSurface::notifyApp(UiMessageType type, const void *payload) const {
    return _app_target && sendUiMessage(_app_target, type, payload, _root);
}

bool UiSurface::showAlert(const char *text, uint16_t duration_ms) const {
    UiAlertRequest request{};
    request.text = text;
    request.duration_ms = duration_ms;
    return notifyApp(UiMessageType::AlertOpen, &request);
}

} // namespace Heltec::MeshCore::Ui
