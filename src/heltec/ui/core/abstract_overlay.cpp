#include "abstract_overlay.hpp"

#include "ui/widgets/widget_classes.hpp"

namespace heltec::meshcore::ui {

AbstractOverlay::AbstractOverlay(biz::IBizFacade &biz)
    : AbstractOverlay(biz, ht_overlay_root_class) {
}

AbstractOverlay::AbstractOverlay(biz::IBizFacade &biz, const lv_obj_class_t &root_class)
    : UiSurface(biz, root_class) {
}

bool AbstractOverlay::buildSurface() {
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
    return true;
}

void AbstractOverlay::onEnter() {
    if (_root) {
        lv_obj_t *const parent = lv_obj_get_parent(_root);
        if (parent) {
            lv_obj_update_layout(parent);
            const lv_coord_t width = lv_obj_get_width(parent);
            const lv_coord_t height = lv_obj_get_height(parent);
            if (width > 0 && height > 0) {
                lv_obj_set_size(_root, width, height);
                lv_obj_set_pos(_root, 0, 0);
            }
        }
    }
    if (_lv_obj_t *target = focusTarget()) setFocusObject(target);
}

void AbstractOverlay::setFocusObject(_lv_obj_t *object) {
    if (!focusGroup() || !object || !lv_obj_is_valid(object)) return;
    lv_group_set_editing(focusGroup(), false);
    // Reopening a single-target overlay does not require reallocating its group node.
    if (lv_obj_get_group(object) == focusGroup() && lv_group_get_obj_count(focusGroup()) == 1) {
        lv_group_focus_obj(object);
        return;
    }
    lv_group_remove_all_objs(focusGroup());
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(object, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_group_add_obj(focusGroup(), object);
}

_lv_obj_t *AbstractOverlay::focusTarget() const {
    return _root;
}

} // namespace heltec::meshcore::ui
