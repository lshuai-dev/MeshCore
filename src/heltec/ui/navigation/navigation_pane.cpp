#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
#include "navigation_pane.hpp"
#include "ui/app/ui_app_frame_metrics.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_motion_scheduler.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "ui/theme/ui_widget_theme.hpp"
#include "ui/widgets/top_pane.hpp"
#include "ui/core/ui_events.h"
#include "heltec/drivers/input/touch_port.hpp"
#include "heltec/ui/images.h"
#include "MeshCore.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef HELTEC_TOUCH_GESTURE_SWIPE_PX
#define HELTEC_TOUCH_GESTURE_SWIPE_PX 24
#endif

namespace heltec::meshcore::ui {

namespace {

static void layoutGridTitleBar(_lv_obj_t* bar) {
  if (!bar) return;
  lv_obj_set_flex_grow(bar, 0);
  lv_obj_set_height(bar, LV_SIZE_CONTENT);
  _lv_obj_t* lbl = lv_obj_get_child(bar, 0);
  if (!lbl) return;
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lbl, lv_pct(100));
}

#if defined(HELTEC_V4_R8_TFT)
static constexpr lv_coord_t kGridIconPadPx = 0;
#else
static constexpr lv_coord_t kGridIconPadPx = 4;
#endif
static constexpr lv_coord_t kGridIconTitleGapPx = 3;
static constexpr lv_coord_t kGridTitleBottomMarginPx = 4;

static uint16_t gridNavOpenAnimMs(const lv_obj_t* obj) {
  return ui_navigation_metrics(obj).open_anim_ms;
}

static uint16_t gridNavCloseAnimMs(const lv_obj_t* obj) {
  return static_cast<uint16_t>((static_cast<uint32_t>(gridNavOpenAnimMs(obj)) * 3U) / 4U);
}

static _lv_obj_t* gridCellIconArea(_lv_obj_t* cell) {
  return cell ? lv_obj_get_child(cell, 0) : nullptr;
}

static _lv_obj_t* gridCellIcon(_lv_obj_t* cell) {
  _lv_obj_t* area = gridCellIconArea(cell);
  return area ? lv_obj_get_child(area, 0) : nullptr;
}

static _lv_obj_t* gridCellTitleBar(_lv_obj_t* cell) {
  return cell ? lv_obj_get_child(cell, 1) : nullptr;
}

static void layoutGridIconImage(_lv_obj_t* icon, lv_coord_t max_w, lv_coord_t max_h) {
  if (!icon || max_w < 2 || max_h < 2) return;
  const void* src = lv_img_get_src(icon);
  if (!src) return;
  lv_img_header_t hdr;
  if (lv_img_decoder_get_info(src, &hdr) != LV_RES_OK) return;
  if (hdr.w < 1 || hdr.h < 1) return;

  const uint32_t zw =
      (static_cast<uint32_t>(max_w) * 256U) / static_cast<uint32_t>(hdr.w);
  const uint32_t zh =
      (static_cast<uint32_t>(max_h) * 256U) / static_cast<uint32_t>(hdr.h);
  uint32_t zoom = zw < zh ? zw : zh;
  if (zoom > LV_IMG_ZOOM_NONE) zoom = LV_IMG_ZOOM_NONE;
  if (zoom < 1) zoom = 1;
  lv_img_set_zoom(icon, static_cast<uint16_t>(zoom));

  const lv_coord_t disp_w =
      static_cast<lv_coord_t>((static_cast<uint32_t>(hdr.w) * zoom) / 256U);
  const lv_coord_t disp_h =
      static_cast<lv_coord_t>((static_cast<uint32_t>(hdr.h) * zoom) / 256U);
  lv_obj_set_size(icon, disp_w, disp_h);
  lv_obj_set_flex_grow(icon, 0);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_coord_t gridCellLabelBarHeightPx(const lv_obj_t* obj) {
  return ui_navigation_metrics(obj).grid_label_h + 2;
}

static void layoutGridCellIcon(_lv_obj_t* cell, lv_coord_t cell_w, lv_coord_t cell_h) {
  _lv_obj_t* icon_area = gridCellIconArea(cell);
  _lv_obj_t* icon = gridCellIcon(cell);
  _lv_obj_t* bar = gridCellTitleBar(cell);
  if (!icon_area || !icon || !bar) return;
  lv_obj_set_style_layout(cell, 0, LV_PART_MAIN);
  lv_obj_set_style_layout(cell, 0, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_layout(cell, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
  lv_obj_set_style_layout(cell, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_PRESSED);
  lv_obj_set_style_layout(cell, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_set_style_layout(cell, 0, LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_PRESSED);
  lv_obj_set_width(bar, cell_w);
  layoutGridTitleBar(bar);
  const lv_coord_t bar_h = gridCellLabelBarHeightPx(cell);
  const lv_coord_t area_w = cell_w;
  const lv_coord_t bar_y = LV_MAX(0, cell_h - bar_h - kGridTitleBottomMarginPx);
  const lv_coord_t max_area_h = LV_MAX(2, bar_y - kGridIconTitleGapPx);
  lv_obj_set_flex_grow(icon_area, 0);
  lv_obj_set_style_layout(icon_area, 0, LV_PART_MAIN);
  lv_obj_set_height(bar, bar_h);
  lv_obj_set_pos(bar, 0, bar_y);
  const lv_coord_t icon_max_h = LV_MAX(2, max_area_h - kGridIconPadPx * 2);
  layoutGridIconImage(icon, area_w - kGridIconPadPx * 2, icon_max_h);
  lv_obj_set_size(icon_area, area_w, max_area_h);
  lv_obj_set_pos(icon_area, 0, 0);
  const lv_coord_t icon_w = lv_obj_get_width(icon);
  const lv_coord_t icon_h = lv_obj_get_height(icon);
  const lv_coord_t icon_x = LV_MAX(0, (area_w - icon_w) / 2);
  const lv_coord_t icon_y = LV_MAX(0, (max_area_h - icon_h) / 2);
  lv_obj_set_pos(icon, icon_x, icon_y);
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

static uint8_t navCellId(_lv_obj_t* cell) {
  return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(ht_user_data(cell)));
}

static void layoutRootBelowTopPane(_lv_obj_t* root) {
  if (!root) return;
  _lv_obj_t* parent = lv_obj_get_parent(root);

  lv_coord_t parent_w = 0;
  lv_coord_t parent_h = 0;
  if (parent) {
    parent_w = lv_obj_get_width(parent);
    parent_h = lv_obj_get_height(parent);
    if (parent_w <= 0 || parent_h <= 0) {
      lv_obj_update_layout(parent);
      parent_w = lv_obj_get_width(parent);
      parent_h = lv_obj_get_height(parent);
    }
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

  lv_coord_t bottom_pad = ui_app_frame_metrics(root).frame_margin_bottom;
  if (bottom_pad < 0) bottom_pad = 0;

  lv_coord_t h = parent_h - top_h - bottom_pad;
  if (h < 0) h = 0;
  lv_obj_set_size(root, parent_w, h);
  lv_obj_set_pos(root, 0, top_h);
}

}  // namespace

NavigationPane::~NavigationPane() {
  if (_nav) ui_motion_cancel(_nav);
  ui_motion_cancel(this);
}

_lv_obj_t* NavigationPane::navButtonHost() const {
  return itemHost();
}

_lv_obj_t* NavigationPane::itemHost() const {
  return ui_navigator_content(_nav);
}

_lv_obj_t* NavigationPane::findCellById(uint8_t id) const {
  _lv_obj_t* host = itemHost();
  if (!host) return nullptr;
  const uint32_t n = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    if (navCellId(cell) == id) {
      return cell;
    }
  }
  return nullptr;
}

uint8_t NavigationPane::btnCount() const {
  _lv_obj_t* host = itemHost();
  if (!host) return 0;
  uint8_t count = 0;
  const uint32_t n = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* child = lv_obj_get_child(host, i);
    if (!child) continue;
    ++count;
  }
  return count;
}

void NavigationPane::layoutNav(bool animate, bool snap_theta, bool update_emphasis) {
  (void)animate;
  (void)snap_theta;
  if (_layout_busy) return;
  _layout_busy = true;
  layoutGrid(update_emphasis);
  _layout_busy = false;
}


void NavigationPane::setLabel(uint8_t id, const char* label) {
  if (id >= kMaxButtons) return;
  _labels[id] = label;
  _lv_obj_t* cell = findCellById(id);
  if (!cell) return;
  _lv_obj_t* bar = gridCellTitleBar(cell);
  if (!bar) return;
  _lv_obj_t* lbl = lv_obj_get_child(bar, 0);
  if (lbl) lv_label_set_text(lbl, label ? label : "");
}

void NavigationPane::setFooterSlot(uint8_t id) {
  if (id >= kMaxButtons) return;
  _footer_id = id;
  if (_lv_obj_t* cell = findCellById(id)) {
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (_lv_obj_t* icon_area = gridCellIconArea(cell)) {
      lv_obj_add_flag(icon_area, LV_OBJ_FLAG_HIDDEN);
    }
    if (_lv_obj_t* bar = gridCellTitleBar(cell)) {
      lv_obj_set_height(bar, lv_pct(100));
      lv_obj_set_flex_grow(bar, 1);
      if (_lv_obj_t* label = lv_obj_get_child(bar, 0)) lv_obj_center(label);
    }
    ui_navigator_apply_footer_cell_theme(cell);
  }
  if (panelVisible()) layoutNav(false);
}

void NavigationPane::layoutGrid(bool update_emphasis) {
  _lv_obj_t* host = itemHost();
  const uint8_t n = btnCount();
  if (n == 0 || !host) return;

  const lv_coord_t pw = lv_obj_get_width(host);
  const lv_coord_t ph = lv_obj_get_height(host);
  if (pw < 8 || ph < 8) return;

  const UiNavigationMetrics& metrics = ui_navigation_metrics(host);
  uint8_t cols = metrics.grid_cols;
  uint8_t rows = metrics.grid_rows;
  if (cols < 1) cols = 1;
  if (rows < 1) rows = 1;
  const lv_coord_t gap = metrics.grid_gap;
  const lv_coord_t pad = metrics.grid_pad;
  const bool has_footer = (_footer_id != static_cast<uint8_t>(eScreenId::None));
  const lv_coord_t footer_h = has_footer ? metrics.grid_footer_h : 0;
  const lv_coord_t footer_gap = has_footer ? gap : 0;
  const lv_coord_t avail_w = pw - pad * 2;
  const lv_coord_t avail_h = ph - pad * 2;
  const lv_coord_t footer_block = has_footer ? (footer_h + footer_gap) : 0;
  const lv_coord_t max_grid_h = avail_h - footer_block;

  const lv_coord_t cell_w_cap =
      (avail_w - gap * (static_cast<lv_coord_t>(cols) - 1)) / cols;
  const lv_coord_t cell_h_cap =
      (max_grid_h - gap * (static_cast<lv_coord_t>(rows) - 1)) / rows;
  const lv_coord_t cell_w = LV_MAX(2, cell_w_cap);
  const lv_coord_t cell_h = LV_MAX(2, cell_h_cap);

  const lv_coord_t grid_w = cols * cell_w + gap * (static_cast<lv_coord_t>(cols) - 1);
  const lv_coord_t grid_h = rows * cell_h + gap * (static_cast<lv_coord_t>(rows) - 1);
  const lv_coord_t block_h = grid_h + footer_block;
  const lv_coord_t origin_x = pad + (avail_w - grid_w) / 2;
  const lv_coord_t origin_y = pad + (avail_h - block_h) / 2;

  const uint8_t focus = focusedIndex();
  if (update_emphasis) _emphasis_index = focus;

  uint8_t grid_idx = 0;
  for (uint8_t i = 0; i < n; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    const uint8_t id = navCellId(cell);

    if (!panelVisible()) {
      lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_clear_flag(cell, LV_OBJ_FLAG_HIDDEN);
      if (!_ring_fade_busy) {
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
      } else {
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
      }
    }

    if (has_footer && id == _footer_id) {
      lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      const lv_coord_t y = origin_y + grid_h + footer_gap;
      lv_obj_set_size(cell, avail_w, footer_h);
      lv_obj_set_pos(cell, pad, y);
      if (id == focus) {
        lv_obj_add_state(cell, LV_STATE_FOCUS_KEY);
      } else {
        lv_obj_clear_state(cell, LV_STATE_FOCUS_KEY);
      }
      continue;
    }

    const uint8_t col = grid_idx % cols;
    const uint8_t row = grid_idx / cols;
    ++grid_idx;
    const lv_coord_t x = origin_x + col * (cell_w + gap);
    const lv_coord_t y = origin_y + row * (cell_h + gap);

    lv_obj_set_size(cell, cell_w, cell_h);
    lv_obj_set_pos(cell, x, y);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    layoutGridCellIcon(cell, cell_w, cell_h);

    if (id == focus) {
      lv_obj_add_state(cell, LV_STATE_FOCUS_KEY);
    } else {
      lv_obj_clear_state(cell, LV_STATE_FOCUS_KEY);
    }
  }
  lv_obj_invalidate(host);
}


uint8_t NavigationPane::focusedIndex() const {
  if (_ring_layout_focus != kNoEmphasis) return _ring_layout_focus;
  return 0;
}

bool NavigationPane::panelVisible() const {
  return _nav && !lv_obj_has_flag(_nav, LV_OBJ_FLAG_HIDDEN);
}

void NavigationPane::setNavButtonsInteractive(bool interactive) {
  _lv_obj_t* host = itemHost();
  if (!host) return;
  const uint8_t n = btnCount();
  for (uint8_t i = 0; i < n; ++i) {
    _lv_obj_t* btn = lv_obj_get_child(host, i);
    if (!btn) continue;
    if (interactive) {
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    }
  }
}

static void gridPaneFrameRect(_lv_obj_t* root, _lv_obj_t* tileview, lv_coord_t* out_x,
                              lv_coord_t* out_y, lv_coord_t* out_w,
                              lv_coord_t* out_h, bool update_layout = true) {
  const lv_coord_t pw = root ? lv_obj_get_width(root) : 0;
  const lv_coord_t ph = root ? lv_obj_get_height(root) : 0;
  *out_x = 0;
  *out_y = 0;
  *out_w = pw;
  *out_h = ph;

  if (!root || !tileview || !lv_obj_is_valid(tileview)) return;

  if (update_layout) lv_obj_update_layout(tileview);
  lv_area_t root_coords;
  lv_area_t tile_coords;
  lv_obj_get_coords(root, &root_coords);
  lv_obj_get_coords(tileview, &tile_coords);

  const lv_coord_t tw = lv_area_get_width(&tile_coords);
  const lv_coord_t th = lv_area_get_height(&tile_coords);
  if (tw < 8 || th < 8) return;

  *out_x = tile_coords.x1 - root_coords.x1;
  *out_y = tile_coords.y1 - root_coords.y1;
  *out_w = tw;
  *out_h = th;
}

static lv_coord_t closedPaneX(_lv_obj_t* pane, lv_coord_t open_x) {
  if (!pane) return 0;
  const lv_coord_t w = lv_obj_get_width(pane);
  return w > 0 ? open_x - w : open_x;
}

static void syncPaneRadiusToTileView(_lv_obj_t* pane, _lv_obj_t* tileview) {
  if (!pane || !tileview || !lv_obj_is_valid(tileview)) return;
  const lv_coord_t radius = lv_obj_get_style_radius(tileview, LV_PART_MAIN);
  lv_obj_set_style_radius(pane, radius, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(pane, radius > 0, LV_PART_MAIN);

  _lv_obj_t* host = ui_navigator_content(pane);
  if (!host) return;
  const lv_coord_t title_radius = radius;
  const uint32_t n = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    if (_lv_obj_t* icon = gridCellIcon(cell)) {
      lv_obj_set_style_radius(icon, radius, LV_PART_MAIN);
      lv_obj_set_style_clip_corner(icon, radius > 0, LV_PART_MAIN);
    }
    if (_lv_obj_t* bar = gridCellTitleBar(cell)) {
      lv_obj_set_style_radius(bar, title_radius, LV_PART_MAIN);
      lv_obj_set_style_clip_corner(bar, title_radius > 0, LV_PART_MAIN);
      lv_obj_set_style_radius(bar, title_radius, LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_clip_corner(bar, title_radius > 0, LV_PART_MAIN | LV_STATE_PRESSED);
    }
  }
}


void NavigationPane::updateGeometry() {
  if (!_root || !_nav || _updating_geometry) return;

  _updating_geometry = true;
  layoutRootBelowTopPane(_root);
  if (_frame_root && lv_obj_is_valid(_frame_root)) {
    lv_obj_update_layout(_frame_root);
  } else if (_tileview && lv_obj_is_valid(_tileview)) {
    lv_obj_update_layout(_tileview);
  }
  // _root receives its size and position directly above.  Its child panel is
  // sized and positioned below as well, so a second layout pass here only
  // re-enters LVGL layout handlers without changing the geometry we read.
  const lv_coord_t pw = lv_obj_get_width(_root);
  const lv_coord_t ph = lv_obj_get_height(_root);
  if (pw < 8 || ph < 8) {
    _updating_geometry = false;
    return;
  }
  if (pw >= 8 && ph >= 8) {
    lv_coord_t nx = 0;
    lv_coord_t ny = 0;
    lv_coord_t nw = pw;
    lv_coord_t nh = ph;
    gridPaneFrameRect(_root, _tileview, &nx, &ny, &nw, &nh, false);
    lv_obj_set_size(_nav, nw, nh);
    syncPaneRadiusToTileView(_nav, _tileview);
    lv_obj_set_pos(_nav, panelVisible() ? nx : closedPaneX(_nav, nx), ny);
    if (!panelVisible()) {
      _emphasis_index = focusedIndex();
    }
    layoutNav(false, false);
  }
  _updating_geometry = false;
}

void NavigationPane::setTileView(_lv_obj_t* tileview) {
  _tileview = tileview;
  updateGeometry();
}

_lv_obj_t* NavigationPane::create(_lv_obj_t* parent) {
  if (_root) return _root;
  if (!UiSurface::create(parent)) return nullptr;
  ht_set_meta_id(_root, meta_id::NavigationRoot);
  ui_widget_theme_apply(_root);
  layoutRootBelowTopPane(_root);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_group_set_wrap(group(), false);

  _nav = ui_navigator_create_grid(_root);
  if (!_nav) return nullptr;
  addFocusObject(_nav);

  lv_obj_add_event_cb(_root, [](lv_event_t* e) {
    auto* self = static_cast<NavigationPane*>(lv_event_get_user_data(e));
    if (!self || !self->_nav || lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
    const bool fading = self->_ring_fade_busy;
    ui_motion_cancel(self->_nav);
    ui_motion_cancel(self);
    if (fading) {
      self->_ring_fade_busy = false;
      lv_obj_clear_state(self->_nav, LV_STATE_USER_4);
      lv_obj_clear_flag(self->_nav, LV_OBJ_FLAG_HIDDEN);
      self->setNavButtonsInteractive(true);
      self->layoutNav(false);
    }
    self->updateGeometry();
  }, LV_EVENT_SIZE_CHANGED, this);
  lv_obj_add_state(_nav, LV_STATE_USER_4);
  lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
  updateGeometry();
  lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
  return _root;
}

void NavigationPane::setIcon(uint8_t id, const lv_img_dsc_t* img) {
  if (id >= kMaxButtons || !img) return;
  if (!_nav || !group()) return;

#if defined(UI_NAV_USE_SCREEN_ICONS) && UI_NAV_USE_SCREEN_ICONS
  const lv_img_dsc_t* const nav_img = img;
#else
  const lv_img_dsc_t* const nav_img = nav_ring_icon_for(img);
#endif
  _lv_obj_t* const host = itemHost();
  if (!host) return;

  _lv_obj_t* existing = findCellById(id);
  if (existing) {
    if (_lv_obj_t* icon = gridCellIcon(existing)) lv_img_set_src(icon, nav_img);
    if (panelVisible()) layoutNav(false);
    return;
  }

  auto add_cell_click_cb = [this](_lv_obj_t* cell) {
    lv_obj_add_event_cb(cell, [](lv_event_t* e) {
      auto* nav = static_cast<NavigationPane*>(lv_event_get_user_data(e));
      if (!nav) return;
      nav->onCellTouchEvent(e);
    }, LV_EVENT_ALL, this);
    lv_obj_add_event_cb(cell, [](lv_event_t* e) {
      if (LV_EVENT_CLICKED != lv_event_get_code(e)) return;
      auto* nav = static_cast<NavigationPane*>(lv_event_get_user_data(e));
      if (!nav || !nav->panelVisible() || nav->_ring_fade_busy) return;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
      // Crossing the swipe threshold suppresses LVGL's pointer path by
      // delivering a synthetic release while the finger is still down. Do not
      // turn that release into a navigation click/commit.
      if (heltec::meshcore::dal::touch_port::isPressed()) return;
#endif
      if (nav->_touch_dragged) {
        nav->_touch_active = false;
        nav->_touch_dragged = false;
        return;
      }
      nav->_touch_active = false;
      nav->onCellClicked(lv_event_get_target(e));
      (void)nav->commitFocused();
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
    }, LV_EVENT_CLICKED, this);
  };

  _lv_obj_t* cell = ht_obj_create(
      host, meta_id::NavigationCell, reinterpret_cast<void*>(static_cast<uintptr_t>(id)));
  if (!cell) return;
  lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(cell, 0, LV_PART_MAIN);
  lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(cell, LV_OBJ_FLAG_EVENT_BUBBLE);

  _lv_obj_t* icon_area = ht_obj_create(cell, meta_id::NavigationIconArea);
  if (!icon_area) return;
  lv_obj_set_width(icon_area, lv_pct(100));
  lv_obj_set_flex_grow(icon_area, 1);
  lv_obj_set_flex_flow(icon_area, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(icon_area, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(icon_area, kGridIconPadPx, LV_PART_MAIN);
  lv_obj_clear_flag(icon_area, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(icon_area, LV_OBJ_FLAG_EVENT_BUBBLE);

  _lv_obj_t* icon = ht_img_create(icon_area, meta_id::NavigationIcon);
  if (!icon) return;
  lv_img_set_src(icon, nav_img);
  lv_img_set_antialias(icon, false);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(icon, LV_OBJ_FLAG_EVENT_BUBBLE);

  _lv_obj_t* bar = ht_obj_create(cell, meta_id::NavigationTitleBar);
  if (!bar) return;
  lv_obj_set_width(bar, lv_pct(100));
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_hor(bar, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(bar, 0, LV_PART_MAIN);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(bar, LV_OBJ_FLAG_EVENT_BUBBLE);

  _lv_obj_t* lbl = ht_label_create(bar, meta_id::NavigationTitleLabel,
                                   _labels[id] ? _labels[id] : "");
  if (!lbl) return;
  lv_obj_set_width(lbl, lv_pct(100));
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
  layoutGridTitleBar(bar);

  if (panelVisible() && !_ring_fade_busy) {
    lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  add_cell_click_cb(cell);


  if (panelVisible()) layoutNav(false);
}

void NavigationPane::onCellTouchEvent(lv_event_t* e) {
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
  if (!indev) return;

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

void NavigationPane::setSelectedIndex(uint8_t id, bool preview) {
  if (id >= kMaxButtons || !_nav) return;
  if (!findCellById(id)) return;
  _ring_layout_focus = id;
  if (!panelVisible()) return;
  layoutNav(!preview);
  if (preview) sendTilePreview(id);
}

void NavigationPane::resetPanel() {
  if (_nav) ui_motion_cancel(_nav);
  ui_motion_cancel(this);
  _ring_fade_busy = false;
  _close_animating = false;
  if (!_nav) return;
  lv_obj_add_state(_nav, LV_STATE_USER_4);
  updateGeometry();
  lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(_nav);
  setNavButtonsInteractive(false);
  layoutNav(false);
}

void NavigationPane::onEnter() {
  UiSurface::onEnter();
  _close_animating = false;
  if (!_nav || btnCount() == 0) return;
  openPanel();
}

void NavigationPane::onExit() {
  resetPanel();
}

uint16_t NavigationPane::inputRebindDelayMs() const {
  return 0;
}

void NavigationPane::sendTilePreview(uint8_t tile_idx) {
  if (!_tileview || tile_idx >= kMaxButtons) return;
  ui_event_send(_tileview, UiEventType::TilePreview, &tile_idx);
}

_lv_obj_t* NavigationPane::frameRoot() const {
  if (_frame_root) return _frame_root;
  return _root;
}

void NavigationPane::onCellClicked(_lv_obj_t* cell) {
  if (!cell) return;
  const uint8_t id = navCellId(cell);
  if (id == _ring_layout_focus) return;
  _ring_layout_focus = id;
  layoutNav(true);
  sendTilePreview(_ring_layout_focus);
  if (_lv_obj_t* frame = frameRoot()) {
    ui_event_send(frame, UiEventType::NavActivity);
  }
}

void NavigationPane::stepNavFocus(int delta) {
  _lv_obj_t* host = itemHost();
  if (!host || delta == 0) return;
  const uint32_t child_cnt = lv_obj_get_child_cnt(host);
  if (child_cnt == 0) return;
  int idx = -1;
  uint8_t nav_count = 0;
  for (uint32_t i = 0; i < child_cnt; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    if (_ring_layout_focus == kNoEmphasis) {
      idx = static_cast<int>(nav_count);
      break;
    }
    const uint8_t cid = navCellId(cell);
    if (cid == _ring_layout_focus) {
      idx = static_cast<int>(nav_count);
      break;
    }
    ++nav_count;
  }
  if (idx < 0) idx = 0;
  const uint8_t n = btnCount();
  if (n == 0) return;
  int next = idx + delta;
  while (next < 0) next += static_cast<int>(n);
  next %= static_cast<int>(n);
  nav_count = 0;
  for (uint32_t i = 0; i < child_cnt; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    if (nav_count == static_cast<uint8_t>(next)) {
      _ring_layout_focus = navCellId(cell);
      layoutNav(true);
      sendTilePreview(_ring_layout_focus);
      return;
    }
    ++nav_count;
  }
}

bool NavigationPane::commitFocused() {
  if (!_tileview || _ring_layout_focus == kNoEmphasis ||
      _ring_layout_focus >= kMaxButtons) {
    return false;
  }
  ui_event_send(_tileview, UiEventType::TileCommit, &_ring_layout_focus);
  return true;
}

bool NavigationPane::onKey(uint32_t lv_key) {
  if (!panelVisible()) return false;
  if (_ring_fade_busy) {
    if (lv_key == LV_KEY_ESC || lv_key == LV_KEY_ENTER || lv_key == LV_KEY_PREV ||
        lv_key == LV_KEY_NEXT || lv_key == LV_KEY_LEFT || lv_key == LV_KEY_RIGHT ||
        lv_key == LV_KEY_UP || lv_key == LV_KEY_DOWN) {
      return true;
    }
    return false;
  }
  if (lv_key == LV_KEY_ESC) {
    if (lv_obj_t* frame = frameRoot()) {
      ui_event_send(frame, UiEventType::NavClose);
    }
    return true;
  }
  if (lv_key == LV_KEY_ENTER) {
    return commitFocused();
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

bool NavigationPane::requestCloseAnimation() {
  if (!_root || !_nav || !panelVisible()) return false;
  if (_close_animating) return true;

  lv_coord_t nx = 0;
  lv_coord_t ny = 0;
  lv_coord_t nw = lv_obj_get_width(_nav);
  lv_coord_t nh = lv_obj_get_height(_nav);
  gridPaneFrameRect(_root, _tileview, &nx, &ny, &nw, &nh);
  const lv_coord_t closed_x = closedPaneX(_nav, nx);
  const lv_coord_t current_x = lv_obj_get_x(_nav);
  const uint16_t close_slide_ms = gridNavCloseAnimMs(_root);
  if (current_x <= closed_x || close_slide_ms == 0) return false;

  ui_motion_cancel(_nav);
  ui_motion_cancel(this);
  _ring_fade_busy = true;
  _close_animating = true;
  setNavButtonsInteractive(false);

  UiMotionSpec motion;
  motion.target = _nav;
  motion.exec = [](void* var, int32_t value) {
    lv_obj_set_x(static_cast<_lv_obj_t*>(var), static_cast<lv_coord_t>(value));
  };
  motion.ready = [](void* user_data) {
    auto* self = static_cast<NavigationPane*>(user_data);
    if (!self) return;
    self->_close_animating = false;
    self->_ring_fade_busy = false;
    if (_lv_obj_t* frame = self->frameRoot()) {
      ui_event_send(frame, UiEventType::NavClose);
    }
  };
  motion.ready_data = this;
  motion.start_value = current_x;
  motion.end_value = closed_x;
  motion.duration_ms = close_slide_ms;
  motion.path = UiMotionPath::EaseIn;
  if (!ui_motion_start(motion)) {
    _ring_fade_busy = false;
    _close_animating = false;
    return false;
  }
  return true;
}

void NavigationPane::openPanel() {
  if (!_root || !_nav) return;
  const uint16_t open_slide_ms = gridNavOpenAnimMs(_root);

  ui_motion_cancel(_nav);
  ui_motion_cancel(this);

  _ring_fade_busy = true;
  setNavButtonsInteractive(false);
  if (_ring_layout_focus == kNoEmphasis && btnCount() > 0) {
    if (_lv_obj_t* host = itemHost()) {
      const uint32_t n = lv_obj_get_child_cnt(host);
      for (uint32_t i = 0; i < n; ++i) {
        _lv_obj_t* cell = lv_obj_get_child(host, i);
        if (!cell) continue;
        _ring_layout_focus = navCellId(cell);
        break;
      }
    }
  }
  updateGeometry();
  lv_obj_clear_state(_nav, LV_STATE_USER_4);
  lv_coord_t nx = 0;
  lv_coord_t ny = 0;
  lv_coord_t nw = lv_obj_get_width(_nav);
  lv_coord_t nh = lv_obj_get_height(_nav);
  gridPaneFrameRect(_root, _tileview, &nx, &ny, &nw, &nh, false);
  const lv_coord_t closed_x = closedPaneX(_nav, nx);
  lv_obj_set_x(_nav, closed_x);
  lv_obj_clear_flag(_nav, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(_nav);
  layoutNav(false);
  UiMotionSpec motion;
  motion.target = _nav;
  motion.exec = [](void* var, int32_t v) {
    lv_obj_set_x(static_cast<lv_obj_t*>(var), static_cast<lv_coord_t>(v));
  };
  motion.ready = [](void* user_data) {
    auto* self = static_cast<NavigationPane*>(user_data);
    if (!self) return;
    self->_ring_fade_busy = false;
    self->updateGeometry();
    self->setNavButtonsInteractive(true);
    self->layoutNav(false);
    if (_lv_group_t* g = self->group()) {
      lv_group_focus_obj(self->_nav);
    }
  };
  motion.ready_data = this;
  motion.start_value = closed_x;
  motion.end_value = nx;
  motion.duration_ms = open_slide_ms;
  motion.path = UiMotionPath::EaseOut;
  if (!ui_motion_start(motion)) {
    lv_obj_set_x(_nav, nx);
    _ring_fade_busy = false;
    updateGeometry();
    setNavButtonsInteractive(true);
    layoutNav(false);
    if (_lv_group_t* g = group()) {
      lv_group_focus_obj(_nav);
    }
  }
}



}  // namespace heltec::meshcore::ui

#endif  // UI_NAVIGATION_GRID
