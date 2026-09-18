#include "context_menu.hpp"

#include "heltec/ui/app/theme.hpp"
#include "heltec/ui/core/abstract_menu.hpp"
#include "heltec/ui/images.h"
#include "ui/app/behavior_profile.hpp"
#include "ui/core/ht_event.h"
#include "ui/core/widget_classes.hpp"
#include "ui/core/scheduler.hpp"
#include "ui/theme/widget_theme.hpp"

#include <lvgl.h>

#if !HELTEC_HAS_TOUCH

namespace heltec::meshcore::ui
{

ContextMenu::ContextMenu(biz::IBizFacade &biz)
    : UiSurface(biz, ht_context_menu_class)
{
}

namespace
{

struct FrameGeometry
{
    lv_coord_t parent_w = 0;
    lv_coord_t parent_h = 0;
    lv_coord_t root_w = 0;
    lv_coord_t root_h = 0;
    lv_coord_t root_x = 0;
    lv_coord_t target_y = 0;
    lv_coord_t start_y = 0;
};

uint16_t contextPageAnimMs(const lv_obj_t *obj)
{
    (void)obj;
    return ui_behavior_profile().context_menu.page_anim_ms;
}

FrameGeometry layoutFrame(_lv_obj_t *obj)
{
    FrameGeometry geom{};
    if (!obj)
        return geom;
    _lv_obj_t *parent = lv_obj_get_parent(obj);
    if (!parent)
        return geom;

    lv_obj_update_layout(parent);
    lv_coord_t parent_w = lv_obj_get_width(parent);
    lv_coord_t parent_h = lv_obj_get_height(parent);

    if (parent_w <= 0)
        parent_w = lv_disp_get_hor_res(nullptr);
    if (parent_h <= 0)
        parent_h = lv_disp_get_ver_res(nullptr);

    lv_obj_update_layout(obj);
    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    if (w <= 0)
        w = parent_w;
    if (h <= 0)
        h = parent_h;
    geom.parent_w = parent_w;
    geom.parent_h = parent_h;
    geom.root_w = w;
    geom.root_h = h;
    geom.root_x = lv_obj_get_x(obj);
    geom.target_y = lv_obj_get_y(obj);
    geom.start_y = -h;
    lv_obj_set_y(obj, geom.start_y);
    return geom;
}

} // namespace

bool ContextMenu::isEmpty() const
{
    return !_header_icon_row || lv_obj_get_child_cnt(_header_icon_row) == 0;
}

bool ContextMenu::canOpen() const
{
    return _root && _menu && _header_icon_row && focusGroup() && !isEmpty();
}

void ContextMenu::updateIconSelection()
{
    if (!_header_icon_row)
        return;
    const uint32_t n = lv_obj_get_child_cnt(_header_icon_row);
    for (uint32_t i = 0; i < n; ++i)
    {
        lv_obj_t *b = lv_obj_get_child(_header_icon_row, i);
        if (!b)
            continue;
        if (i == _active_menu)
        {
            lv_obj_add_state(b, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_clear_state(b, LV_STATE_CHECKED);
        }

        lv_obj_t *icon = lv_obj_get_child(b, 0);
        if (icon)
        {
            if (i == _active_menu)
                lv_obj_add_state(icon, LV_STATE_CHECKED);
            else
                lv_obj_clear_state(icon, LV_STATE_CHECKED);
        }
    }
}

void ContextMenu::selectPane(uint8_t index)
{
    if (_registering_panes || !_header_icon_row || !_menu)
        return;
    const uint32_t n = lv_obj_get_child_cnt(_header_icon_row);
    if (n == 0)
        return;

    _active_menu = static_cast<uint8_t>(index % n);
    updateIconSelection();

    lv_obj_t *btn = lv_obj_get_child(_header_icon_row, _active_menu);
    auto *menu = btn ? static_cast<AbstractMenu *>(lv_obj_get_user_data(btn)) : nullptr;
    if (!menu || !menu->page())
        return;

    if (lv_menu_get_cur_main_page(_menu) != menu->page())
    {
        lv_menu_set_page(_menu, menu->page());
    }
    if (!_in_leaf)
        menu->resetInputState(false);
}

void ContextMenu::enterMenuLeaf(AbstractMenu *menu)
{
    if (_registering_panes || !menu)
        return;
    _in_leaf = true;
    _leaf_menu = menu;
    setHeaderInputEnabled(false);
    menu->resetInputState(true);
}

void ContextMenu::setHeaderInputEnabled(bool enabled)
{
    if (!_header_icon_row || !focusGroup())
        return;

    const bool was_registering = _registering_panes;
    _registering_panes = true;
    if (!enabled)
        lv_group_focus_obj(nullptr);

    const uint32_t count = lv_obj_get_child_cnt(_header_icon_row);
    for (uint32_t i = 0; i < count; ++i)
    {
        lv_obj_t *const button = lv_obj_get_child(_header_icon_row, i);
        if (!button)
            continue;
        if (enabled)
            lv_group_add_obj(focusGroup(), button);
        else
            lv_group_remove_obj(button);
    }
    _registering_panes = was_registering;
}

bool ContextMenu::activatePaneButton(lv_obj_t *btn, bool enter_leaf)
{
    if (!btn || !_root || lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN))
        return false;

    const uint8_t index = static_cast<uint8_t>(lv_obj_get_index(btn));
    selectPane(index);
    if (!enter_leaf)
        return true;

    auto *menu = static_cast<AbstractMenu *>(lv_obj_get_user_data(btn));
    if (!menu)
        return false;
    enterMenuLeaf(menu);
    return true;
}

void ContextMenu::handleLeafEsc()
{
    if (!_in_leaf)
        return;
    leaveMenuLeaf();
}

void ContextMenu::on_menu_leaf_esc(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY || lv_event_get_key(e) != LV_KEY_ESC)
        return;
    auto *self = static_cast<ContextMenu *>(lv_event_get_user_data(e));
    if (!self)
        return;
    self->handleLeafEsc();
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

void ContextMenu::leaveMenuLeaf()
{
    if (!_in_leaf)
        return;
    if (_leaf_menu)
        _leaf_menu->resetInputState(false);
    _in_leaf = false;
    _leaf_menu = nullptr;
    selectPane(_active_menu);
    setHeaderInputEnabled(true);
    lv_obj_t *const button = lv_obj_get_child(_header_icon_row, _active_menu);
    if (button)
        lv_group_focus_obj(button);
}

void ContextMenu::onEnter()
{
    if (!canOpen())
        return;
    ui_motion_cancel(_root);
    leaveMenuLeaf();
    selectPane(0);
    lv_obj_t *const button = lv_obj_get_child(_header_icon_row, _active_menu);
    if (button)
        lv_group_focus_obj(button);
    lv_menu_clear_history(_menu);
    const FrameGeometry geom = layoutFrame(_root);
    const uint16_t anim_ms = contextPageAnimMs(_root);
    if (anim_ms == 0)
    {
        lv_obj_set_y(_root, geom.target_y);
    }
    else
    {
        UiMotionSpec motion;
        motion.target = _root;
        motion.exec = [](void *var, int32_t value)
        { lv_obj_set_y(static_cast<lv_obj_t *>(var), static_cast<lv_coord_t>(value)); };
        motion.start_value = geom.start_y;
        motion.end_value = geom.target_y;
        motion.duration_ms = anim_ms;
        motion.path = UiMotionPath::EaseOut;
        if (!ui_motion_start(motion))
            lv_obj_set_y(_root, geom.target_y);
    }
}

void ContextMenu::onExit()
{
    leaveMenuLeaf();
    if (_root)
        ui_motion_cancel(_root);
}

bool ContextMenu::buildSurface()
{
    lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);

    _menu = lv_menu_create(_root);
    if (!_menu)
        return false;
    lv_obj_set_size(_menu, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(_menu, 1);
    lv_obj_clear_flag(_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *mh = lv_menu_get_main_header(_menu);
    if (!mh)
        return false;
    lv_obj_set_size(mh, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(mh, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mh, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *back_btn = lv_menu_get_main_header_back_btn(_menu);
    lv_obj_t *title_lbl = nullptr;
    const uint32_t header_child_count = lv_obj_get_child_cnt(mh);
    for (uint32_t i = 0; i < header_child_count; ++i)
    {
        lv_obj_t *child = lv_obj_get_child(mh, i);
        if (child != back_btn && lv_obj_check_type(child, &lv_label_class))
        {
            title_lbl = child;
            break;
        }
    }
    if (!back_btn || !title_lbl)
        return false;

    lv_obj_clear_flag(mh, LV_OBJ_FLAG_HIDDEN);
    _header_icon_row = ht_class_create(&ht_context_menu_icon_row_class, mh);
    if (!_header_icon_row)
        return false;
    lv_obj_set_flex_flow(_header_icon_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_header_icon_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_move_to_index(_header_icon_row, 0);
    lv_obj_clear_flag(_header_icon_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *nav_row = lv_obj_create(mh);
    if (!nav_row)
        return false;
    lv_obj_set_size(nav_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(nav_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(nav_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_parent(title_lbl, nav_row);
    lv_obj_set_parent(back_btn, nav_row);
    lv_obj_set_size(back_btn, 0, 0);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_width(title_lbl, lv_pct(100));
    lv_obj_set_flex_grow(title_lbl, 1);
    lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(title_lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_to_index(nav_row, 1);
    ui_theme_apply_context_menu_title(title_lbl);

    lv_group_set_wrap(focusGroup(), true);
    lv_group_set_editing(focusGroup(), false);
    return true;
}

void ContextMenu::beginRegister()
{
    _registering_panes = true;
    if (focusGroup())
        lv_group_focus_freeze(focusGroup(), true);
}

void ContextMenu::endRegister()
{
    _registering_panes = false;
    if (focusGroup())
        lv_group_focus_freeze(focusGroup(), false);
}

bool ContextMenu::registerMenu(const char *title, const lv_img_dsc_t *icon, AbstractMenu &menu)
{
    if (!_root || !_header_icon_row || !_menu || !icon || !title)
        return false;
    if (lv_obj_get_child_cnt(_header_icon_row) >= kMaxPanes)
        return false;
    if (!menu.create(_menu, focusGroup(), title, eventTarget()))
        return false;
    menu.resetInputState(false);

    if (menu.root())
        lv_obj_add_event_cb(menu.root(), &ContextMenu::on_menu_leaf_esc, LV_EVENT_KEY, this);
    if (lv_obj_t *page = menu.page())
        lv_obj_add_event_cb(page, &ContextMenu::on_menu_leaf_esc, LV_EVENT_KEY, this);

    lv_obj_t *btn =
        ht_btn_create(&ht_context_menu_icon_button_class, _header_icon_row, &menu);
    if (!btn)
        return false;

    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e)
        {
            auto *self = static_cast<ContextMenu *>(lv_event_get_user_data(e));
            if (self)
                (void)self->activatePaneButton(lv_event_get_target(e), false);
        },
        LV_EVENT_FOCUSED, this);

    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e)
        {
            auto *self = static_cast<ContextMenu *>(lv_event_get_user_data(e));
            if (!self || !self->activatePaneButton(lv_event_get_target(e), true))
                return;
            lv_event_stop_bubbling(e);
            lv_event_stop_processing(e);
        },
        LV_EVENT_CLICKED, this);

    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e)
        {
            if (lv_event_get_code(e) != LV_EVENT_KEY)
                return;
            auto *ctx_menu = static_cast<ContextMenu *>(lv_event_get_user_data(e));
            if (!ctx_menu)
                return;

            if (lv_event_get_key(e) != LV_KEY_ESC)
                return;
            ctx_menu->emitEvent(EventType::ContextClose);
            lv_event_stop_bubbling(e);
            lv_event_stop_processing(e);
        },
        LV_EVENT_KEY, this);

    lv_obj_t *im = lv_img_create(btn);
    if (!im)
        return false;
    lv_img_set_src(im, icon);
    lv_img_set_antialias(im, false);

    const lv_coord_t icon_width = static_cast<lv_coord_t>(icon->header.w);
    const lv_coord_t icon_height = static_cast<lv_coord_t>(icon->header.h);
    lv_obj_set_size(im, icon_width, icon_height);
    lv_obj_set_size(btn, icon_width, icon_height);
    if (icon_height > _icon_row_height)
    {
        _icon_row_height = icon_height;
        lv_obj_set_height(_header_icon_row, _icon_row_height);
    }

    lv_group_add_obj(focusGroup(), btn);
    return true;
}

} // namespace heltec::meshcore::ui

#endif // !HELTEC_HAS_TOUCH
