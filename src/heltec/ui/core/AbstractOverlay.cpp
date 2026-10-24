#include "AbstractOverlay.hpp"

#include "ui/widgets/WidgetClasses.hpp"

namespace Heltec::MeshCore::Ui {

AbstractOverlay::AbstractOverlay(Biz::IBizFacade &biz)
    : UiSurface(biz) {
}

bool AbstractOverlay::build() {
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
    if (!object || !lv_obj_is_valid(object)) return;
    _gridNav.clear();
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
    if (object == _root) return;
    _gridNav.add(object);
    (void)_gridNav.enterControls();
}

_lv_obj_t *AbstractOverlay::focusTarget() const {
    return _root;
}

} // namespace Heltec::MeshCore::Ui
