#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
#include "radial_navigator.hpp"

#include "heltec/ui/images.h"
#include "ui/app/behavior_profile.hpp"
#include "ui/theme/ui_theme.hpp"
#include "ui/core/ht_event.h"
#include "ui/core/scheduler.hpp"
#include "ui/widgets/widget_classes.hpp"
#include "ui/navigation/navigation_widget.hpp"
#include "ui/theme/widget_theme.hpp"

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace heltec::meshcore::ui {
namespace {
static uint8_t navButtonId(_lv_obj_t *btn) {
    return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn)));
}

static bool isNavStepKey(uint32_t key) {
    return key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP || key == LV_KEY_NEXT ||
           key == LV_KEY_RIGHT || key == LV_KEY_DOWN;
}

static int navStepDelta(uint32_t key) {
    return (key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP) ? -1 : 1;
}

static void layoutRootToTileView(_lv_obj_t *root, _lv_obj_t *tileview) {
    if (!root) return;
    _lv_obj_t *parent = lv_obj_get_parent(root);
    if (!parent) return;

    lv_obj_update_layout(parent);
    if (tileview) {
        lv_obj_update_layout(tileview);
        lv_area_t target;
        lv_area_t parent_content;
        lv_obj_get_coords(tileview, &target);
        lv_obj_get_content_coords(parent, &parent_content);
        lv_obj_set_size(root, lv_area_get_width(&target), lv_area_get_height(&target));
        lv_obj_set_pos(root, target.x1 - parent_content.x1, target.y1 - parent_content.y1);
        return;
    }

    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(root, 0, 0);
}

static void layoutSquareInParent(_lv_obj_t *obj) {
    if (!obj) return;
    _lv_obj_t *parent = lv_obj_get_parent(obj);
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

} // namespace

RadialNavigator::~RadialNavigator() {
    clearAnimations();
}

void RadialNavigator::configure(const UiNavigationItem *items, uint8_t count) {
    if (!items) return;
    if (count > kMaxButtons) count = kMaxButtons;
    _configuring = true;
    for (uint8_t i = 0; i < count; ++i) {
        const UiNavigationItem &item = items[i];
        if (item.screen_index >= kMaxButtons || !item.icon) continue;
        setIcon(item.screen_index, item.icon);
    }
    _configuring = false;
    invalidateSlotCache();
    layoutRing(false);
}

_lv_obj_t *RadialNavigator::itemHost() const {
    return _nav;
}

_lv_obj_t *RadialNavigator::findCellById(uint8_t id) const {
    _lv_obj_t *host = itemHost();
    if (!host) return nullptr;
    const uint32_t n = lv_obj_get_child_cnt(host);
    for (uint32_t i = 0; i < n; ++i) {
        _lv_obj_t *cell = lv_obj_get_child(host, i);
        if (cell && navButtonId(cell) == id) {
            return cell;
        }
    }
    return nullptr;
}

uint8_t RadialNavigator::btnCount() const {
    _lv_obj_t *host = itemHost();
    return host ? static_cast<uint8_t>(lv_obj_get_child_cnt(host)) : 0;
}

uint8_t RadialNavigator::slotForId(uint8_t id) const {
    _lv_obj_t *host = itemHost();
    if (!host) return 0;
    const uint32_t n = lv_obj_get_child_cnt(host);
    for (uint32_t i = 0; i < n; ++i) {
        _lv_obj_t *btn = lv_obj_get_child(host, i);
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

void RadialNavigator::clearAnimations() {
    ui_motion_cancel(this);
}

void RadialNavigator::invalidateSlotCache() {
    _slot_cache_count = 0;
}

void RadialNavigator::rebuildSlotCache(uint8_t count) {
    if (count == 0 || count > kMaxButtons || _slot_cache_count == count) return;
    for (uint8_t i = 0; i < count; ++i) {
        // Screen Y grows downwards. Negative slot angles arrange logical NEXT
        // counter-clockwise, allowing a clockwise input to rotate the ring clockwise.
        const float angle = -2.f * static_cast<float>(M_PI) * i / count;
        _slot_cos[i] = cosf(angle);
        _slot_sin[i] = sinf(angle);
    }
    _slot_cache_count = count;
}

void RadialNavigator::layoutRing(bool animate, bool snap_theta, bool update_emphasis,
                                 bool update_selection) {
    const uint8_t n = btnCount();
    if (n == 0 || !_nav) return;
    rebuildSlotCache(n);

    const UiNavigationBehavior &behavior = ui_behavior_profile().navigation;
    const uint8_t focus = focusedIndex();
    const uint8_t focus_slot = focusedSlot();
    const float target_abs =
        -static_cast<float>(M_PI / 2.0) + (2.f * static_cast<float>(M_PI) * focus_slot / n);
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

    const uint16_t ms = animate ? behavior.ring_anim_ms : 0;
    if (animate && ms != 0 && snap_theta && fabsf(target - _ring_theta) >= 0.001f) {
        _emphasis_index = kNoEmphasis;
        layoutRing(false, false, false);
        clearAnimations();
        UiMotionSpec motion;
        motion.target = this;
        motion.exec = [](void *var, int32_t v) {
            auto *self = static_cast<RadialNavigator *>(var);
            self->_ring_theta = v / 1000.f;
            self->layoutRing(false, false, false, false);
        };
        motion.ready = [](void *user_data) {
            auto *self = static_cast<RadialNavigator *>(user_data);
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

    lv_area_t nav_coords;
    lv_area_t content_coords;
    lv_obj_get_coords(_nav, &nav_coords);
    lv_obj_get_content_coords(_nav, &content_coords);
    const lv_coord_t w = lv_area_get_width(&content_coords);
    const lv_coord_t h = lv_area_get_height(&content_coords);
    if (w < 8 || h < 8) return;

    const float cx = static_cast<float>(content_coords.x1 - nav_coords.x1) +
                     (static_cast<float>(w) - 1.f) * 0.5f;
    const float cy = static_cast<float>(content_coords.y1 - nav_coords.y1) +
                     (static_cast<float>(h) - 1.f) * 0.5f;
    _lv_obj_t *const host = itemHost();
    if (!host) return;

    if (_max_icon_side <= 0) return;

    float R = (static_cast<float>(LV_MIN(w, h)) - static_cast<float>(_max_icon_side)) * 0.5f;
    if (R < 0.f) R = 0.f;
    const float theta_cos = cosf(_ring_theta);
    const float theta_sin = sinf(_ring_theta);

    for (uint8_t i = 0; i < n; ++i) {
        _lv_obj_t *const btn = lv_obj_get_child(host, i);
        if (!btn) continue;
        const uint8_t id = navButtonId(btn);
        const lv_coord_t icon_w = _icon_width[id];
        const lv_coord_t icon_h = _icon_height[id];
        if (icon_w <= 0 || icon_h <= 0) continue;
        const bool emphasized = (_emphasis_index != kNoEmphasis && id == _emphasis_index);
        const float x_unit = _slot_cos[i] * theta_cos - _slot_sin[i] * theta_sin;
        const float y_unit = _slot_sin[i] * theta_cos + _slot_cos[i] * theta_sin;
        const float icon_cx = cx + R * x_unit;
        const float icon_cy = cy + R * y_unit;
        const float icon_half_w = (static_cast<float>(icon_w) - 1.f) * 0.5f;
        const float icon_half_h = (static_cast<float>(icon_h) - 1.f) * 0.5f;
        const lv_coord_t icon_x = static_cast<lv_coord_t>(roundf(icon_cx - icon_half_w));
        const lv_coord_t icon_y = static_cast<lv_coord_t>(roundf(icon_cy - icon_half_h));

        lv_obj_set_pos(btn, icon_x, icon_y);
        if (update_selection && emphasized) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else if (update_selection) {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    }
}

void RadialNavigator::updateGeometry() {
    if (!_root || !_nav || _updating_geometry) return;
    _updating_geometry = true;

    layoutRootToTileView(_root, _tileview);

    lv_obj_update_layout(_root);
    const lv_coord_t pw = lv_obj_get_width(_root);
    const lv_coord_t ph = lv_obj_get_height(_root);
    if (pw >= 8 && ph >= 8) {
        layoutSquareInParent(_nav);
        const lv_coord_t nav_x = lv_obj_get_x(_nav);
        const lv_coord_t nav_y = lv_obj_get_y(_nav);
        const lv_coord_t nav_side = lv_obj_get_width(_nav);
        const bool visible = panelVisible();
        const bool geometry_changed = !_geometry_valid || _cached_panel_visible != visible ||
                                      _cached_nav_x != nav_x || _cached_nav_y != nav_y ||
                                      _cached_nav_side != nav_side;
        if (!panelVisible()) {
            const uint8_t n = btnCount();
            if (n != 0) {
                const uint8_t focus_slot = focusedSlot();
                _ring_theta = -static_cast<float>(M_PI / 2.0) +
                              (2.f * static_cast<float>(M_PI) * focus_slot / n);
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

RadialNavigator::RadialNavigator(biz::IBizFacade &biz) : UiSurface(biz, ht_navigation_root_class) {
}

bool RadialNavigator::buildSurface() {
    layoutRootToTileView(_root, _tileview);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_group_set_wrap(group(), false);

    _nav = ht_class_create(&ht_navigation_ring_class, _root);
    if (!_nav) return false;
    lv_obj_clear_flag(_nav, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(_nav, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE | LV_OBJ_FLAG_HIDDEN);
    lv_group_add_obj(focusGroup(), _nav);
    lv_obj_add_event_cb(
        _nav,
        [](lv_event_t *e) {
            if (LV_EVENT_KEY != lv_event_get_code(e)) return;
            auto *self = static_cast<RadialNavigator *>(lv_event_get_user_data(e));
            if (!self || !self->onKey(lv_event_get_key(e))) return;
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
        },
        LV_EVENT_KEY, this);
    lv_obj_add_event_cb(
        _nav,
        [](lv_event_t *e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
            auto *self = static_cast<RadialNavigator *>(lv_event_get_user_data(e));
            if (!self || !self->panelVisible() || !self->_tileview) return;
            if (self->_ring_layout_focus >= kMaxButtons) return;
            ht_event_send(self->_tileview, EventType::TileCommit, &self->_ring_layout_focus);
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
        },
        LV_EVENT_CLICKED, this);

    lv_obj_add_event_cb(
        _root,
        [](lv_event_t *e) {
            auto *self = static_cast<RadialNavigator *>(lv_event_get_user_data(e));
            if (!self || !self->_nav || lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
            self->updateGeometry();
        },
        LV_EVENT_SIZE_CHANGED, this);
    lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
    updateGeometry();
    lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
    return true;
}

void RadialNavigator::setTileView(_lv_obj_t *tileview) {
    _tileview = tileview;
    _geometry_valid = false;
    updateGeometry();
}

void RadialNavigator::setIcon(uint8_t id, const lv_img_dsc_t *img) {
    if (id >= kMaxButtons || !img || !_nav || !group()) return;
    lv_img_header_t icon_header{};
    if (LV_RES_OK != lv_img_decoder_get_info(img, &icon_header)) return;
    if (icon_header.w == 0 || icon_header.h == 0 || icon_header.w > UINT8_MAX ||
        icon_header.h > UINT8_MAX)
        return;
    _lv_obj_t *host = itemHost();
    if (!host) return;

    _lv_obj_t *icon = findCellById(id);
    if (!icon) {
        icon = ht_class_create(&ht_radial_navigation_icon_class, host);
        if (!icon) return;
        lv_obj_set_user_data(icon, reinterpret_cast<void *>(static_cast<uintptr_t>(id)));
        lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        invalidateSlotCache();
    }

    lv_img_set_src(icon, img);
    lv_obj_set_size(icon, static_cast<lv_coord_t>(icon_header.w),
                    static_cast<lv_coord_t>(icon_header.h));

    _icon_width[id] = static_cast<uint8_t>(icon_header.w);
    _icon_height[id] = static_cast<uint8_t>(icon_header.h);

    _max_icon_side = 0;
    for (uint8_t i = 0; i < kMaxButtons; ++i) {
        _max_icon_side = LV_MAX(_max_icon_side,
                                static_cast<lv_coord_t>(LV_MAX(_icon_width[i], _icon_height[i])));
    }

    _geometry_valid = false;
    if (!_configuring) layoutRing(false);
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
    layoutRing(false);
}

void RadialNavigator::ensureDefaultFocus() {
    if (_ring_layout_focus != kNoEmphasis || btnCount() == 0) return;
    _lv_obj_t *host = itemHost();
    if (!host) return;
    _lv_obj_t *first = lv_obj_get_child(host, 0);
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

void RadialNavigator::sendTilePreview(uint8_t tile_idx) {
    if (_tileview && tile_idx < kMaxButtons) {
        ht_event_send(_tileview, EventType::TilePreview, &tile_idx);
    }
}

_lv_obj_t *RadialNavigator::frameRoot() const {
    if (_frame_root) return _frame_root;
    return _root;
}

void RadialNavigator::stepNavFocus(int delta) {
    if (!itemHost() || delta == 0) return;
    const uint8_t n = btnCount();
    if (n == 0) return;
    int next = (static_cast<int>(_ring_focus_slot) + delta) % static_cast<int>(n);
    if (next < 0) next += static_cast<int>(n);
    _lv_obj_t *btn = lv_obj_get_child(itemHost(), static_cast<uint32_t>(next));
    if (!btn) return;
    _ring_focus_slot = static_cast<uint8_t>(next);
    _ring_layout_focus = navButtonId(btn);
    layoutRing(true);
    sendTilePreview(_ring_layout_focus);
}

bool RadialNavigator::onKey(uint32_t lv_key) {
    if (!panelVisible()) return false;
    if (lv_key == LV_KEY_ESC) {
        if (_lv_obj_t *frame = frameRoot()) {
            ht_event_send(frame, EventType::NavClose);
        }
        return true;
    }
    if (isNavStepKey(lv_key)) {
        if (_lv_obj_t *frame = frameRoot()) {
            ht_event_send(frame, EventType::NavActivity);
        }
        stepNavFocus(navStepDelta(lv_key));
        return true;
    }
    return false;
}

void RadialNavigator::openPanel() {
    if (!_root || !_nav) return;
    clearAnimations();
    ensureDefaultFocus();
    updateGeometry();
    lv_obj_clear_flag(_nav, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(_nav);
    layoutRing(false);
    if (_lv_group_t *g = group()) {
        lv_group_focus_obj(_nav);
    }
}

} // namespace heltec::meshcore::ui

#endif // !UI_NAVIGATION_GRID
