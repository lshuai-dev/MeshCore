#include "AbstractOverlay.hpp"

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

} // namespace Heltec::MeshCore::Ui
