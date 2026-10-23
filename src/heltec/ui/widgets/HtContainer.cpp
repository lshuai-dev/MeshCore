#include "HtContainer.hpp"

namespace Heltec::MeshCore::Ui {
namespace {
void set_selected_state(lv_obj_t *object, bool selected) {
    if (!object) {
        return;
    }

    const bool current = lv_obj_has_state(object, LV_STATE_USER_1);
    if (selected && !current) {
        lv_obj_add_state(object, LV_STATE_USER_1);
    } else if (!selected && current) {
        lv_obj_clear_state(object, LV_STATE_USER_1);
    }
    const uint32_t child_count = lv_obj_get_child_cnt(object);
    for (uint32_t index = 0; index < child_count; ++index) {
        lv_obj_t *const child = lv_obj_get_child(object, index);
        set_selected_state(child, selected);
    }
}

void child_focus_changed(lv_event_t *event) {
    if (!event) {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED) {
        return;
    }

    lv_obj_t *const container = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
    lv_obj_t *const child = lv_event_get_target(event);
    if (!container || !child || lv_obj_get_parent(child) != container) {
        return;
    }

    ht_container_set_selected(container, code == LV_EVENT_FOCUSED);
    lv_event_stop_bubbling(event);
}

void container_child_created(lv_event_t *event) {
    if (!event || lv_event_get_code(event) != LV_EVENT_CHILD_CREATED) {
        return;
    }
    lv_obj_t *const container = lv_event_get_current_target(event);
    lv_obj_t *const child = static_cast<lv_obj_t *>(lv_event_get_param(event));
    if (!container || !child || lv_obj_get_parent(child) != container) {
        return;
    }
    lv_obj_add_event_cb(child, child_focus_changed, LV_EVENT_FOCUSED, container);
    lv_obj_add_event_cb(child, child_focus_changed, LV_EVENT_DEFOCUSED, container);
}

void container_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    (void)class_p;
    if (!obj) {
        return;
    }

    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(obj, container_child_created, LV_EVENT_CHILD_CREATED, nullptr);
}

} // namespace

void ht_container_set_selected(lv_obj_t *container, bool selected) {
    if (!container || !lv_obj_has_class(container, &ht_container_class)) {
        return;
    }

    if (lv_obj_has_state(container, LV_STATE_USER_1) == selected) {
        return;
    }
    set_selected_state(container, selected);
}

const lv_obj_class_t ht_container_class = {
    &lv_obj_class,
    container_constructor,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(ht_container_t),
};

lv_obj_t *ht_container_create(lv_obj_t *parent, void *user_data) {
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_container_class, parent);
    if (!obj) {
        return nullptr;
    }

    lv_obj_class_init_obj(obj);

    lv_obj_set_user_data(obj, user_data);

    return obj;
}

} // namespace Heltec::MeshCore::Ui
