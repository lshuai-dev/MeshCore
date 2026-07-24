#include "context_menu.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_motion_scheduler.hpp"
#include "heltec/ui/core/abstract_menu.hpp"
#include "heltec/ui/app/ui_theme.hpp"
#include "ui/menus/context_menu_metrics.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "ui/theme/ui_widget_theme.hpp"

#include <lvgl.h>

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

namespace heltec::meshcore::ui {

_lv_obj_t* ContextMenu::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::ContextMenuRoot);
}

namespace {

struct FrameGeometry {
  lv_coord_t parent_w = 0;
  lv_coord_t parent_h = 0;
  lv_coord_t root_w = 0;
  lv_coord_t root_h = 0;
  lv_coord_t root_x = 0;
  lv_coord_t target_y = 0;
  lv_coord_t start_y = 0;
};

uint16_t contextPageAnimMs(const lv_obj_t* obj) {
  return ui_context_menu_metrics(obj).page_anim_ms;
}

lv_coord_t componentGap() {
#if defined(HELTEC_V4_R8_TFT)
  return LV_DPX(10);
#else
  return 3;
#endif
}

lv_coord_t frameOuterInsetX(const lv_obj_t* obj) {
#if (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  (void)obj;
  return 8;
#else
  const lv_coord_t frame_pad = ui_context_menu_metrics(obj).frame_pad;
  return frame_pad > 0 ? frame_pad : 0;
#endif
}

lv_coord_t frameOuterInsetY(const lv_obj_t* obj) {
#if (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789)
  (void)obj;
  return 18;
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  (void)obj;
  return 6;
#else
  const lv_coord_t frame_pad = ui_context_menu_metrics(obj).frame_pad;
  return frame_pad > 0 ? frame_pad : 0;
#endif
}

FrameGeometry layoutFrame(_lv_obj_t* obj) {
  FrameGeometry geom{};
  if (!obj) return geom;
  _lv_obj_t* parent = lv_obj_get_parent(obj);
  if (!parent) return geom;

  lv_obj_update_layout(parent);
  lv_coord_t parent_w = lv_obj_get_width(parent);
  lv_coord_t parent_h = lv_obj_get_height(parent);

  if (parent_w <= 0) parent_w = lv_disp_get_hor_res(nullptr);
  if (parent_h <= 0) parent_h = lv_disp_get_ver_res(nullptr);

  const lv_coord_t inset_x = frameOuterInsetX(obj);
  const lv_coord_t inset_y = frameOuterInsetY(obj);
  lv_coord_t w = parent_w - inset_x * 2;
  lv_coord_t h = parent_h - inset_y * 2;
  if (w <= 0) w = parent_w;
  if (h <= 0) h = parent_h;
  lv_obj_set_align(obj, LV_ALIGN_TOP_LEFT);
  lv_obj_set_size(obj, w, h);
  geom.parent_w = parent_w;
  geom.parent_h = parent_h;
  geom.root_w = w;
  geom.root_h = h;
  geom.root_x = (parent_w > w) ? (parent_w - w) / 2 : 0;
  geom.target_y = (parent_h > h) ? (parent_h - h) / 2 : 0;
  geom.start_y = -h;
  lv_obj_set_pos(obj, geom.root_x, geom.start_y);
  lv_obj_update_layout(obj);
  return geom;
}

}  // namespace

bool ContextMenu::isEmpty() const {
  return !_header_icon_row || lv_obj_get_child_cnt(_header_icon_row) == 0;
}

bool ContextMenu::canOpen() const {
  return _root && _menu && _header_icon_row && _focus_group && !isEmpty();
}

lv_group_t* ContextMenu::group() const {
  if (_pending_menu_bind) return nullptr;
  if (_in_leaf && _leaf_menu && _leaf_menu->group()) return _leaf_menu->group();
  return _focus_group;
}

lv_obj_t* ContextMenu::focusedObject() const {
  if (_pending_menu_bind) return nullptr;
  if (_in_leaf && _leaf_menu) {
    if (lv_obj_t* foc = _leaf_menu->focusedObject()) return foc;
    if (lv_group_t* g = _leaf_menu->group()) return lv_group_get_focused(g);
    return nullptr;
  }
  if (!_header_icon_row) return nullptr;
  const uint32_t n = lv_obj_get_child_cnt(_header_icon_row);
  if (n == 0) return nullptr;
  const uint32_t idx = (_active_menu < n) ? _active_menu : 0;
  return lv_obj_get_child(_header_icon_row, idx);
}

void ContextMenu::updateIconSelection() {
  if (!_header_icon_row) return;
  const uint32_t n = lv_obj_get_child_cnt(_header_icon_row);
  for (uint32_t i = 0; i < n; ++i) {
    lv_obj_t* b = lv_obj_get_child(_header_icon_row, i);
    if (!b) continue;
    if (i == _active_menu) {
      lv_obj_add_state(b, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(b, LV_STATE_CHECKED);
    }
  }
}

void ContextMenu::selectPane(uint8_t index) {
  if (_registering_panes || !_header_icon_row || !_menu) return;
  const uint32_t n = lv_obj_get_child_cnt(_header_icon_row);
  if (n == 0) return;

  _active_menu = static_cast<uint8_t>(index % n);
  updateIconSelection();

  lv_obj_t* btn = lv_obj_get_child(_header_icon_row, _active_menu);
  auto* menu = btn ? static_cast<AbstractMenu*>(ht_user_data(btn)) : nullptr;
  if (!menu || !menu->page()) return;

  if (lv_menu_get_cur_main_page(_menu) != menu->page()) {
    lv_menu_set_page(_menu, menu->page());
  }
}

void ContextMenu::enterMenuLeaf(AbstractMenu* menu) {
  if (_registering_panes || !menu || !menu->group()) return;
  _in_leaf = true;
  _leaf_menu = menu;
  _pending_leaf_bind = true;
  menu->resetInputState(false);
  schedulePendingBind();
}

bool ContextMenu::activatePaneButton(lv_obj_t* btn, bool enter_leaf) {
  if (!btn || !_root || lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN)) return false;

  const uint8_t index = static_cast<uint8_t>(lv_obj_get_index(btn));
  selectPane(index);
  if (!enter_leaf || _pending_menu_bind) return true;

  auto* menu = static_cast<AbstractMenu*>(ht_user_data(btn));
  if (!menu) return false;
  enterMenuLeaf(menu);
  return true;
}

void ContextMenu::bindPendingLeaf() {
  if (!_pending_leaf_bind || !_in_leaf || !_leaf_menu || !_leaf_menu->group()) return;
  _pending_leaf_bind = false;
  _leaf_menu->resetInputState(true);
  emitEvent(UiEventType::RebindInput);
}

void ContextMenu::bindPendingMenu() {
  if (!_pending_menu_bind || !_root || lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN)) return;
  _pending_menu_bind = false;
  emitEvent(UiEventType::RebindInput);
}

void ContextMenu::schedulePendingBind(uint32_t delay_ms) {
  if (_pending_bind_timer) {
    lv_timer_set_period(_pending_bind_timer, delay_ms);
    lv_timer_set_repeat_count(_pending_bind_timer, -1);
    lv_timer_reset(_pending_bind_timer);
    lv_timer_resume(_pending_bind_timer);
    return;
  }
}

void ContextMenu::cancelPendingBind() {
  if (!_pending_bind_timer) return;
  lv_timer_pause(_pending_bind_timer);
}

void ContextMenu::pendingBindTimerCb(lv_timer_t* timer) {
  auto* self = timer ? static_cast<ContextMenu*>(timer->user_data) : nullptr;
  if (!self) return;
  self->bindPendingMenu();
  self->bindPendingLeaf();
  if (self->_pending_menu_bind || self->_pending_leaf_bind) {
    self->schedulePendingBind(10U);
  } else if (timer) {
    lv_timer_pause(timer);
  }
}

void ContextMenu::handleLeafEsc() {
  if (!_in_leaf) return;
  leaveMenuLeaf();
}

void ContextMenu::on_menu_leaf_esc(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY || lv_event_get_key(e) != LV_KEY_ESC) return;
  auto* self = static_cast<ContextMenu*>(lv_event_get_user_data(e));
  if (!self) return;
  self->handleLeafEsc();
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

void ContextMenu::leaveMenuLeaf() {
  if (!_in_leaf) return;
  _pending_leaf_bind = false;
  if (_leaf_menu) _leaf_menu->resetInputState(false);
  _in_leaf = false;
  _leaf_menu = nullptr;
  emitEvent(UiEventType::RebindInput);
}

void ContextMenu::onEnter() {
  if (!canOpen()) return;
  ui_motion_cancel(_root);
  leaveMenuLeaf();
  _pending_menu_bind = true;
  selectPane(0);
  lv_menu_clear_history(_menu);
  const FrameGeometry geom = layoutFrame(_root);
  const uint16_t anim_ms = contextPageAnimMs(_root);
  if (anim_ms == 0) {
    lv_obj_set_y(_root, geom.target_y);
  } else {
    UiMotionSpec motion;
    motion.target = _root;
    motion.exec = [](void* var, int32_t value) {
      lv_obj_set_y(static_cast<lv_obj_t*>(var), static_cast<lv_coord_t>(value));
    };
    motion.start_value = geom.start_y;
    motion.end_value = geom.target_y;
    motion.duration_ms = anim_ms;
    motion.path = UiMotionPath::EaseOut;
    if (!ui_motion_start(motion)) lv_obj_set_y(_root, geom.target_y);
  }
  schedulePendingBind();
}

void ContextMenu::onExit() {
  _pending_menu_bind = false;
  _pending_leaf_bind = false;
  cancelPendingBind();
  leaveMenuLeaf();
  if (_root) ui_motion_cancel(_root);
}

_lv_obj_t* ContextMenu::create(_lv_obj_t* parent) {
  if (!parent) return nullptr;
  if (_root) return _root;
  if (!UiSurface::create(parent)) return nullptr;

  lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, componentGap(), LV_PART_MAIN);

  _menu = ht_menu_create(_root, meta_id::ContextMenuMenu);
  if (!_menu) return nullptr;
  lv_obj_set_size(_menu, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_grow(_menu, 1);
  lv_obj_set_style_pad_all(_menu, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_menu, componentGap(), LV_PART_MAIN);
  lv_obj_clear_flag(_menu, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(_menu, [](lv_event_t* e) {
    const UiEvent* event = ui_event_get(e);
    if (!event || event->type != UiEventType::RebindInput) return;
    auto* self = static_cast<ContextMenu*>(lv_event_get_user_data(e));
    if (!self) return;
    self->emitEvent(UiEventType::RebindInput);
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
  }, ui_event_code(), this);

  lv_obj_t* mh = lv_menu_get_main_header(_menu);
  if (!mh) return nullptr;
  ht_set_meta_id(mh, meta_id::ContextMenuHeader);
  lv_obj_set_size(mh, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(mh, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(mh, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(mh, 0, LV_PART_MAIN);

  lv_obj_t* back_btn = lv_menu_get_main_header_back_btn(_menu);
  lv_obj_t* title_lbl = nullptr;
  const uint32_t header_child_count = lv_obj_get_child_cnt(mh);
  for (uint32_t i = 0; i < header_child_count; ++i) {
    lv_obj_t* child = lv_obj_get_child(mh, i);
    if (child != back_btn && lv_obj_check_type(child, &lv_label_class)) {
      title_lbl = child;
      break;
    }
  }
  if (!back_btn || !title_lbl) return nullptr;

  lv_obj_clear_flag(mh, LV_OBJ_FLAG_HIDDEN);

  _header_icon_row = ht_obj_create(mh, meta_id::ContextMenuHeaderIconRow);
  if (!_header_icon_row) return nullptr;
  const UiContextMenuMetrics& metrics = ui_context_menu_metrics(_root);
  lv_obj_set_size(_header_icon_row, lv_pct(100),
                  metrics.icon_btn + 2 * metrics.icon_pad);
  lv_obj_set_flex_flow(_header_icon_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(_header_icon_row, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_header_icon_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(_header_icon_row, metrics.icon_pad, LV_PART_MAIN);
  lv_obj_move_to_index(_header_icon_row, 0);
  lv_obj_clear_flag(_header_icon_row, LV_OBJ_FLAG_SCROLLABLE);

  ht_set_meta_id(back_btn, meta_id::ContextMenuBackButton);
  ht_set_meta_id(title_lbl, meta_id::ContextMenuTitle);

  lv_obj_t* nav_row = ht_obj_create(mh, meta_id::ContextMenuHeaderNavRow);
  if (!nav_row) return nullptr;
  lv_obj_set_size(nav_row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(nav_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(nav_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(nav_row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(nav_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_parent(title_lbl, nav_row);
  lv_obj_set_parent(back_btn, nav_row);
  lv_obj_set_size(back_btn, 0, 0);
  lv_obj_add_flag(back_btn, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_width(title_lbl, lv_pct(100));
  lv_obj_set_flex_grow(title_lbl, 1);
  lv_obj_set_style_pad_all(title_lbl, 0, LV_PART_MAIN);
  lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
  lv_obj_clear_flag(title_lbl, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_to_index(nav_row, 1);

  ui_widget_theme_apply(mh);
  ui_widget_theme_apply(back_btn);
  ui_widget_theme_apply(title_lbl);

  lv_group_set_wrap(_focus_group, true);
  lv_group_set_editing(_focus_group, false);
  if (!_pending_bind_timer) {
    _pending_bind_timer = lv_timer_create(pendingBindTimerCb, 10U, this);
    if (!_pending_bind_timer) return nullptr;
    lv_timer_set_repeat_count(_pending_bind_timer, -1);
    lv_timer_pause(_pending_bind_timer);
  }
  return _root;
}

void ContextMenu::beginRegister() {
  _registering_panes = true;
  if (_focus_group) lv_group_focus_freeze(_focus_group, true);
}

void ContextMenu::endRegister() {
  _registering_panes = false;
  if (_focus_group) lv_group_focus_freeze(_focus_group, false);
}

bool ContextMenu::registerMenu(const char* title, const lv_img_dsc_t* icon, AbstractMenu& menu) {
  if (!_root || !_header_icon_row || !_menu || !icon || !title) return false;
  if (lv_obj_get_child_cnt(_header_icon_row) >= kMaxPanes) return false;
  if (!menu.create(_menu, title)) return false;

  if (menu.root()) lv_obj_add_event_cb(menu.root(), &ContextMenu::on_menu_leaf_esc, LV_EVENT_KEY, this);
  if (lv_obj_t* page = menu.page()) lv_obj_add_event_cb(page, &ContextMenu::on_menu_leaf_esc, LV_EVENT_KEY, this);

  lv_obj_t* btn = ht_btn_create(_header_icon_row, meta_id::ContextMenuIconButton, &menu);
  if (!btn) return false;
  const UiContextMenuMetrics& metrics = ui_context_menu_metrics(_root);
  const lv_coord_t button_size = metrics.icon_btn + 2 * metrics.icon_pad;
  lv_obj_set_size(btn, button_size, button_size);
  lv_obj_set_style_pad_all(btn, metrics.icon_pad, LV_PART_MAIN);

  lv_obj_add_event_cb(btn, [](lv_event_t* e) {
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_CLICKED) return;
    auto* self = static_cast<ContextMenu*>(lv_event_get_user_data(e));
    if (!self) return;
    const bool enter_leaf = (code == LV_EVENT_CLICKED);
    if (!self->activatePaneButton(lv_event_get_target(e), enter_leaf)) return;
    if (enter_leaf) {
      lv_event_stop_bubbling(e);
      lv_event_stop_processing(e);
    }
  }, LV_EVENT_ALL, this);

  lv_obj_add_event_cb(btn, [](lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    auto* ctx_menu = static_cast<ContextMenu*>(lv_event_get_user_data(e));
    if (!ctx_menu) return;

    if (lv_event_get_key(e) != LV_KEY_ESC) return;
    ctx_menu->emitEvent(UiEventType::ContextClose);
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
  }, LV_EVENT_KEY, this);

  lv_obj_t* im = ht_img_create(btn, meta_id::ContextMenuIcon);
  if (!im) return false;
  lv_obj_set_size(im, metrics.icon_btn, metrics.icon_btn);
  lv_img_set_src(im, icon);

  lv_obj_t* page = menu.page();
  if (page) lv_menu_set_load_page_event(_menu, btn, page);
  addFocusObject(btn);
  return true;
}

}  // namespace heltec::meshcore::ui

#endif  // !HELTEC_V4_R8_TFT || !HELTEC_HAS_TOUCH
