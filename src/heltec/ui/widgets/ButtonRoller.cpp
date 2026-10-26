#include "ButtonRoller.hpp"

#include "MeshCore.h"
#include "ui/app/BehaviorProfile.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/widgets/SemanticClasses.hpp"
#include "ui/core/Scheduler.hpp"
#include "ui/theme/WidgetTheme.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/DisplayPort.hpp"
#endif

namespace Heltec::MeshCore::Ui
{
namespace
{

void setItemSelected(lv_obj_t *item, bool selected)
{
    if (!item)
        return;
    if (selected)
    {
        lv_obj_add_state(item, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_clear_state(item, LV_STATE_CHECKED);
    }
}

void clearItemFocusState(lv_obj_t *item)
{
    if (!item)
        return;
    lv_obj_clear_state(item, LV_STATE_CHECKED | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
}

} // namespace

void ButtonRoller::layoutItems(uint8_t focus_index, lv_anim_enable_t anim, bool is_checked)
{
    if (_root && _active_count > 0 && _gridNav)
    {
        lv_obj_t *focus_obj = _gridNav->focused();
        if (focus_obj && lv_obj_get_parent(focus_obj) == _root)
        {
            focus_index = lv_obj_get_index(focus_obj);
        }
        lv_obj_t *const focused_item = itemAt(focus_index);
        if (focused_item)
        {
            const uint16_t ms = behaviorProfile().context_menu.page_anim_ms;
            auto const anim_set_y =
                +[](void *obj, int32_t v) { lv_obj_set_y(static_cast<lv_obj_t *>(obj), v); };

            lv_coord_t vp_h = lv_obj_get_content_height(_root);
            lv_coord_t row_h = lv_obj_get_height(focused_item);
            if (vp_h <= 0 || row_h <= 0)
            {
                lv_obj_update_layout(_root);
                vp_h = lv_obj_get_content_height(_root);
                row_h = lv_obj_get_height(focused_item);
            }
            if (row_h <= 0)
                row_h = vp_h;
            if (row_h <= 0 || vp_h < row_h)
            {
                if (anim == LV_ANIM_OFF && !is_checked)
                    _last_vp_h = 0;
                return;
            }

            if (anim == LV_ANIM_OFF && !is_checked && vp_h == _last_vp_h)
                return;

            if (anim != LV_ANIM_OFF && is_checked && focus_index == _last_layout_focus &&
                vp_h == _last_vp_h)
            {
                setItemSelected(focused_item, true);
                return;
            }

            uint8_t maxVisible = 1;
            if (vp_h > row_h)
            {
                maxVisible = static_cast<uint8_t>((vp_h - row_h) / row_h + 1);
                if (maxVisible < 1)
                    maxVisible = 1;
            }
            uint8_t visibleItems = LV_MIN(maxVisible, _active_count);
            if (visibleItems < 1)
                visibleItems = 1;

            const lv_coord_t center_offset = (vp_h - row_h) / 2;

            uint8_t i_max = (visibleItems - 1) / 2;
            int8_t i_min = (0 != i_max) ? static_cast<int8_t>(-i_max) : 0;
            if ((visibleItems - 1) % 2)
                --i_min;

            bool item_visible[kMaxItems]{};
            for (int8_t i = i_min; i <= i_max; ++i)
            {
                int8_t index = static_cast<int8_t>((focus_index + i) % _active_count);
                if (index < 0)
                    index += static_cast<int8_t>(_active_count);
                item_visible[index] = true;
            }

            for (uint8_t index = 0; index < _active_count; ++index)
            {
                lv_obj_t *const item = itemAt(index);
                if (!item)
                    continue;
                if (item_visible[index])
                {
                    lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
                }
                else
                {
                    cancelMotion(item, anim_set_y);
                    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
                }
            }

            for (int8_t i = i_min; i <= i_max; ++i)
            {
                int8_t index = static_cast<int8_t>((focus_index + i) % _active_count);
                if (index < 0)
                    index += static_cast<int8_t>(_active_count);

                const lv_coord_t target_y = center_offset + i * row_h;
                lv_obj_t *const item = itemAt(static_cast<uint8_t>(index));
                if (!item)
                    continue;
                cancelMotion(item, anim_set_y);
                if (anim == LV_ANIM_OFF || ms == 0)
                {
                    lv_obj_set_y(item, target_y);
                    if (is_checked && 0 == i)
                        setItemSelected(item, true);
                    continue;
                }

                UiMotionSpec motion;
                motion.target = item;
                if (is_checked && 0 == i)
                {
                    motion.ready = [](void *user_data)
                    {
                        lv_obj_t *btn = static_cast<lv_obj_t *>(user_data);
                        if (btn)
                            setItemSelected(btn, true);
                    };
                    motion.ready_data = item;
                }
                motion.exec = anim_set_y;
                motion.start_value = lv_obj_get_y(item);
                motion.end_value = target_y;
                motion.duration_ms = ms;
                if (!startMotion(motion))
                {
                    lv_obj_set_y(item, target_y);
                    if (is_checked && 0 == i)
                        setItemSelected(item, true);
                }
            }
            _last_vp_h = vp_h;
            _last_layout_focus = focus_index;
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
            lv_obj_invalidate(_root);
            Heltec::MeshCore::Dal::DisplayPort::requestSelectionRefresh("button-roller-selection");
#endif
        }
    }
}

lv_obj_t *ButtonRoller::addItem(const char *label)
{
    if (!_root || lv_obj_get_child_cnt(_root) >= kMaxItems || !_gridNav)
        return nullptr;

    lv_obj_t *const btn = ht_class_create(_item_class, _root);
    if (!btn)
        return nullptr;
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_t *const lbl = lv_label_create(btn);
    if (lbl)
    {
        // Menu builders may pass a temporary formatting buffer. Copy it once
        // while constructing the fixed roller item pool.
        ht_label_set_text_copy(lbl, label);
        lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
    }

    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e)
        {
            ButtonRoller *const roller = static_cast<ButtonRoller *>(lv_event_get_user_data(e));
            lv_obj_t *const target = lv_event_get_target(e);
            if (roller && target && roller->root())
            {
                roller->layoutItems(lv_obj_get_index(target), LV_ANIM_ON, true);
            }
        },
        LV_EVENT_FOCUSED, this);

    lv_obj_add_event_cb(
        btn,
        [](lv_event_t *e)
        {
            lv_obj_t *btn = lv_event_get_target(e);
            if (btn)
            {
                setItemSelected(btn, false);
            }
        },
        LV_EVENT_DEFOCUSED, this);

    ++_active_count;
    _gridNav->add(btn);
    return btn;
}

lv_obj_t *ButtonRoller::itemAt(uint8_t index) const
{
    if (!_root || index >= lv_obj_get_child_cnt(_root))
        return nullptr;
    return lv_obj_get_child(_root, index);
}

bool ButtonRoller::ensureCapacity(uint8_t count)
{
    if (!_root || count > kMaxItems)
        return false;
    while (lv_obj_get_child_cnt(_root) < count)
    {
        if (!addItem(""))
            return false;
    }
    return true;
}

bool ButtonRoller::showItems(uint8_t count)
{
    if (!ensureCapacity(count))
        return false;
    lv_obj_t *const focused = _gridNav ? _gridNav->focused() : nullptr;
    if (focused && lv_obj_get_parent(focused) == _root)
        _gridNav->enterBrowse();
    const uint8_t capacity = static_cast<uint8_t>(lv_obj_get_child_cnt(_root));
    _active_count = count;
    for (uint8_t index = 0; index < capacity; ++index)
    {
        lv_obj_t *const item = itemAt(index);
        if (!item)
            continue;
        clearItemFocusState(item);
        if (index < count)
        {
            lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
            _gridNav->add(item);
        }
        else
        {
            lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
            _gridNav->remove(item);
        }
    }
    _last_layout_focus = 0xFF;
    _last_vp_h = 0;
    if (count > 0)
        layoutItems(0, LV_ANIM_OFF, false);
    return true;
}

bool ButtonRoller::setItemLabelStatic(uint8_t index, const char *label)
{
    lv_obj_t *const item = itemAt(index);
    lv_obj_t *const label_obj = item ? lv_obj_get_child(item, 0) : nullptr;
    if (!label_obj || !label)
        return false;
    lv_label_set_text_static(label_obj, label);
    return true;
}

bool ButtonRoller::create(lv_obj_t *parent, HtGridNav *navigator,
                          const lv_obj_class_t *root_class,
                          const lv_obj_class_t *item_class)
{
    if (!parent || !navigator)
        return false;
    if (_root)
        return true;

    _gridNav = navigator;
    _item_class = item_class ? item_class : &ht_button_roller_item_class;
    _root = ht_class_create(root_class ? root_class : &ht_button_roller_class, parent);
    if (!_root)
        return false;
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_add_event_cb(
        _root,
        [](lv_event_t *e)
        {
            if (LV_EVENT_SIZE_CHANGED == lv_event_get_code(e))
            {
                if (lv_event_get_target(e) == lv_event_get_current_target(e))
                {
                    ButtonRoller *const roller =
                        static_cast<ButtonRoller *>(lv_event_get_user_data(e));
                    if (roller && roller->root() && roller->itemCount() > 0)
                    {
                        roller->layoutItems(0, LV_ANIM_OFF, false);
                    }
                }
            }
        },
        LV_EVENT_SIZE_CHANGED, this);

    return true;
}

void ButtonRoller::resetFocus(bool focus_first_item)
{
    if (!_gridNav || _active_count == 0)
        return;
    for (uint8_t i = 0; i < _active_count; ++i)
    {
        clearItemFocusState(itemAt(i));
    }
    _last_layout_focus = 0xFF;
    _last_vp_h = 0;
    if (focus_first_item)
    {
        (void)_gridNav->focus(itemAt(0));
        layoutItems(0, LV_ANIM_OFF, false);
    }
    else
    {
        lv_obj_t *const focused = _gridNav->focused();
        if (focused && lv_obj_get_parent(focused) == _root)
            _gridNav->enterBrowse();
        layoutItems(0, LV_ANIM_OFF, false);
    }
}

void ButtonRoller::setInputEnabled(bool enabled)
{
    if (!_gridNav || _input_enabled == enabled)
        return;

    _input_enabled = enabled;
    for (uint8_t i = 0; i < _active_count; ++i)
    {
        lv_obj_t *const item = itemAt(i);
        if (!item)
            continue;
        if (enabled)
            _gridNav->add(item);
        else
            _gridNav->remove(item);
    }
}

bool ButtonRoller::focusItem(uint8_t index, bool checked)
{
    if (!_gridNav || index >= _active_count || !itemAt(index))
        return false;
    for (uint8_t i = 0; i < _active_count; ++i)
    {
        clearItemFocusState(itemAt(i));
    }
    _last_layout_focus = 0xFF;
    _last_vp_h = 0;
    (void)_gridNav->focus(itemAt(index));
    layoutItems(index, LV_ANIM_OFF, checked);
    return true;
}

} // namespace Heltec::MeshCore::Ui
