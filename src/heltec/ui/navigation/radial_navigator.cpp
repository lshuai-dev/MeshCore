#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
#include "radial_navigator.hpp"
#include "ui/app/ui_app_frame_metrics.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_motion_scheduler.hpp"
#include "ui/core/ui_events.h"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "ui/theme/ui_widget_theme.hpp"
#include "ui/widgets/top_pane.hpp"
#include "heltec/ui/images.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef HELTEC_TOUCH_GESTURE_SWIPE_PX
#define HELTEC_TOUCH_GESTURE_SWIPE_PX 24
#endif

namespace heltec::meshcore::ui {
namespace {
struct NavImgButtonStyleSet {
  bool ready = false;
  lv_opa_t recolor_opa = LV_OPA_TRANSP;
  lv_style_t idle;
  lv_style_t focus;
};

static NavImgButtonStyleSet s_nav_imgbtn_styles[2];

static bool navImgHeader(_lv_obj_t* btn, lv_img_header_t* hdr) {
  if (!btn || !hdr) return false;
  const void* src = lv_imgbtn_get_src_left(btn, LV_IMGBTN_STATE_RELEASED);
  return src && LV_RES_OK == lv_img_decoder_get_info(src, hdr);
}

static uint8_t navButtonId(_lv_obj_t* btn) {
  return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(ht_user_data(btn)));
}

static void setNavButtonClickPad(_lv_obj_t* btn, lv_coord_t pad) {
  if (!btn) return;
  // lv_imgbtn draws the left source at object origin; use hit padding without moving the bitmap.
  lv_obj_set_ext_click_area(btn, pad > 0 ? pad : 0);
}

static bool navIconUsesRecolor(const lv_img_dsc_t* img) {
  if (!img) return true;
  switch (img->header.cf) {
    case LV_IMG_CF_ALPHA_1BIT:
    case LV_IMG_CF_ALPHA_2BIT:
    case LV_IMG_CF_ALPHA_4BIT:
    case LV_IMG_CF_ALPHA_8BIT:
    case LV_IMG_CF_INDEXED_1BIT:
    case LV_IMG_CF_INDEXED_2BIT:
    case LV_IMG_CF_INDEXED_4BIT:
    case LV_IMG_CF_INDEXED_8BIT:
      return true;
    default:
      return false;
  }
}

static NavImgButtonStyleSet* navImgButtonStyles(lv_opa_t recolor_opa) {
  for (NavImgButtonStyleSet& slot : s_nav_imgbtn_styles) {
    if (slot.ready && slot.recolor_opa == recolor_opa) return &slot;
  }
  for (NavImgButtonStyleSet& slot : s_nav_imgbtn_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.recolor_opa = recolor_opa;

    lv_style_init(&slot.idle);
    lv_style_set_bg_opa(&slot.idle, LV_OPA_TRANSP);
    lv_style_set_border_width(&slot.idle, 0);
    lv_style_set_shadow_width(&slot.idle, 0);
    lv_style_set_pad_all(&slot.idle, 0);
    lv_style_set_img_recolor(&slot.idle, ui_color_fg());
    lv_style_set_img_recolor_opa(&slot.idle, recolor_opa);

    lv_style_init(&slot.focus);
    lv_style_set_bg_opa(&slot.focus, LV_OPA_TRANSP);
    lv_style_set_border_width(&slot.focus, 0);
    lv_style_set_img_recolor(&slot.focus, ui_color_accent());
    lv_style_set_img_recolor_opa(&slot.focus, recolor_opa);
    return &slot;
  }
  return nullptr;
}

static void style_nav_imgbtn(_lv_obj_t* btn, const lv_img_dsc_t* img) {
  if (!btn) return;
  lv_obj_remove_style_all(btn);
  const lv_opa_t recolor_opa = navIconUsesRecolor(img) ? LV_OPA_COVER : LV_OPA_TRANSP;
  NavImgButtonStyleSet* styles = navImgButtonStyles(recolor_opa);
  if (!styles) return;
  static const lv_style_selector_t idle_sels[] = {LV_PART_MAIN, LV_PART_MAIN | LV_STATE_PRESSED};
  static const lv_style_selector_t focus_sels[] = {
      LV_PART_MAIN | LV_STATE_FOCUS_KEY, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_PRESSED,
      LV_PART_MAIN | LV_STATE_FOCUSED, LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_PRESSED,
  };
  for (lv_style_selector_t sel : idle_sels) {
    lv_obj_add_style(btn, &styles->idle, sel);
  }
  for (lv_style_selector_t sel : focus_sels) {
    lv_obj_add_style(btn, &styles->focus, sel);
  }
}

static bool isNavStepKey(uint32_t key) {
  return key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP ||
         key == LV_KEY_NEXT || key == LV_KEY_RIGHT || key == LV_KEY_DOWN;
}

static int navStepDelta(uint32_t key) {
  return (key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP) ? -1 : 1;
}

static bool navTouchMovedBeyondThreshold(const lv_point_t& origin,
                                         const lv_point_t& current) {
  const int32_t dx = static_cast<int32_t>(current.x) - origin.x;
  const int32_t dy = static_cast<int32_t>(current.y) - origin.y;
  const int32_t abs_dx = dx < 0 ? -dx : dx;
  const int32_t abs_dy = dy < 0 ? -dy : dy;
  return abs_dx >= HELTEC_TOUCH_GESTURE_SWIPE_PX ||
         abs_dy >= HELTEC_TOUCH_GESTURE_SWIPE_PX;
}

static void layoutRootBelowTopPane(_lv_obj_t* root) {
  if (!root) return;
  _lv_obj_t* parent = lv_obj_get_parent(root);

  lv_coord_t parent_w = 0;
  lv_coord_t parent_h = 0;
  if (parent) {
    lv_obj_update_layout(parent);
    parent_w = lv_obj_get_width(parent);
    parent_h = lv_obj_get_height(parent);
  }
  if (parent_w <= 0) parent_w = lv_disp_get_hor_res(nullptr);
  if (parent_h <= 0) parent_h = lv_disp_get_ver_res(nullptr);
  if (parent_w <= 0 || parent_h <= 0) return;

  lv_coord_t top_h = ui_top_pane_metrics(root).height;
  if (top_h < 0) top_h = 0;
  lv_coord_t top_pad = ui_app_frame_metrics(root).frame_margin_top;
  if (top_pad < 0) top_pad = 0;
  top_h = static_cast<lv_coord_t>(top_h + top_pad);
  if (top_h > parent_h) top_h = parent_h;
#if defined(HELTEC_LORA_V4_OLED)
  constexpr lv_coord_t kTopPaneClearance = 2;
  top_h = (top_h + kTopPaneClearance > parent_h) ? parent_h
                                                 : static_cast<lv_coord_t>(top_h + kTopPaneClearance);
#endif

  lv_coord_t bottom_pad = ui_app_frame_metrics(root).frame_margin_bottom;
  if (bottom_pad < 0) bottom_pad = 0;

  lv_coord_t h = parent_h - top_h - bottom_pad;
  if (h < 0) h = 0;
  lv_obj_set_size(root, parent_w, h);
  lv_obj_set_pos(root, 0, top_h);
}

static void layoutSquareInParent(_lv_obj_t* obj) {
  if (!obj) return;
  _lv_obj_t* parent = lv_obj_get_parent(obj);
  if (!parent) return;

  lv_obj_update_layout(parent);
  const lv_coord_t parent_w = lv_obj_get_width(parent);
  const lv_coord_t parent_h = lv_obj_get_height(parent);
  if (parent_w <= 0 || parent_h <= 0) return;

  const lv_coord_t side = LV_MIN(parent_w, parent_h);
  const lv_coord_t x = parent_w > side ? (parent_w - side) / 2 : 0;
  const lv_coord_t y = parent_h > side ? (parent_h - side) / 2 : 0;
  if (lv_obj_get_width(obj) != side || lv_obj_get_height(obj) != side) {
    lv_obj_set_size(obj, side, side);
  }
  if (lv_obj_get_x(obj) != x || lv_obj_get_y(obj) != y) {
    lv_obj_set_pos(obj, x, y);
  }
}

}  // namespace

RadialNavigator::~RadialNavigator() {
  clearAnimations();
}

void RadialNavigator::configure(const UiNavigationItem* items, uint8_t count) {
  if (!items) return;
  if (count > kMaxButtons) count = kMaxButtons;
  for (uint8_t i = 0; i < count; ++i) {
    const UiNavigationItem& item = items[i];
    if (item.screen_index >= kMaxButtons || !item.icon) continue;
    setIcon(item.screen_index, item.icon);
  }
}

_lv_obj_t* RadialNavigator::itemHost() const {
  return _nav;
}

_lv_obj_t* RadialNavigator::findCellById(uint8_t id) const {
  _lv_obj_t* host = itemHost();
  if (!host) return nullptr;
  const uint32_t n = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (cell && navButtonId(cell) == id) {
      return cell;
    }
  }
  return nullptr;
}

uint8_t RadialNavigator::btnCount() const {
  _lv_obj_t* host = itemHost();
  return host ? static_cast<uint8_t>(lv_obj_get_child_cnt(host)) : 0;
}

uint8_t RadialNavigator::slotForId(uint8_t id) const {
  _lv_obj_t* host = itemHost();
  if (!host) return 0;
  const uint32_t n = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* btn = lv_obj_get_child(host, i);
    if (!btn) continue;
    const uint8_t child_id = navButtonId(btn);
    if (child_id == id) return static_cast<uint8_t>(i);
  }
  return 0;
}

uint8_t RadialNavigator::focusedIndex() const {
  return _ring_layout_focus != kNoEmphasis ? _ring_layout_focus : 0;
}

uint8_t RadialNavigator::focusedSlot() const {
  const uint8_t n = btnCount();
  return n ? static_cast<uint8_t>(_ring_focus_slot % n) : 0;
}

bool RadialNavigator::panelVisible() const {
  return _nav && !lv_obj_has_flag(_nav, LV_OBJ_FLAG_HIDDEN);
}

void RadialNavigator::setNavButtonsInteractive(bool interactive) {
  _lv_obj_t* host = itemHost();
  if (!host) return;
  const uint32_t n = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* btn = lv_obj_get_child(host, i);
    if (!btn) continue;
    if (interactive) {
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    }
  }
}

void RadialNavigator::clearAnimations() {
  if (_nav) ui_motion_cancel(_nav);
  ui_motion_cancel(this);
}

void RadialNavigator::invalidateSlotCache() {
  _slot_cache_count = 0;
}

void RadialNavigator::rebuildSlotCache(uint8_t count) {
  if (count == 0 || count > kMaxButtons || _slot_cache_count == count) return;
  for (uint8_t i = 0; i < count; ++i) {
    const float angle = 2.f * static_cast<float>(M_PI) * i / count;
    _slot_cos[i] = cosf(angle);
    _slot_sin[i] = sinf(angle);
  }
  _slot_cache_count = count;
}

void RadialNavigator::layoutRing(bool animate, bool snap_theta, bool update_emphasis) {
  const uint8_t n = btnCount();
  if (n == 0 || !_nav) return;
  rebuildSlotCache(n);

  const UiNavigationMetrics& metrics = ui_navigation_metrics(_nav);
  const lv_coord_t focus_extra = metrics.focus_extra;
  const uint8_t focus = focusedIndex();
  const uint8_t focus_slot = focusedSlot();
  const float target_abs =
      -static_cast<float>(M_PI / 2.0) - (2.f * static_cast<float>(M_PI) * focus_slot / n);
  float target = snap_theta ? target_abs : _ring_theta;
  if (snap_theta) {
    float diff = target - _ring_theta;
    while (diff > static_cast<float>(M_PI)) {
      target -= 2.f * static_cast<float>(M_PI);
      diff = target - _ring_theta;
    }
    while (diff < -static_cast<float>(M_PI)) {
      target += 2.f * static_cast<float>(M_PI);
      diff = target - _ring_theta;
    }
  }

  const uint16_t ms = animate ? metrics.ring_anim_ms : 0;
  if (animate && ms != 0 && snap_theta && fabsf(target - _ring_theta) >= 0.001f) {
    _emphasis_index = kNoEmphasis;
    layoutRing(false, false, false);
    clearAnimations();
    UiMotionSpec motion;
    motion.target = this;
    motion.exec = [](void* var, int32_t v) {
      auto* self = static_cast<RadialNavigator*>(var);
      self->_ring_theta = v / 1000.f;
      self->layoutRing(false, false, false);
    };
    motion.ready = [](void* user_data) {
      auto* self = static_cast<RadialNavigator*>(user_data);
      if (!self) return;
      self->_emphasis_index = self->focusedIndex();
      self->layoutRing(false, false, false);
    };
    motion.ready_data = this;
    motion.start_value = static_cast<int32_t>(roundf(_ring_theta * 1000.f));
    motion.end_value = static_cast<int32_t>(roundf(target * 1000.f));
    motion.duration_ms = ms;
    motion.path = UiMotionPath::EaseInOut;
    if (ui_motion_start(motion)) return;
    _ring_theta = target;
    _emphasis_index = focus;
  }

  if (snap_theta) _ring_theta = target;
  if (update_emphasis) _emphasis_index = focus;

  const lv_coord_t w = lv_obj_get_width(_nav);
  const lv_coord_t h = lv_obj_get_height(_nav);
  if (w < 8 || h < 8) return;

  const float cx = (static_cast<float>(w) - 1.f) * 0.5f;
  const float cy = (static_cast<float>(h) - 1.f) * 0.5f;
  lv_coord_t max_visual_side = 0;
  for (uint8_t i = 0; i < n; ++i) {
    _lv_obj_t* btn = lv_obj_get_child(itemHost(), i);
    lv_img_header_t hdr;
    if (!btn || !navImgHeader(btn, &hdr)) continue;
    max_visual_side = LV_MAX(max_visual_side, static_cast<lv_coord_t>(LV_MAX(hdr.w, hdr.h)));
  }
  if (max_visual_side == 0) return;

  const lv_coord_t edge_pad = metrics.ring_edge_pad;
  float R = (static_cast<float>(LV_MIN(w, h)) - static_cast<float>(max_visual_side)) * 0.5f -
            static_cast<float>(edge_pad);
  if (R < 0.f) R = 0.f;
  const float theta_cos = cosf(_ring_theta);
  const float theta_sin = sinf(_ring_theta);

  for (uint8_t i = 0; i < n; ++i) {
    _lv_obj_t* btn = lv_obj_get_child(itemHost(), i);
    lv_img_header_t hdr;
    if (!btn || !navImgHeader(btn, &hdr)) continue;
    const uint8_t id = navButtonId(btn);
    const bool emphasized = (_emphasis_index != kNoEmphasis && id == _emphasis_index);
    const lv_coord_t click_pad =
        metrics.min_touch_pad + (emphasized ? (focus_extra + 1) / 2 : 0);
    const float x_unit = _slot_cos[i] * theta_cos - _slot_sin[i] * theta_sin;
    const float y_unit = _slot_sin[i] * theta_cos + _slot_cos[i] * theta_sin;
    const float icon_cx = cx + R * x_unit;
    const float icon_cy = cy + R * y_unit;
    const lv_coord_t icon_x =
        static_cast<lv_coord_t>(roundf(icon_cx - (static_cast<float>(hdr.w) - 1.f) * 0.5f));
    const lv_coord_t icon_y =
        static_cast<lv_coord_t>(roundf(icon_cy - (static_cast<float>(hdr.h) - 1.f) * 0.5f));

    if (panelVisible()) {
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_set_size(btn, static_cast<lv_coord_t>(hdr.w), static_cast<lv_coord_t>(hdr.h));
    lv_obj_set_pos(btn, icon_x, icon_y);
    setNavButtonClickPad(btn, click_pad);
    if (_emphasis_index != kNoEmphasis && id == _emphasis_index) {
      lv_obj_add_state(btn, LV_STATE_FOCUS_KEY);
    } else {
      lv_obj_clear_state(btn, LV_STATE_FOCUS_KEY);
    }
  }
  lv_obj_invalidate(_nav);
}

void RadialNavigator::updateGeometry() {
  if (!_root || !_nav || _updating_geometry) return;
  _updating_geometry = true;

  layoutRootBelowTopPane(_root);

  lv_obj_update_layout(_root);
  const lv_coord_t pw = lv_obj_get_width(_root);
  const lv_coord_t ph = lv_obj_get_height(_root);
  if (pw >= 8 && ph >= 8) {
    layoutSquareInParent(_nav);
    const lv_coord_t nav_x = lv_obj_get_x(_nav);
    const lv_coord_t nav_y = lv_obj_get_y(_nav);
    const lv_coord_t nav_side = lv_obj_get_width(_nav);
    const bool visible = panelVisible();
    const bool geometry_changed =
        !_geometry_valid || _cached_panel_visible != visible ||
        _cached_nav_x != nav_x || _cached_nav_y != nav_y ||
        _cached_nav_side != nav_side;
    if (!panelVisible()) {
      const uint8_t n = btnCount();
      if (n != 0) {
        const uint8_t focus_slot = focusedSlot();
        _ring_theta =
            -static_cast<float>(M_PI / 2.0) - (2.f * static_cast<float>(M_PI) * focus_slot / n);
        _emphasis_index = focusedIndex();
      }
    }
    if (geometry_changed) layoutRing(false, false);
    _cached_panel_visible = visible;
    _cached_nav_x = nav_x;
    _cached_nav_y = nav_y;
    _cached_nav_side = nav_side;
    _geometry_valid = true;
  }
  _updating_geometry = false;
}

_lv_obj_t* RadialNavigator::create(_lv_obj_t* parent) {
  if (_root) return _root;
  if (!UiSurface::create(parent)) return nullptr;
  ht_set_meta_id(_root, meta_id::NavigationRoot);
  ui_widget_theme_apply(_root);
  layoutRootBelowTopPane(_root);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_group_set_wrap(group(), false);

  _nav = ht_obj_create(_root, meta_id::NavigationRing);
  if (!_nav) {
    if (_focus_group) {
      lv_group_del(_focus_group);
      _focus_group = nullptr;
    }
    lv_obj_del(_root);
    _root = nullptr;
    return nullptr;
  }
  lv_obj_set_style_pad_all(_nav, 0, LV_PART_MAIN);
  lv_obj_clear_flag(_nav, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(_nav, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE |
                           LV_OBJ_FLAG_HIDDEN);
  addFocusObject(_nav);
  lv_obj_add_event_cb(_nav, [](lv_event_t* e) {
    if (LV_EVENT_KEY != lv_event_get_code(e)) return;
    auto* self = static_cast<RadialNavigator*>(lv_event_get_user_data(e));
    if (!self || !self->onKey(lv_event_get_key(e))) return;
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
  }, LV_EVENT_KEY, this);
  lv_obj_add_event_cb(_nav, [](lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    auto* self = static_cast<RadialNavigator*>(lv_event_get_user_data(e));
    if (!self || !self->panelVisible() || !self->_tileview) return;
    if (self->_ring_layout_focus >= kMaxButtons) return;
    ui_event_send(self->_tileview, UiEventType::TileCommit,
                  &self->_ring_layout_focus);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
  }, LV_EVENT_CLICKED, this);

  lv_obj_add_event_cb(_root, [](lv_event_t* e) {
    auto* self = static_cast<RadialNavigator*>(lv_event_get_user_data(e));
    if (!self || !self->_nav || lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
    self->updateGeometry();
  }, LV_EVENT_SIZE_CHANGED, this);
  lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
  updateGeometry();
  lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
  return _root;
}

void RadialNavigator::setIcon(uint8_t id, const lv_img_dsc_t* img) {
  if (id >= kMaxButtons || !img || !_nav || !group()) return;
#if defined(UI_NAV_USE_SCREEN_ICONS) && UI_NAV_USE_SCREEN_ICONS
  const lv_img_dsc_t* nav_img = img;
#else
  const lv_img_dsc_t* nav_img = nav_ring_icon_for(img);
#endif
  _lv_obj_t* host = itemHost();
  if (!host) return;

  if (_lv_obj_t* existing = findCellById(id)) {
    lv_imgbtn_set_src(existing, LV_IMGBTN_STATE_PRESSED, nav_img, nullptr, nullptr);
    lv_imgbtn_set_src(existing, LV_IMGBTN_STATE_RELEASED, nav_img, nullptr, nullptr);
    style_nav_imgbtn(existing, nav_img);
    _geometry_valid = false;
    layoutRing(false);
    return;
  }

  _lv_obj_t* btn = ht_imgbtn_create(
      host, meta_id::NavigationRingItem, reinterpret_cast<void*>(static_cast<uintptr_t>(id)));
  if (!btn) return;
  invalidateSlotCache();
  _geometry_valid = false;
  lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, nav_img, nullptr, nullptr);
  lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, nav_img, nullptr, nullptr);
  style_nav_imgbtn(btn, nav_img);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  if (panelVisible()) {
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t* e) {
    auto* nav = static_cast<RadialNavigator*>(lv_event_get_user_data(e));
    if (!nav) return;
    nav->onCellTouchEvent(e);
    if (LV_EVENT_CLICKED != lv_event_get_code(e)) return;
    if (!nav->panelVisible()) return;
    if (nav->_touch_dragged) {
      nav->_touch_active = false;
      nav->_touch_dragged = false;
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
      return;
    }
    nav->_touch_active = false;
    nav->onCellClicked(lv_event_get_target(e));
  }, LV_EVENT_ALL, this);
  layoutRing(false);
}

void RadialNavigator::setSelectedIndex(uint8_t id, bool preview) {
  if (id >= kMaxButtons || !_nav || !findCellById(id)) return;
  _ring_layout_focus = id;
  _ring_focus_slot = slotForId(id);
  if (!panelVisible()) return;
  layoutRing(!preview);
  if (preview) sendTilePreview(id);
}

void RadialNavigator::resetPanel() {
  clearAnimations();
  if (!_nav) return;
  lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(_nav);
  setNavButtonsInteractive(false);
  layoutRing(false);
}

void RadialNavigator::ensureDefaultFocus() {
  if (_ring_layout_focus != kNoEmphasis || btnCount() == 0) return;
  _lv_obj_t* host = itemHost();
  if (!host) return;
  _lv_obj_t* first = lv_obj_get_child(host, 0);
  if (!first) return;
  _ring_layout_focus = navButtonId(first);
  _ring_focus_slot = 0;
}

void RadialNavigator::onEnter() {
  UiSurface::onEnter();
  if (!_root || !_nav || btnCount() == 0) return;
  openPanel();
}

void RadialNavigator::onExit() {
  resetPanel();
  UiSurface::onExit();
}

uint16_t RadialNavigator::inputRebindDelayMs() const {
  return 0;
}

void RadialNavigator::sendTilePreview(uint8_t tile_idx) {
  if (_tileview && tile_idx < kMaxButtons) {
    ui_event_send(_tileview, UiEventType::TilePreview, &tile_idx);
  }
}

_lv_obj_t* RadialNavigator::frameRoot() const {
  if (_frame_root) return _frame_root;
  return _root;
}

void RadialNavigator::onCellTouchEvent(lv_event_t* e) {
  if (!e) return;
  const lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
      code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) {
    return;
  }

  _lv_obj_t* cell = lv_event_get_target(e);
  if (!cell) return;
  lv_indev_t* indev = lv_event_get_indev(e);
  if (!indev) indev = lv_indev_get_act();
  if (!indev) {
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
      _touch_active = false;
    }
    return;
  }

  lv_point_t point;
  lv_indev_get_point(indev, &point);
  if (code == LV_EVENT_PRESSED) {
    _touch_active = true;
    _touch_dragged = false;
    _touch_origin = point;
    return;
  }
  if (code == LV_EVENT_PRESSING) {
    if (_touch_active && !_touch_dragged &&
        navTouchMovedBeyondThreshold(_touch_origin, point)) {
      _touch_dragged = true;
      lv_obj_clear_state(cell, LV_STATE_PRESSED);
    }
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (_touch_dragged) lv_obj_clear_state(cell, LV_STATE_PRESSED);
    if (code == LV_EVENT_PRESS_LOST) _touch_active = false;
  }
}

void RadialNavigator::onCellClicked(_lv_obj_t* cell) {
  if (!cell) return;
  const uint8_t id = navButtonId(cell);
  if (id == _ring_layout_focus) return;
  _ring_layout_focus = id;
  _ring_focus_slot = static_cast<uint8_t>(lv_obj_get_index(cell));
  layoutRing(true);
  sendTilePreview(id);
  if (_lv_obj_t* frame = frameRoot()) {
    ui_event_send(frame, UiEventType::NavActivity);
  }
}

void RadialNavigator::stepNavFocus(int delta) {
  if (!itemHost() || delta == 0) return;
  const uint8_t n = btnCount();
  if (n == 0) return;
  int next = (static_cast<int>(_ring_focus_slot) + delta) % static_cast<int>(n);
  if (next < 0) next += static_cast<int>(n);
  _lv_obj_t* btn = lv_obj_get_child(itemHost(), static_cast<uint32_t>(next));
  if (!btn) return;
  _ring_focus_slot = static_cast<uint8_t>(next);
  _ring_layout_focus = navButtonId(btn);
  layoutRing(true);
  sendTilePreview(_ring_layout_focus);
}

bool RadialNavigator::onKey(uint32_t lv_key) {
  if (!panelVisible()) return false;
  if (lv_key == LV_KEY_ESC) {
    if (_lv_obj_t* frame = frameRoot()) {
      ui_event_send(frame, UiEventType::NavClose);
    }
    return true;
  }
  if (isNavStepKey(lv_key)) {
    if (_lv_obj_t* frame = frameRoot()) {
      ui_event_send(frame, UiEventType::NavActivity);
    }
    stepNavFocus(navStepDelta(lv_key));
    return true;
  }
  return false;
}

void RadialNavigator::openPanel() {
  if (!_root || !_nav) return;
  clearAnimations();
  setNavButtonsInteractive(false);
  ensureDefaultFocus();
  updateGeometry();
  lv_obj_clear_flag(_nav, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(_nav);
  layoutRing(false);
  setNavButtonsInteractive(true);
  if (_lv_group_t* g = group()) {
    lv_group_focus_obj(_nav);
  }
}

}  // namespace heltec::meshcore::ui

#endif  // !UI_NAVIGATION_GRID
