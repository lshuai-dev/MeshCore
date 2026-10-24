#if HELTEC_UI_HAS_TOUCH
#include "NavigationPane.hpp"

#include "MeshCore.h"
#if HELTEC_UI_HAS_TOUCH
#include "heltec/drivers/input/TouchPort.hpp"
#endif
#include "heltec/ui/Images.h"
#include "ui/app/BehaviorProfile.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include "ui/core/UiMessage.hpp"
#include "ui/core/Scheduler.hpp"
#include "ui/navigation/NavigationWidget.hpp"
#include "ui/theme/WidgetTheme.hpp"

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef HELTEC_TOUCH_GESTURE_SWIPE_PX
#define HELTEC_TOUCH_GESTURE_SWIPE_PX 24
#endif

namespace Heltec::MeshCore::Ui
{

namespace
{

static void layoutGridTitleBar(_lv_obj_t *bar)
{
    if (!bar)
        return;
    lv_obj_set_flex_grow(bar, 0);
    _lv_obj_t *lbl = lv_obj_get_child(bar, 0);
    if (!lbl)
        return;
    lv_obj_set_width(lbl, lv_pct(100));
}

static constexpr lv_coord_t kGridIconTitleGapPx = 3;
static constexpr lv_coord_t kGridTitleBottomMarginPx = 4;
static constexpr uint8_t kGridColumns = 2;
static constexpr uint8_t kGridRows = 3;
static constexpr lv_coord_t kGridGapPx = 6;
static constexpr lv_coord_t kGridFooterHeightPx = 36;

static uint16_t gridNavOpenAnimMs(const lv_obj_t *obj)
{
    (void)obj;
    return ui_behavior_profile().navigation.open_anim_ms;
}

static uint16_t gridNavCloseAnimMs(const lv_obj_t *obj)
{
    return static_cast<uint16_t>((static_cast<uint32_t>(gridNavOpenAnimMs(obj)) * 3U) / 4U);
}

static _lv_obj_t *gridCellIconArea(_lv_obj_t *cell)
{
    return cell ? lv_obj_get_child(cell, 0) : nullptr;
}

static _lv_obj_t *gridCellIcon(_lv_obj_t *cell)
{
    _lv_obj_t *area = gridCellIconArea(cell);
    return area ? lv_obj_get_child(area, 0) : nullptr;
}

static _lv_obj_t *gridCellTitleBar(_lv_obj_t *cell)
{
    return cell ? lv_obj_get_child(cell, 1) : nullptr;
}

static void layoutGridCellIcon(_lv_obj_t *cell, lv_coord_t cell_w, lv_coord_t cell_h)
{
    _lv_obj_t *icon_area = gridCellIconArea(cell);
    _lv_obj_t *icon = gridCellIcon(cell);
    _lv_obj_t *bar = gridCellTitleBar(cell);
    if (!icon_area || !icon || !bar)
        return;
    lv_obj_set_layout(cell, 0);
    lv_obj_set_width(bar, cell_w);
    layoutGridTitleBar(bar);
    lv_obj_update_layout(bar);
    const lv_coord_t bar_h = lv_obj_get_height(bar);
    const lv_coord_t area_w = cell_w;
    const lv_coord_t bar_y = LV_MAX(0, cell_h - bar_h - kGridTitleBottomMarginPx);
    const lv_coord_t max_area_h = LV_MAX(2, bar_y - kGridIconTitleGapPx);
    lv_obj_set_layout(icon_area, 0);
    lv_obj_set_height(bar, bar_h);
    lv_obj_set_pos(bar, 0, bar_y);
    lv_obj_set_size(icon_area, area_w, max_area_h);
    lv_obj_set_pos(icon_area, 0, 0);
    lv_obj_center(icon);
}

static bool isNavStepKey(uint32_t key)
{
    return key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP || key == LV_KEY_NEXT ||
           key == LV_KEY_RIGHT || key == LV_KEY_DOWN;
}

static int navStepDelta(uint32_t key)
{
    return (key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP) ? -1 : 1;
}

#if HELTEC_UI_HAS_TOUCH
static bool navTouchMovedBeyondThreshold(const lv_point_t &origin, const lv_point_t &current)
{
    const int32_t dx = static_cast<int32_t>(current.x) - origin.x;
    const int32_t dy = static_cast<int32_t>(current.y) - origin.y;
    const int32_t abs_dx = dx < 0 ? -dx : dx;
    const int32_t abs_dy = dy < 0 ? -dy : dy;
    return abs_dx >= HELTEC_TOUCH_GESTURE_SWIPE_PX || abs_dy >= HELTEC_TOUCH_GESTURE_SWIPE_PX;
}
#endif

static uint8_t navCellId(_lv_obj_t *cell)
{
    return static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(cell)));
}

static void layoutNavigationGrid(_lv_obj_t *host, _lv_obj_t *footer_cell)
{
    const lv_coord_t width = lv_obj_get_width(host);
    const lv_coord_t height = lv_obj_get_height(host);
    const lv_coord_t footer_height = footer_cell ? kGridFooterHeightPx : 0;
    const lv_coord_t footer_gap = footer_cell ? kGridGapPx : 0;
    const lv_coord_t grid_height_limit = height - footer_height - footer_gap;
    const lv_coord_t cell_width =
        LV_MAX(2, (width - kGridGapPx * (kGridColumns - 1)) / kGridColumns);
    const lv_coord_t cell_height =
        LV_MAX(2, (grid_height_limit - kGridGapPx * (kGridRows - 1)) / kGridRows);
    const lv_coord_t grid_width = kGridColumns * cell_width + kGridGapPx;
    const lv_coord_t grid_height = kGridRows * cell_height + kGridGapPx * (kGridRows - 1);
    const lv_coord_t origin_x = (width - grid_width) / 2;
    const lv_coord_t origin_y = (height - grid_height - footer_height - footer_gap) / 2;

    uint8_t grid_index = 0;
    const uint32_t count = lv_obj_get_child_cnt(host);
    for (uint32_t i = 0; i < count; ++i)
    {
        _lv_obj_t *cell = lv_obj_get_child(host, i);
        if (!cell)
            continue;
        lv_obj_set_layout(cell, 0);
        if (cell == footer_cell)
        {
            lv_obj_set_size(cell, width, footer_height);
            lv_obj_set_pos(cell, 0, origin_y + grid_height + footer_gap);
            continue;
        }
        const uint8_t column = grid_index % kGridColumns;
        const uint8_t row = grid_index / kGridColumns;
        ++grid_index;
        lv_obj_set_size(cell, cell_width, cell_height);
        lv_obj_set_pos(cell, origin_x + column * (cell_width + kGridGapPx),
                       origin_y + row * (cell_height + kGridGapPx));
    }
}

static void layoutRootToTileView(_lv_obj_t *root, _lv_obj_t *tileview)
{
    if (!root)
        return;
    _lv_obj_t *parent = lv_obj_get_parent(root);
    if (!parent)
        return;

    lv_obj_update_layout(parent);
    if (tileview && lv_obj_is_valid(tileview))
    {
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

} // namespace

NavigationPane::~NavigationPane()
{
    if (_nav)
        ui_motion_cancel(_nav);
    ui_motion_cancel(this);
}

void NavigationPane::configure(const UiNavigationItem *items, uint8_t count)
{
    if (!items)
        return;
    if (count > kMaxButtons)
        count = kMaxButtons;
    for (uint8_t i = 0; i < count; ++i)
    {
        const UiNavigationItem &item = items[i];
        if (item.screen_index >= kMaxButtons)
            continue;
        if (item.icon)
            setIcon(item.screen_index, item.icon);
        if (item.label)
            setLabel(item.screen_index, item.label);
        if (item.footer)
            setFooterSlot(item.screen_index);
    }
}

_lv_obj_t *NavigationPane::navButtonHost() const
{
    return itemHost();
}

_lv_obj_t *NavigationPane::itemHost() const
{
    return ui_gridNav_content(_nav);
}

_lv_obj_t *NavigationPane::findCellById(uint8_t id) const
{
    _lv_obj_t *host = itemHost();
    if (!host)
        return nullptr;
    const uint32_t n = lv_obj_get_child_cnt(host);
    for (uint32_t i = 0; i < n; ++i)
    {
        _lv_obj_t *cell = lv_obj_get_child(host, i);
        if (!cell)
            continue;
        if (navCellId(cell) == id)
        {
            return cell;
        }
    }
    return nullptr;
}

uint8_t NavigationPane::btnCount() const
{
    _lv_obj_t *host = itemHost();
    if (!host)
        return 0;
    uint8_t count = 0;
    const uint32_t n = lv_obj_get_child_cnt(host);
    for (uint32_t i = 0; i < n; ++i)
    {
        _lv_obj_t *child = lv_obj_get_child(host, i);
        if (!child)
            continue;
        ++count;
    }
    return count;
}

void NavigationPane::layoutNav(bool animate, bool snap_theta, bool update_emphasis)
{
    (void)animate;
    (void)snap_theta;
    if (_layout_busy)
        return;
    _layout_busy = true;
    layoutGrid(update_emphasis);
    _layout_busy = false;
}

void NavigationPane::setLabel(uint8_t id, const char *label)
{
    if (id >= kMaxButtons)
        return;
    _labels[id] = label;
    _lv_obj_t *cell = findCellById(id);
    if (!cell)
        return;
    _lv_obj_t *bar = gridCellTitleBar(cell);
    if (!bar)
        return;
    _lv_obj_t *lbl = lv_obj_get_child(bar, 0);
    if (lbl)
        lv_label_set_text_static(lbl, label ? label : "");
}

void NavigationPane::setFooterSlot(uint8_t id)
{
    if (id >= kMaxButtons)
        return;
    _footer_id = id;
    _geometry_valid = false;
    if (_lv_obj_t *cell = findCellById(id))
    {
        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        if (_lv_obj_t *icon_area = gridCellIconArea(cell))
        {
            lv_obj_add_flag(icon_area, LV_OBJ_FLAG_HIDDEN);
        }
        if (_lv_obj_t *bar = gridCellTitleBar(cell))
        {
            lv_obj_add_state(bar, LV_STATE_USER_1);
            if (_lv_obj_t *label = lv_obj_get_child(bar, 0))
                lv_obj_center(label);
        }
    }
    if (panelVisible())
        layoutNav(false);
}

void NavigationPane::layoutGrid(bool update_emphasis)
{
    _lv_obj_t *host = itemHost();
    const uint8_t n = btnCount();
    if (n == 0 || !host)
        return;

    const lv_coord_t pw = lv_obj_get_width(host);
    const lv_coord_t ph = lv_obj_get_height(host);
    if (pw < 8 || ph < 8)
        return;

    const bool has_footer = (_footer_id != static_cast<uint8_t>(ScreenId::None));
    _lv_obj_t *footer_cell = nullptr;
    if (has_footer)
        footer_cell = findCellById(_footer_id);
    layoutNavigationGrid(host, footer_cell);
    lv_obj_update_layout(host);

    const uint8_t focus = focusedIndex();
    if (update_emphasis)
        _emphasis_index = focus;

    for (uint8_t i = 0; i < n; ++i)
    {
        _lv_obj_t *cell = lv_obj_get_child(host, i);
        if (!cell)
            continue;
        const uint8_t id = navCellId(cell);

        if (!panelVisible())
        {
            lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        }
        else
        {
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_HIDDEN);
            if (!_ring_fade_busy)
            {
                lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            }
            else
            {
                lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            }
        }

        if (has_footer && id == _footer_id)
        {
            lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            if (id == focus)
            {
                lv_obj_add_state(cell, LV_STATE_FOCUS_KEY);
            }
            else
            {
                lv_obj_clear_state(cell, LV_STATE_FOCUS_KEY);
            }
            continue;
        }

        lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
        layoutGridCellIcon(cell, lv_obj_get_width(cell), lv_obj_get_height(cell));

        if (id == focus)
        {
            lv_obj_add_state(cell, LV_STATE_FOCUS_KEY);
        }
        else
        {
            lv_obj_clear_state(cell, LV_STATE_FOCUS_KEY);
        }
    }

    lv_obj_invalidate(host);
}

uint8_t NavigationPane::focusedIndex() const
{
    if (_ring_layout_focus != kNoEmphasis)
        return _ring_layout_focus;
    return 0;
}

bool NavigationPane::panelVisible() const
{
    return _nav && !lv_obj_has_flag(_nav, LV_OBJ_FLAG_HIDDEN);
}

void NavigationPane::setNavButtonsInteractive(bool interactive)
{
    _lv_obj_t *host = itemHost();
    if (!host)
        return;
    const uint8_t n = btnCount();
    for (uint8_t i = 0; i < n; ++i)
    {
        _lv_obj_t *btn = lv_obj_get_child(host, i);
        if (!btn)
            continue;
        if (interactive)
        {
            lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        }
        else
        {
            lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

static void gridPaneFrameRect(_lv_obj_t *root, _lv_obj_t *tileview, lv_coord_t *out_x,
                              lv_coord_t *out_y, lv_coord_t *out_w, lv_coord_t *out_h,
                              bool update_layout = true)
{
    const lv_coord_t pw = root ? lv_obj_get_width(root) : 0;
    const lv_coord_t ph = root ? lv_obj_get_height(root) : 0;
    *out_x = 0;
    *out_y = 0;
    *out_w = pw;
    *out_h = ph;

    if (!root || !tileview || !lv_obj_is_valid(tileview))
        return;

    if (update_layout)
        lv_obj_update_layout(tileview);
    lv_area_t root_coords;
    lv_area_t tile_coords;
    lv_obj_get_coords(root, &root_coords);
    lv_obj_get_coords(tileview, &tile_coords);

    const lv_coord_t tw = lv_area_get_width(&tile_coords);
    const lv_coord_t th = lv_area_get_height(&tile_coords);
    if (tw < 8 || th < 8)
        return;

    *out_x = tile_coords.x1 - root_coords.x1;
    *out_y = tile_coords.y1 - root_coords.y1;
    *out_w = tw;
    *out_h = th;
}

static lv_coord_t closedPaneX(_lv_obj_t *pane, lv_coord_t open_x)
{
    if (!pane)
        return 0;
    const lv_coord_t w = lv_obj_get_width(pane);
    return w > 0 ? open_x - w : open_x;
}

void NavigationPane::updateGeometry()
{
    if (!_root || !_nav || _updating_geometry)
        return;

    _updating_geometry = true;
    layoutRootToTileView(_root, _tileview);
    if (_frame_root && lv_obj_is_valid(_frame_root))
    {
        lv_obj_update_layout(_frame_root);
    }
    else if (_tileview && lv_obj_is_valid(_tileview))
    {
        lv_obj_update_layout(_tileview);
    }
    // _root receives its size and position directly above.  Its child panel is
    // sized and positioned below as well, so a second layout pass here only
    // re-enters LVGL layout handlers without changing the geometry we read.
    const lv_coord_t pw = lv_obj_get_width(_root);
    const lv_coord_t ph = lv_obj_get_height(_root);
    if (pw < 8 || ph < 8)
    {
        _updating_geometry = false;
        return;
    }
    if (pw >= 8 && ph >= 8)
    {
        lv_coord_t nx = 0;
        lv_coord_t ny = 0;
        lv_coord_t nw = pw;
        lv_coord_t nh = ph;
        gridPaneFrameRect(_root, _tileview, &nx, &ny, &nw, &nh, false);
        const bool visible = panelVisible();
        // Use the newly computed panel width for the hidden position.  Reading
        // the old object width here would leave a freshly created pane partially
        // visible until the next geometry pass.
        const lv_coord_t desired_x = visible ? nx : (nw > 0 ? nx - nw : nx);
        const bool geometry_changed = !_geometry_valid || _cached_panel_visible != visible ||
                                      _cached_nav_x != desired_x || _cached_nav_y != ny ||
                                      _cached_nav_w != nw || _cached_nav_h != nh;
        if (geometry_changed)
        {
            lv_obj_set_size(_nav, nw, nh);
            lv_obj_set_pos(_nav, desired_x, ny);
            _cached_panel_visible = visible;
            _cached_nav_x = desired_x;
            _cached_nav_y = ny;
            _cached_nav_w = nw;
            _cached_nav_h = nh;
        }
        if (!panelVisible())
        {
            _emphasis_index = focusedIndex();
        }
        if (geometry_changed)
            layoutNav(false, false);
        _geometry_valid = true;
    }
    _updating_geometry = false;
}

void NavigationPane::setTileView(_lv_obj_t *tileview)
{
    _tileview = tileview;
    _geometry_valid = false;
    updateGeometry();
}

NavigationPane::NavigationPane(Biz::IBizFacade &biz)
    : UiSurface(biz)
{
}

bool NavigationPane::build()
{
    layoutRootToTileView(_root, _tileview);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    _gridNav.setWrap(false);

    _nav = ui_gridNav_create_grid(_root);
    if (!_nav)
        return false;
    _gridNav.add(_nav);
    (void)_gridNav.enterControls();
    lv_obj_add_event_cb(
        _nav,
        [](lv_event_t *event)
        {
            if (lv_event_get_code(event) != LV_EVENT_KEY)
                return;
            auto *self = static_cast<NavigationPane *>(lv_event_get_user_data(event));
            if (!self || !self->onKey(lv_event_get_key(event)))
                return;
            lv_event_stop_processing(event);
            lv_event_stop_bubbling(event);
        },
        LV_EVENT_KEY, this);

    lv_obj_add_event_cb(
        _root,
        [](lv_event_t *e)
        {
            auto *self = static_cast<NavigationPane *>(lv_event_get_user_data(e));
            if (!self || !self->_nav || lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED)
                return;
            const bool fading = self->_ring_fade_busy;
            ui_motion_cancel(self->_nav);
            ui_motion_cancel(self);
            if (fading)
            {
                self->_ring_fade_busy = false;
                lv_obj_clear_state(self->_nav, LV_STATE_USER_4);
                lv_obj_clear_flag(self->_nav, LV_OBJ_FLAG_HIDDEN);
                self->setNavButtonsInteractive(true);
                self->layoutNav(false);
            }
            self->updateGeometry();
        },
        LV_EVENT_SIZE_CHANGED, this);
    lv_obj_add_state(_nav, LV_STATE_USER_4);
    lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
    updateGeometry();
    lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
    return true;
}

void NavigationPane::setIcon(uint8_t id, const lv_img_dsc_t *img)
{
    if (id >= kMaxButtons || !img)
        return;
    if (!_nav)
        return;

    _lv_obj_t *const host = itemHost();
    if (!host)
        return;

    _lv_obj_t *existing = findCellById(id);
    if (existing)
    {
        if (_lv_obj_t *icon = gridCellIcon(existing))
            lv_img_set_src(icon, img);
        _geometry_valid = false;
        if (panelVisible())
            layoutNav(false);
        return;
    }

    _lv_obj_t *cell = ht_class_create(
        &ht_navigation_cell_class, host,
        reinterpret_cast<void *>(static_cast<uintptr_t>(id)));
    if (!cell)
        return;
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    _lv_obj_t *icon_area = ht_class_create(&ht_navigation_icon_area_class, cell);
    if (!icon_area)
    {
        lv_obj_del(cell);
        return;
    }
    lv_obj_set_width(icon_area, lv_pct(100));
    lv_obj_clear_flag(icon_area, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    _lv_obj_t *icon = ht_class_create(&ht_navigation_grid_icon_class, icon_area);
    if (!icon)
    {
        lv_obj_del(cell);
        return;
    }
    lv_img_set_src(icon, img);
    lv_img_set_zoom(icon, LV_IMG_ZOOM_NONE);
    lv_img_set_size_mode(icon, LV_IMG_SIZE_MODE_REAL);
    lv_obj_set_size(icon, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_img_set_antialias(icon, false);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    _lv_obj_t *bar = ht_class_create(&ht_navigation_title_bar_class, cell);
    if (!bar)
    {
        lv_obj_del(cell);
        return;
    }
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    _lv_obj_t *lbl =
        ht_label_create_static(&ht_navigation_title_label_class, bar,
                               _labels[id] ? _labels[id] : "");
    if (!lbl)
    {
        lv_obj_del(cell);
        return;
    }

    lv_obj_set_width(lbl, lv_pct(100));

    layoutGridTitleBar(bar);

    if (panelVisible() && !_ring_fade_busy)
    {
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    }
    else
    {
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_event_cb(
        cell,
        [](lv_event_t *e)
        {
            auto *nav = static_cast<NavigationPane *>(lv_event_get_user_data(e));
            if (!nav)
                return;
#if HELTEC_UI_HAS_TOUCH
            nav->onCellTouchEvent(e);
#endif
            if (LV_EVENT_CLICKED != lv_event_get_code(e))
                return;
            if (!nav->panelVisible() || nav->_ring_fade_busy)
                return;
#if HELTEC_UI_HAS_TOUCH
            // Custom long-press or double-tap actions can hold LVGL at RELEASED while
            // the finger is still down. Do not turn that transition into a nav commit.
            if (Heltec::MeshCore::Dal::TouchPort::isPressed())
                return;
#endif
#if HELTEC_UI_HAS_TOUCH
            if (nav->_touch_dragged)
            {
                nav->_touch_active = false;
                nav->_touch_dragged = false;
                return;
            }
            nav->_touch_active = false;
#endif
            nav->onCellClicked(lv_event_get_target(e));
            (void)nav->commitFocused();
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
        },
#if HELTEC_UI_HAS_TOUCH
        LV_EVENT_ALL,
#else
        LV_EVENT_CLICKED,
#endif
        this);

    _geometry_valid = false;
    if (panelVisible())
        layoutNav(false);
}

#if HELTEC_UI_HAS_TOUCH
void NavigationPane::onCellTouchEvent(lv_event_t *e)
{
    if (!e)
        return;
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING && code != LV_EVENT_RELEASED &&
        code != LV_EVENT_PRESS_LOST)
    {
        return;
    }

    _lv_obj_t *cell = lv_event_get_target(e);
    if (!cell)
        return;

    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev)
        indev = lv_indev_get_act();
    if (!indev)
        return;

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    if (code == LV_EVENT_PRESSED)
    {
        _touch_active = true;
        _touch_dragged = false;
        _touch_origin = point;
        return;
    }

    if (code == LV_EVENT_PRESSING)
    {
        if (_touch_active && !_touch_dragged && navTouchMovedBeyondThreshold(_touch_origin, point))
        {
            _touch_dragged = true;
            lv_obj_clear_state(cell, LV_STATE_PRESSED);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        if (_touch_dragged)
            lv_obj_clear_state(cell, LV_STATE_PRESSED);
        if (code == LV_EVENT_PRESS_LOST)
            _touch_active = false;
    }
}
#endif

void NavigationPane::setSelectedIndex(uint8_t id, bool preview)
{
    if (id >= kMaxButtons || !_nav)
        return;
    if (!findCellById(id))
        return;
    _ring_layout_focus = id;
    if (!panelVisible())
        return;
    layoutNav(!preview);
    if (preview)
        sendTilePreview(id);
}

void NavigationPane::resetPanel()
{
    if (_nav)
        ui_motion_cancel(_nav);
    ui_motion_cancel(this);
    _ring_fade_busy = false;
    _close_animating = false;
    if (!_nav)
        return;
    lv_obj_add_state(_nav, LV_STATE_USER_4);
    updateGeometry();
    lv_obj_add_flag(_nav, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(_nav);
    setNavButtonsInteractive(false);
    layoutNav(false);
}

void NavigationPane::onEnter()
{
    _close_animating = false;
    if (!_nav || btnCount() == 0)
        return;
    openPanel();
}

void NavigationPane::onExit()
{
    resetPanel();
}

void NavigationPane::sendTilePreview(uint8_t tile_idx)
{
    if (!_tileview || tile_idx >= kMaxButtons)
        return;
    sendUiMessage(_tileview, UiMessageType::TilePreview, &tile_idx);
}

_lv_obj_t *NavigationPane::frameRoot() const
{
    if (_frame_root)
        return _frame_root;
    return _root;
}

void NavigationPane::onCellClicked(_lv_obj_t *cell)
{
    if (!cell)
        return;
    const uint8_t id = navCellId(cell);
    if (id == _ring_layout_focus)
        return;
    _ring_layout_focus = id;
    layoutNav(true);
    sendTilePreview(_ring_layout_focus);
    if (_lv_obj_t *frame = frameRoot())
    {
        sendUiMessage(frame, UiMessageType::NavActivity);
    }
}

void NavigationPane::stepNavFocus(int delta)
{
    _lv_obj_t *host = itemHost();
    if (!host || delta == 0)
        return;
    const uint32_t child_cnt = lv_obj_get_child_cnt(host);
    if (child_cnt == 0)
        return;
    int idx = -1;
    uint8_t nav_count = 0;
    for (uint32_t i = 0; i < child_cnt; ++i)
    {
        _lv_obj_t *cell = lv_obj_get_child(host, i);
        if (!cell)
            continue;
        if (_ring_layout_focus == kNoEmphasis)
        {
            idx = static_cast<int>(nav_count);
            break;
        }
        const uint8_t cid = navCellId(cell);
        if (cid == _ring_layout_focus)
        {
            idx = static_cast<int>(nav_count);
            break;
        }
        ++nav_count;
    }
    if (idx < 0)
        idx = 0;
    const uint8_t n = btnCount();
    if (n == 0)
        return;
    int next = idx + delta;
    while (next < 0)
        next += static_cast<int>(n);
    next %= static_cast<int>(n);
    nav_count = 0;
    for (uint32_t i = 0; i < child_cnt; ++i)
    {
        _lv_obj_t *cell = lv_obj_get_child(host, i);
        if (!cell)
            continue;
        if (nav_count == static_cast<uint8_t>(next))
        {
            _ring_layout_focus = navCellId(cell);
            layoutNav(true);
            sendTilePreview(_ring_layout_focus);
            return;
        }
        ++nav_count;
    }
}

bool NavigationPane::commitFocused()
{
    if (!_tileview || _ring_layout_focus == kNoEmphasis || _ring_layout_focus >= kMaxButtons)
    {
        return false;
    }
    sendUiMessage(_tileview, UiMessageType::TileCommit, &_ring_layout_focus);
    return true;
}

bool NavigationPane::onKey(uint32_t lv_key)
{
    if (!panelVisible())
        return false;
    if (_ring_fade_busy)
    {
        if (lv_key == LV_KEY_ESC || lv_key == LV_KEY_ENTER || lv_key == LV_KEY_PREV ||
            lv_key == LV_KEY_NEXT || lv_key == LV_KEY_LEFT || lv_key == LV_KEY_RIGHT ||
            lv_key == LV_KEY_UP || lv_key == LV_KEY_DOWN)
        {
            return true;
        }
        return false;
    }
    if (lv_key == LV_KEY_ESC)
    {
        if (lv_obj_t *frame = frameRoot())
        {
            sendUiMessage(frame, UiMessageType::NavClose);
        }
        return true;
    }
    if (lv_key == LV_KEY_ENTER)
    {
        return commitFocused();
    }
    if (isNavStepKey(lv_key))
    {
        if (_lv_obj_t *frame = frameRoot())
        {
            sendUiMessage(frame, UiMessageType::NavActivity);
        }
        stepNavFocus(navStepDelta(lv_key));
        return true;
    }
    return false;
}

bool NavigationPane::requestCloseAnimation()
{
    if (!_root || !_nav || !panelVisible())
        return false;
    if (_close_animating)
        return true;

    lv_coord_t nx = 0;
    lv_coord_t ny = 0;
    lv_coord_t nw = lv_obj_get_width(_nav);
    lv_coord_t nh = lv_obj_get_height(_nav);
    gridPaneFrameRect(_root, _tileview, &nx, &ny, &nw, &nh);
    const lv_coord_t closed_x = closedPaneX(_nav, nx);
    const lv_coord_t current_x = lv_obj_get_x(_nav);
    const uint16_t close_slide_ms = gridNavCloseAnimMs(_root);
    if (current_x <= closed_x || close_slide_ms == 0)
        return false;

    ui_motion_cancel(_nav);
    ui_motion_cancel(this);
    _ring_fade_busy = true;
    _close_animating = true;
    setNavButtonsInteractive(false);

    UiMotionSpec motion;
    motion.target = _nav;
    motion.exec = [](void *var, int32_t value)
    { lv_obj_set_x(static_cast<_lv_obj_t *>(var), static_cast<lv_coord_t>(value)); };
    motion.ready = [](void *user_data)
    {
        auto *self = static_cast<NavigationPane *>(user_data);
        if (!self)
            return;
        self->_close_animating = false;
        self->_ring_fade_busy = false;
        if (_lv_obj_t *frame = self->frameRoot())
        {
            sendUiMessage(frame, UiMessageType::NavClose);
        }
    };
    motion.ready_data = this;
    motion.start_value = current_x;
    motion.end_value = closed_x;
    motion.duration_ms = close_slide_ms;
    motion.path = UiMotionPath::EaseIn;
    if (!ui_motion_start(motion))
    {
        _ring_fade_busy = false;
        _close_animating = false;
        return false;
    }
    return true;
}

void NavigationPane::openPanel()
{
    if (!_root || !_nav)
        return;
    const uint16_t open_slide_ms = gridNavOpenAnimMs(_root);

    ui_motion_cancel(_nav);
    ui_motion_cancel(this);

    _ring_fade_busy = true;
    setNavButtonsInteractive(false);
    if (_ring_layout_focus == kNoEmphasis && btnCount() > 0)
    {
        if (_lv_obj_t *host = itemHost())
        {
            const uint32_t n = lv_obj_get_child_cnt(host);
            for (uint32_t i = 0; i < n; ++i)
            {
                _lv_obj_t *cell = lv_obj_get_child(host, i);
                if (!cell)
                    continue;
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
    motion.exec = [](void *var, int32_t v)
    { lv_obj_set_x(static_cast<lv_obj_t *>(var), static_cast<lv_coord_t>(v)); };
    motion.ready = [](void *user_data)
    {
        auto *self = static_cast<NavigationPane *>(user_data);
        if (!self)
            return;
        self->_ring_fade_busy = false;
        self->updateGeometry();
        self->setNavButtonsInteractive(true);
        self->layoutNav(false);
        (void)self->_gridNav.focus(self->_nav);
    };
    motion.ready_data = this;
    motion.start_value = closed_x;
    motion.end_value = nx;
    motion.duration_ms = open_slide_ms;
    motion.path = UiMotionPath::EaseOut;
    if (!ui_motion_start(motion))
    {
        lv_obj_set_x(_nav, nx);
        _ring_fade_busy = false;
        updateGeometry();
        setNavButtonsInteractive(true);
        layoutNav(false);
        (void)_gridNav.focus(_nav);
    }
}

} // namespace Heltec::MeshCore::Ui

#endif // HELTEC_UI_HAS_TOUCH
