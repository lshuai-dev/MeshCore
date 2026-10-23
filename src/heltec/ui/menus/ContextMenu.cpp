#include "ContextMenu.hpp"

#include "ui/app/BehaviorProfile.hpp"
#include "ui/core/HtEvent.h"
#include "ui/core/Scheduler.hpp"
#include "ui/theme/WidgetTheme.hpp"
#include "ui/widgets/WidgetClasses.hpp"

#include <lvgl.h>

#if !HELTEC_UI_HAS_TOUCH

namespace Heltec::MeshCore::Ui
{
namespace
{

struct FrameGeometry
{
    lv_coord_t target_y = 0;
    lv_coord_t start_y = 0;
};

FrameGeometry layoutFrame(lv_obj_t *object)
{
    FrameGeometry geometry{};
    if (!object)
        return geometry;

    lv_obj_t *const parent = lv_obj_get_parent(object);
    if (!parent)
        return geometry;

    lv_obj_update_layout(parent);
    lv_obj_update_layout(object);
    lv_coord_t height = lv_obj_get_height(object);
    if (height <= 0)
        height = lv_obj_get_height(parent);
    if (height <= 0)
        height = lv_disp_get_ver_res(nullptr);

    geometry.target_y = lv_obj_get_y(object);
    geometry.start_y = -height;
    lv_obj_set_y(object, geometry.start_y);
    return geometry;
}

} // namespace

ContextMenu::ContextMenu(Biz::IBizFacade &biz)
    : UiSurface(biz, ht_context_menu_class)
{
}

bool ContextMenu::isEmpty() const
{
    return !_header_icon_row || lv_obj_get_child_cnt(_header_icon_row) == 0;
}

bool ContextMenu::canOpen() const
{
    return _root && _menu && _header_icon_row && _roller.root() && !isEmpty();
}

void ContextMenu::updateIconSelection()
{
    if (!_header_icon_row)
        return;
    const uint32_t count = lv_obj_get_child_cnt(_header_icon_row);
    for (uint32_t index = 0; index < count; ++index)
    {
        lv_obj_t *const button = lv_obj_get_child(_header_icon_row, index);
        if (!button)
            continue;
        const bool selected = index == _active_menu;
        if (selected)
            lv_obj_add_state(button, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(button, LV_STATE_CHECKED);

        lv_obj_t *const icon = lv_obj_get_child(button, 0);
        if (!icon)
            continue;
        if (selected)
            lv_obj_add_state(icon, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(icon, LV_STATE_CHECKED);
    }
}

bool ContextMenu::showPage(const ContextMenuPage *page, uint8_t focus_index)
{
    if (!page || !page->title || !page->items || page->item_count == 0 ||
        !_roller.showItems(page->item_count))
        return false;

    for (uint8_t index = 0; index < page->item_count; ++index)
    {
        if (!page->items[index].label ||
            !_roller.setItemLabelStatic(index, page->items[index].label))
            return false;
    }

    _active_page = page;
    if (_title_label)
    {
        lv_label_set_text_static(_title_label, page->title);
        lv_obj_clear_flag(_title_label, LV_OBJ_FLAG_HIDDEN);
    }

    if (_stage == Stage::Submenu)
    {
        _gridNav.bindRoot(_roller.root());
        if (focus_index >= page->item_count)
            focus_index = 0;
        (void)_roller.focusItem(focus_index, false);
    }
    return true;
}

void ContextMenu::selectPane(uint8_t index)
{
    if (!_header_icon_row)
        return;
    const uint32_t count = lv_obj_get_child_cnt(_header_icon_row);
    if (count == 0)
        return;

    _active_menu = static_cast<uint8_t>(index % count);
    updateIconSelection();
    lv_obj_t *const button = lv_obj_get_child(_header_icon_row, _active_menu);
    auto *const page = button
                           ? static_cast<const ContextMenuPage *>(lv_obj_get_user_data(button))
                           : nullptr;
    (void)showPage(page);
}

bool ContextMenu::enterSelectedPane()
{
    if (!_header_icon_row)
        return false;
    lv_obj_t *const button = lv_obj_get_child(_header_icon_row, _active_menu);
    auto *const page = button
                           ? static_cast<const ContextMenuPage *>(lv_obj_get_user_data(button))
                           : nullptr;
    if (!page)
        return false;

    resetHistory();
    _stage = Stage::Submenu;
    return showPage(page);
}

void ContextMenu::enterMainMenu(uint8_t focus_index)
{
    _stage = Stage::MainMenu;
    resetHistory();
    _gridNav.bindRoot(_header_icon_row);
    lv_obj_t *const button =
        _header_icon_row ? lv_obj_get_child(_header_icon_row, focus_index) : nullptr;
    if (button)
        (void)_gridNav.focus(button);
    else
        (void)_gridNav.enterControls();
}

bool ContextMenu::activateItem(uint8_t index)
{
    if (!_active_page || index >= _active_page->item_count)
        return false;
    const ContextMenuItem &item = _active_page->items[index];
    if (item.submenu)
    {
        if (_history_depth >= kMaxMenuDepth)
            return true;
        _history[_history_depth].page = _active_page;
        _history[_history_depth].focused_index = index;
        ++_history_depth;
        return showPage(item.submenu);
    }
    if (!item.command)
        return false;
    item.command(_biz, eventTarget());
    return true;
}

bool ContextMenu::navigateBack()
{
    if (_stage != Stage::Submenu)
        return false;
    if (_history_depth == 0)
    {
        enterMainMenu(_active_menu);
        return true;
    }

    const HistoryEntry entry = _history[--_history_depth];
    return showPage(entry.page, entry.focused_index);
}

void ContextMenu::resetHistory()
{
    _history_depth = 0;
}

void ContextMenu::handleRollerClicked(lv_event_t *event)
{
    if (!event || lv_event_get_code(event) != LV_EVENT_CLICKED)
        return;
    auto *const self = static_cast<ContextMenu *>(lv_event_get_user_data(event));
    lv_obj_t *const target = lv_event_get_target(event);
    if (!self || !target)
        return;
    if (self->activateItem(static_cast<uint8_t>(lv_obj_get_index(target))))
    {
        lv_event_stop_bubbling(event);
        lv_event_stop_processing(event);
    }
}

bool ContextMenu::onKey(uint32_t key)
{
    if (key != LV_KEY_ESC)
        return false;
    if (_gridNav.isEditing())
        return false;
    if (navigateBack())
        return true;
    return emitEvent(EventType::ContextClose);
}

void ContextMenu::onEnter()
{
    if (!canOpen())
        return;
    ui_motion_cancel(_root);
    _stage = Stage::MainMenu;
    selectPane(0);
    enterMainMenu(0);

    const FrameGeometry geometry = layoutFrame(_root);
    const uint16_t duration = ui_behavior_profile().context_menu.page_anim_ms;
    if (duration == 0)
    {
        lv_obj_set_y(_root, geometry.target_y);
        return;
    }

    UiMotionSpec motion;
    motion.target = _root;
    motion.exec = [](void *object, int32_t value)
    { lv_obj_set_y(static_cast<lv_obj_t *>(object), static_cast<lv_coord_t>(value)); };
    motion.start_value = geometry.start_y;
    motion.end_value = geometry.target_y;
    motion.duration_ms = duration;
    motion.path = UiMotionPath::EaseOut;
    if (!ui_motion_start(motion))
        lv_obj_set_y(_root, geometry.target_y);
}

void ContextMenu::onExit()
{
    resetHistory();
    _stage = Stage::MainMenu;
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

    lv_obj_t *const header = lv_menu_get_main_header(_menu);
    lv_obj_t *const back_button = lv_menu_get_main_header_back_btn(_menu);
    if (!header || !back_button)
        return false;
    lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_HIDDEN);

    const uint32_t header_children = lv_obj_get_child_cnt(header);
    for (uint32_t index = 0; index < header_children; ++index)
    {
        lv_obj_t *const child = lv_obj_get_child(header, index);
        if (child != back_button && lv_obj_check_type(child, &lv_label_class))
        {
            _title_label = child;
            break;
        }
    }
    if (!_title_label)
        return false;

    _header_icon_row = ht_class_create(&ht_context_menu_icon_row_class, header);
    if (!_header_icon_row)
        return false;
    lv_obj_set_flex_flow(_header_icon_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_header_icon_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_move_to_index(_header_icon_row, 0);
    lv_obj_clear_flag(_header_icon_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *const navigation_row = ht_class_create(&ht_context_menu_nav_row_class, header);
    if (!navigation_row)
        return false;
    lv_obj_set_size(navigation_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(navigation_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(navigation_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(navigation_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_parent(_title_label, navigation_row);
    lv_obj_set_parent(back_button, navigation_row);
    lv_obj_set_size(back_button, 0, 0);
    lv_obj_add_flag(back_button, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_width(_title_label, lv_pct(100));
    lv_obj_set_flex_grow(_title_label, 1);
    lv_label_set_long_mode(_title_label, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(_title_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_to_index(navigation_row, 1);
    ui_context_menu_title_apply_theme(_title_label);

    lv_obj_t *const page = ht_class_create(&ht_context_menu_page_class, _menu);
    if (!page)
        return false;
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_menu_set_page(_menu, page);
    lv_menu_clear_history(_menu);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_HIDDEN);
    if (!_roller.create(page, &_gridNav, &ht_context_menu_roller_class,
                        &ht_context_menu_item_class, &ht_context_menu_label_class))
        return false;

    _gridNav.setWrap(true);
    _gridNav.setEditing(false);
    return true;
}

bool ContextMenu::ensureRollerCapacity(const ContextMenuPage &page, uint8_t depth)
{
    if (depth > kMaxMenuDepth)
        return false;
    const uint8_t old_capacity = _roller.root()
                                     ? static_cast<uint8_t>(lv_obj_get_child_cnt(_roller.root()))
                                     : 0;
    if (!_roller.ensureCapacity(page.item_count))
        return false;
    const uint8_t new_capacity = static_cast<uint8_t>(lv_obj_get_child_cnt(_roller.root()));
    for (uint8_t index = old_capacity; index < new_capacity; ++index)
    {
        lv_obj_t *const item = lv_obj_get_child(_roller.root(), index);
        if (item)
            lv_obj_add_event_cb(item, handleRollerClicked, LV_EVENT_CLICKED, this);
    }
    for (uint8_t index = 0; index < page.item_count; ++index)
    {
        if (page.items[index].submenu &&
            !ensureRollerCapacity(*page.items[index].submenu, static_cast<uint8_t>(depth + 1U)))
            return false;
    }
    return true;
}

bool ContextMenu::registerMenu(const lv_img_dsc_t *icon, const ContextMenuPage &page)
{
    if (!_root || !_header_icon_row || !_menu || !icon || !page.title || !page.items ||
        page.item_count == 0 || lv_obj_get_child_cnt(_header_icon_row) >= kMaxPanes ||
        !ensureRollerCapacity(page))
        return false;

    lv_obj_t *const button =
        ht_btn_create(&ht_context_menu_icon_button_class, _header_icon_row,
                      const_cast<ContextMenuPage *>(&page));
    if (!button)
        return false;
    lv_obj_set_flex_flow(button, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(
        button,
        [](lv_event_t *event)
        {
            auto *const self = static_cast<ContextMenu *>(lv_event_get_user_data(event));
            if (self)
                self->selectPane(
                    static_cast<uint8_t>(lv_obj_get_index(lv_event_get_target(event))));
        },
        LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(
        button,
        [](lv_event_t *event)
        {
            auto *const self = static_cast<ContextMenu *>(lv_event_get_user_data(event));
            if (!self || !self->enterSelectedPane())
                return;
            lv_event_stop_bubbling(event);
            lv_event_stop_processing(event);
        },
        LV_EVENT_CLICKED, this);

    lv_obj_t *const image = ht_class_create(&ht_context_menu_icon_class, button);
    if (!image)
        return false;
    lv_img_set_src(image, icon);
    lv_img_set_antialias(image, false);
    const lv_coord_t width = static_cast<lv_coord_t>(icon->header.w);
    const lv_coord_t height = static_cast<lv_coord_t>(icon->header.h);
    lv_obj_set_size(image, width, height);
    lv_obj_set_size(button, width, height);
    if (height > _icon_row_height)
    {
        _icon_row_height = height;
        lv_obj_set_height(_header_icon_row, _icon_row_height);
    }

    _gridNav.add(button);
    return true;
}

} // namespace Heltec::MeshCore::Ui

#endif // !HELTEC_UI_HAS_TOUCH
