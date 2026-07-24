#include "ui_navigator.hpp"

#include "ui/core/ht_meta_data.hpp"

#include <lvgl.h>

namespace {

enum {
  kNavigatorLayoutGrid = 1,
};

typedef struct {
  lv_obj_t* content;
  uint8_t layout;
} NavigatorData;

static NavigatorData* navigator_data(lv_obj_t* obj) {
  if (!obj || heltec::meshcore::ui::ht_id(obj) !=
                  heltec::meshcore::ui::meta_id::NavigationPanel) {
    return nullptr;
  }
  return static_cast<NavigatorData*>(heltec::meshcore::ui::ht_user_data(obj));
}

static void release_navigator_data(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
  lv_obj_t* obj = lv_event_get_target(e);
  NavigatorData* w = static_cast<NavigatorData*>(heltec::meshcore::ui::ht_user_data(obj));
  if (!w) return;
  heltec::meshcore::ui::ht_set_user_data(obj, nullptr);
  lv_mem_free(w);
}

static _lv_obj_t* ui_navigator_create_with_layout(_lv_obj_t* parent, uint8_t layout) {
  if (!parent) return nullptr;

  NavigatorData* w = static_cast<NavigatorData*>(lv_mem_alloc(sizeof(NavigatorData)));
  if (!w) return nullptr;
  w->content = nullptr;
  w->layout = layout;

  lv_obj_t* obj = heltec::meshcore::ui::ht_obj_create(
      parent, heltec::meshcore::ui::meta_id::NavigationPanel, w);
  if (!obj) {
    lv_mem_free(w);
    return nullptr;
  }
  lv_obj_add_event_cb(obj, release_navigator_data, LV_EVENT_DELETE, nullptr);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE |
                           LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);

  if (layout == kNavigatorLayoutGrid) {
    w->content = heltec::meshcore::ui::ht_obj_create(
        obj, heltec::meshcore::ui::meta_id::NavigationContent);
    if (!w->content) {
      lv_obj_del(obj);
      return nullptr;
    }
    lv_obj_set_pos(w->content, 0, 0);
    lv_obj_set_size(w->content, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(w->content, 0, LV_PART_MAIN);
    lv_obj_clear_flag(w->content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(w->content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  }
  return obj;
}

}  // namespace

namespace heltec::meshcore::ui {

_lv_obj_t* ui_navigator_create_grid(_lv_obj_t* parent) {
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  return ui_navigator_create_with_layout(parent, kNavigatorLayoutGrid);
#else
  (void)parent;
  return nullptr;
#endif
}

bool ui_navigator_is(_lv_obj_t* obj) {
  return obj && ht_id(obj) == meta_id::NavigationPanel;
}

bool ui_navigator_is_grid(_lv_obj_t* obj) {
  NavigatorData* w = navigator_data(obj);
  return w && w->layout == kNavigatorLayoutGrid;
}

_lv_obj_t* ui_navigator_content(_lv_obj_t* obj) {
  NavigatorData* w = navigator_data(obj);
  if (!w) return ui_navigator_is(obj) ? obj : nullptr;
  return w->content ? w->content : obj;
}

}  // namespace heltec::meshcore::ui
