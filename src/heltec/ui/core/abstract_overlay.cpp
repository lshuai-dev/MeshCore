#include "abstract_overlay.hpp"

#include "ht_meta_data.hpp"

namespace heltec::meshcore::ui {

_lv_obj_t* AbstractOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::OverlayRoot);
}

_lv_obj_t* AbstractOverlay::create(_lv_obj_t* parent) {
  if (!parent) return nullptr;
  if (_root) return _root;
  if (!UiSurface::create(parent)) return nullptr;

  lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
  return _root;
}

_lv_obj_t* AbstractOverlay::focusedObject() const {
  if (_lv_obj_t* target = focusTarget()) return target;
  return UiSurface::focusedObject();
}

void AbstractOverlay::onEnter() {
  UiSurface::onEnter();
  if (_lv_obj_t* target = focusTarget()) setFocusObject(target);
}

void AbstractOverlay::setFocusObject(_lv_obj_t* obj) {
  if (!_focus_group || !obj) return;
  clearFocusObjects();
  addFocusObject(obj);
}

_lv_obj_t* AbstractOverlay::focusTarget() const {
  return _root;
}

bool AbstractOverlay::onKey(uint32_t key) {
  (void)key;
  return false;
}

}  // namespace heltec::meshcore::ui
