#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

#include "find_friend_screen.hpp"
#include "app/team_join_service.hpp"
#include "ui/app/theme.hpp"
#include "ui/core/ht_container.hpp"
#include "ui/core/ht_event.h"
#include "ui/theme/widget_theme.hpp"
#include <lvgl.h>
#include <string.h>

namespace heltec::meshcore::ui
{
namespace
{
using heltec::meshcore::team::JoinState;

#if defined(HELTEC_V4_R8_TFT)
constexpr lv_coord_t kFindFriendRowHeight = 28;
constexpr lv_coord_t kFindFriendRowPadVer = 1;
constexpr lv_coord_t kDropdownHeight = kFindFriendRowHeight - 2 * kFindFriendRowPadVer;
#else
constexpr lv_coord_t kFindFriendRowHeight = ui_settings_row_height();
#endif

void set_hidden(_lv_obj_t *obj, bool hidden)
{
    if (!obj)
        return;
    if (hidden)
    {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

bool hidden_in_tree(const _lv_obj_t *obj)
{
    for (const _lv_obj_t *current = obj; current; current = lv_obj_get_parent(current))
    {
        if (lv_obj_has_flag(current, LV_OBJ_FLAG_HIDDEN))
            return true;
    }
    return false;
}

bool focus_object_available(const _lv_obj_t *obj)
{
    return obj && lv_obj_is_valid(obj) && !hidden_in_tree(obj) &&
           !lv_obj_has_state(obj, LV_STATE_DISABLED);
}

uint32_t options_hash(const char *text)
{
    uint32_t hash = 2166136261UL;
    if (!text)
        return hash;
    for (const uint8_t *p = reinterpret_cast<const uint8_t *>(text); *p; ++p)
    {
        hash ^= *p;
        hash *= 16777619UL;
    }
    return hash;
}

void configure_row(_lv_obj_t *row, lv_flex_align_t main_align)
{
    if (!row)
        return;
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, main_align, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

void configure_label(_lv_obj_t *label, lv_coord_t width)
{
    if (!label)
        return;
    lv_obj_set_width(label, width);
    lv_obj_clear_flag(label,
                      LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
}

bool cycle_dropdown(_lv_obj_t *dropdown, uint32_t key)
{
    if (!dropdown || !lv_dropdown_is_open(dropdown))
        return false;
    const uint16_t count = lv_dropdown_get_option_cnt(dropdown);
    if (count <= 1)
        return false;

    const uint16_t current = lv_dropdown_get_selected(dropdown);
    uint16_t next = current;
    if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT)
    {
        next = static_cast<uint16_t>((current + 1u) % count);
    }
    else if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV)
    {
        next =
            current == 0 ? static_cast<uint16_t>(count - 1u) : static_cast<uint16_t>(current - 1u);
    }
    else
    {
        return false;
    }
    lv_dropdown_set_selected(dropdown, next);
    if (_lv_obj_t *const list = lv_dropdown_get_list(dropdown))
        lv_obj_invalidate(list);
    return true;
}

} // namespace

_lv_obj_t *FindFriendScreen::addDropdownRow(ChoiceRow &choice, const char *title,
                                            const char *options)
{
    choice = ChoiceRow{};
    choice.row = ht_container_create(_right_column);
    if (!choice.row)
        return nullptr;
    configure_row(choice.row, LV_FLEX_ALIGN_START);
    choice.label = lv_label_create(choice.row);
    if (choice.label)
        lv_label_set_text(choice.label, title);
    configure_label(choice.label, LV_SIZE_CONTENT);
    choice.dropdown = lv_dropdown_create(choice.row);
    if (!choice.dropdown)
        return choice.row;

    lv_dropdown_set_options_static(choice.dropdown, options ? options : "");
    lv_obj_set_width(choice.dropdown, 0);
    lv_obj_set_flex_grow(choice.dropdown, 1);
#if LV_COLOR_DEPTH == 1
    lv_obj_set_height(choice.dropdown, 12);
#elif defined(HELTEC_V4_R8_TFT)
    lv_obj_set_height(choice.dropdown, kDropdownHeight);
    ui_theme_center_dropdown_value(choice.dropdown);
#else
    lv_obj_set_height(choice.dropdown, LV_SIZE_CONTENT);
#endif
    lv_obj_clear_flag(choice.dropdown, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(choice.dropdown, LV_SCROLLBAR_MODE_OFF);
    lv_dropdown_set_dir(choice.dropdown, LV_DIR_BOTTOM);
    lv_dropdown_set_selected_highlight(choice.dropdown, true);
    bindControl(choice.dropdown);
    lv_obj_add_event_cb(choice.dropdown, onDropdownValueChanged, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(choice.dropdown, onDropdownStateEvent, LV_EVENT_READY, this);
    lv_obj_add_event_cb(choice.dropdown, onDropdownStateEvent, LV_EVENT_CANCEL, this);
    lv_obj_add_event_cb(choice.dropdown, onDropdownReleasedPre,
                        static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS),
                        this);
    return choice.dropdown;
}

_lv_obj_t *FindFriendScreen::addActionRow(const char *title, Action action)
{
    _lv_obj_t *row = ht_container_create(_right_column);
    if (!row)
        return nullptr;
    configure_row(row, LV_FLEX_ALIGN_START);
    _lv_obj_t *label = lv_label_create(row);
    if(label) lv_label_set_text(label, title);
    configure_label(label, lv_pct(100));
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_opa(row, LV_OPA_40, LV_STATE_DISABLED);
    bindControl(row);
    lv_obj_add_event_cb(row, onActionClicked, LV_EVENT_CLICKED, this);
    if (action == Action::WaypointManual)
        _row_wp_manual = row;
    return row;
}

void FindFriendScreen::bindControl(_lv_obj_t *control)
{
    if (!control)
        return;
    lv_obj_clear_flag(control, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(control, onControlKeyPreprocess,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
}

void FindFriendScreen::createControls()
{
    // LVGL parents popup lists to the screen, outside this scrolling column.
    // Follow the anchor when focus scrolling or a drag moves the column.
    if (_right_column)
    {
        lv_obj_add_event_cb(_right_column, onRightColumnScrolled, LV_EVENT_SCROLL, this);
    }
    addDropdownRow(_choice_mode, "mode", "friend\nwaypoint\nnone");
#if HELTEC_TEAM_FEATURE
    if (_biz.teamStatus().is_leader)
    {
        _row_team = addActionRow("> create team", Action::Team);
    }
    addDropdownRow(_choice_team, "team", "(none)");
#endif
    addDropdownRow(_choice_friend, "member", "(none)");
#if HELTEC_TEAM_FEATURE
    _row_team_message = addActionRow("> Team message", Action::TeamMessage);
#endif
    addActionRow("> set waypoint coordinates", Action::WaypointManual);
}

void FindFriendScreen::configureFocusItems()
{
    /* Keep keypad/encoder traversal identical to the visual row order. */
    addFocusableObject(_choice_mode.dropdown, _choice_mode.row);
    addFocusableObject(_row_team);
    addFocusableObject(_choice_team.dropdown, _choice_team.row);
    addFocusableObject(_choice_friend.dropdown, _choice_friend.row);
    addFocusableObject(_row_team_message);
    addFocusableObject(_row_wp_manual);
    // Match System Screen's deterministic focus scrolling. Pointer focus must
    // not snap the column after a drag or tap; keypad/encoder traversal owns it.
    _lv_obj_t *const focus_controls[] = {
        _row_team, _choice_team.dropdown,
        _choice_mode.dropdown,
        _choice_friend.dropdown,
        _row_team_message,
        _row_wp_manual,
    };
    for (_lv_obj_t *control : focus_controls)
    {
        if (control)
            lv_obj_clear_flag(control, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    }

    lv_obj_set_user_data(_root, this);
    if (group())
    {
        lv_group_set_focus_cb(group(), onGroupFocusChanged);
    }
}

void FindFriendScreen::onRightColumnScrolled(lv_event_t *event)
{
    auto *const self = static_cast<FindFriendScreen *>(lv_event_get_user_data(event));
    if (!self || !self->_open_dropdown)
        return;
    if (lv_async_call(realignDropdownListAsync, self->_open_dropdown) != LV_RES_OK)
        self->realignDropdownList(self->_open_dropdown);
}

void FindFriendScreen::onGroupFocusChanged(lv_group_t *group)
{
    _lv_obj_t *const focused = lv_group_get_focused(group);
    if (!focused)
        return;
    for (_lv_obj_t *object = focused; object; object = lv_obj_get_parent(object))
    {
        auto *const self = static_cast<FindFriendScreen *>(lv_obj_get_user_data(object));
        if (self && self->root() == object)
        {
            self->applyGroupFocus(focused);
            return;
        }
    }
}

void FindFriendScreen::scrollFocusedIntoView(_lv_obj_t *focused) const
{
    if (!_right_column || !focused)
        return;

    _lv_obj_t *row = focused;
    while (row && lv_obj_get_parent(row) != _right_column)
    {
        row = lv_obj_get_parent(row);
    }
    if (!row || row == _right_column)
        return;

    lv_obj_update_layout(_right_column);
    lv_area_t viewport{};
    lv_area_t row_area{};
    lv_obj_get_content_coords(_right_column, &viewport);
    lv_obj_get_coords(row, &row_area);

    const lv_coord_t margin =
#if defined(HELTEC_V4_R8_TFT)
        LV_DPX(6);
#else
        1;
#endif
    const lv_coord_t visible_top = viewport.y1 + margin;
    const lv_coord_t visible_bottom = viewport.y2 - margin;
    lv_coord_t target = lv_obj_get_scroll_top(_right_column);
    if (row_area.y1 < visible_top)
    {
        target -= visible_top - row_area.y1;
    }
    else if (row_area.y2 > visible_bottom)
    {
        target += row_area.y2 - visible_bottom;
    }
    else
    {
        return;
    }

    const lv_coord_t max_scroll =
        lv_obj_get_scroll_top(_right_column) + lv_obj_get_scroll_bottom(_right_column);
    if (target < 0)
        target = 0;
    if (target > max_scroll)
        target = max_scroll;
    lv_obj_scroll_to_y(_right_column, target, LV_ANIM_OFF);
}

void FindFriendScreen::applyGroupFocus(_lv_obj_t *focused)
{
    if (!focused)
        return;
    if (_open_dropdown && focused != _open_dropdown)
        closeOpenDropdown();

    lv_indev_t *const indev = lv_indev_get_act();
    if (!indev)
        return;
    const lv_indev_type_t type = lv_indev_get_type(indev);
    if (type == LV_INDEV_TYPE_KEYPAD || type == LV_INDEV_TYPE_ENCODER)
    {
        scrollFocusedIntoView(focused);
    }
}

void FindFriendScreen::setDropdownIndex(_lv_obj_t *dropdown, uint16_t index, bool force)
{
    if (!dropdown)
        return;
    const uint16_t count = lv_dropdown_get_option_cnt(dropdown);
    if (count == 0)
        return;
    if (index >= count)
        index = count - 1;
    if (!force && lv_dropdown_get_selected(dropdown) == index)
        return;
    _syncing_dropdown = true;
    lv_dropdown_set_selected(dropdown, index);
    _syncing_dropdown = false;
}

void FindFriendScreen::syncControls(bool force_friend_options)
{
    const bool enabled = _biz.findFriendEnabled();
    const biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    if (_open_dropdown != _choice_mode.dropdown)
    {
        const uint16_t mode = enabled ? static_cast<uint16_t>(_biz.findFriendMode()) : 2U;
        setDropdownIndex(_choice_mode.dropdown, mode);
    }
    if (enabled && _biz.findFriendMode() == 0 && _open_dropdown != _choice_friend.dropdown)
    {
        syncFriendDropdown(force_friend_options);
    }
    if (_choice_team.dropdown && _open_dropdown != _choice_team.dropdown)
    {
        char options[sizeof(_team_options)] = {};
        const auto state = static_cast<JoinState>(team.pair_state);
        const char *hint = state == JoinState::Searching ? "searching..." : "select team";
        switch (state)
        {
        case JoinState::TimedOut: hint = "no team / timeout"; break;
        case JoinState::Rejected: hint = "rejected"; break;
        case JoinState::Error: hint = "join failed"; break;
        case JoinState::WaitingForApproval: hint = "waiting..."; break;
        default: break;
        }
        size_t used = static_cast<size_t>(lv_snprintf(options, sizeof(options), "%s", hint));
        const int count = team.has_team ? 0 : _biz.teamOfferCount();
        for (int index = 0; index < count && index < 4; ++index)
        {
            char label[32] = {};
            if (!_biz.teamOfferLabel(index, label, sizeof(label)))
            {
                lv_snprintf(label, sizeof(label), "Team %d", index + 1);
            }
            const int written = lv_snprintf(options + used, sizeof(options) - used,
                                            "\n%s", label);
            if (written <= 0 || static_cast<size_t>(written) >= sizeof(options) - used)
            {
                break;
            }
            used += static_cast<size_t>(written);
        }
        if (team.has_team)
        {
            UiSendMessageTarget target{};
            if (_biz.teamMessageTarget(-1, target))
            {
                lv_snprintf(options, sizeof(options), "%s\nleave team", target.label);
            }
            else
            {
                lv_snprintf(options, sizeof(options), "team active\nleave team");
            }
        }
        else
        {
            lv_snprintf(options + used, sizeof(options) - used, "\nsearch again");
        }
        if (strcmp(options, _team_options) != 0)
        {
            memcpy(_team_options, options, sizeof(_team_options));
            lv_dropdown_set_options_static(_choice_team.dropdown, _team_options);
            setDropdownIndex(_choice_team.dropdown, 0, true);
        }
        if (_focus_team_when_offer_available && count > 0)
        {
            _focus_team_when_offer_available = false;
            if (lv_async_call(focusTeamDeferred, this) != LV_RES_OK)
            {
                focusTeamDeferred(this);
            }
        }
    }
    updateConditionalVisibility();
    if (_focus_member_when_team_active && team.has_team)
    {
        _focus_member_when_team_active = false;
        if (lv_async_call(focusMemberDeferred, this) != LV_RES_OK)
        {
            focusMemberDeferred(this);
        }
    }
}

void FindFriendScreen::updateConditionalVisibility()
{
    const bool enabled = _biz.findFriendEnabled();
    const bool friend_mode = _biz.findFriendMode() == 0;
    const biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    const auto state = static_cast<JoinState>(team.pair_state);
    const bool team_mode = enabled && friend_mode;
    const bool has_team = team.has_team;
    set_hidden(_row_team, !team_mode);
    if (_row_team && lv_obj_get_child(_row_team, 0))
    {
        const char *text = !has_team ? "> create team" :
            (team.join_window_open ? "> close joining" : "> open joining");
        _lv_obj_t *label = lv_obj_get_child(_row_team, 0);
        if (strcmp(lv_label_get_text(label), text) != 0)
        {
            lv_label_set_text_static(label, text);
        }
    }
    set_hidden(_choice_team.row, !enabled || (!has_team && (!friend_mode || team.is_leader)));
    if (_choice_team.label)
    {
        const char *label = !friend_mode && has_team ? "team sharing" : "team";
        if (strcmp(lv_label_get_text(_choice_team.label), label) != 0)
        {
            lv_label_set_text_static(_choice_team.label, label);
        }
    }
    if (_choice_team.dropdown)
    {
        if (!friend_mode)
        {
            lv_obj_add_state(_choice_team.dropdown, LV_STATE_DISABLED);
        }
        else
        {
            lv_obj_clear_state(_choice_team.dropdown, LV_STATE_DISABLED);
        }
    }
    set_hidden(_choice_mode.row, false);
    set_hidden(_choice_friend.row, !team_mode || !has_team);
    set_hidden(_row_team_message, !team_mode || !has_team);
    if (_row_team_message)
    {
        if (has_team)
            lv_obj_clear_state(_row_team_message, LV_STATE_DISABLED);
        else
            lv_obj_add_state(_row_team_message, LV_STATE_DISABLED);
    }
    set_hidden(_row_wp_manual, !enabled || friend_mode);

    syncFocusGroupMembership();

    if (_open_dropdown && !focus_object_available(_open_dropdown))
        closeOpenDropdown();
    if (group())
    {
        _lv_obj_t *const focused = lv_group_get_focused(group());
        if (focused && !focus_object_available(focused) && _choice_mode.dropdown)
        {
            lv_group_focus_obj(_choice_mode.dropdown);
            scrollFocusedIntoView(_choice_mode.dropdown);
        }
    }
}

void FindFriendScreen::syncFocusGroupMembership()
{
    if (!group())
        return;

    _lv_obj_t *const controls[] = {
        _choice_mode.dropdown,
        _row_team,
        _choice_team.dropdown,
        _choice_friend.dropdown,
        _row_team_message,
        _row_wp_manual,
    };
    uint16_t mask = 0;
    for (size_t index = 0; index < sizeof(controls) / sizeof(controls[0]); ++index)
    {
        if (focus_object_available(controls[index]))
        {
            mask |= static_cast<uint16_t>(1U << index);
        }
    }
    if (mask == _focus_membership_mask)
        return;

    _lv_obj_t *const previous = lv_group_get_focused(group());
    _focus_membership_mask = mask;
    lv_group_set_editing(group(), false);
    lv_group_remove_all_objs(group());
    for (size_t index = 0; index < sizeof(controls) / sizeof(controls[0]); ++index)
    {
        if ((mask & static_cast<uint16_t>(1U << index)) != 0U)
        {
            lv_group_add_obj(group(), controls[index]);
        }
    }
    if (focus_object_available(previous) && lv_obj_get_group(previous) == group())
    {
        lv_group_focus_obj(previous);
    }
}

void FindFriendScreen::onDropdownValueChanged(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
        return;
    auto *self = static_cast<FindFriendScreen *>(lv_event_get_user_data(e));
    if (!self || self->_syncing_dropdown)
        return;
    _lv_obj_t *const dropdown = lv_event_get_target(e);
    if (lv_dropdown_is_open(dropdown))
    {
        lv_dropdown_close(dropdown);
    }

    if (dropdown == self->_choice_mode.dropdown)
    {
        (void)lv_async_call_cancel(focusTeamDeferred, self);
        const int mode = static_cast<int>(lv_dropdown_get_selected(dropdown));
        if (mode == 2)
        {
            self->_focus_team_when_offer_available = false;
            self->_focus_member_when_team_active = false;
            self->_biz.teamCancel();
            if (self->_biz.teamStatus().has_team && !self->_biz.teamLeave())
            {
                self->syncControls(true);
                self->_biz.showAlert("Leave team failed", 2000);
                return;
            }
            self->_biz.setFindFriendTargetContactIndex(-1);
            self->syncFriendDropdown(true);
            (void)self->_biz.setFindFriendEnabled(false);
            self->syncControls(true);
            self->_biz.showAlert("Find friend: OFF", 2000);
            return;
        }
        if (!self->_biz.setFindFriendMode(mode) ||
            !self->_biz.setFindFriendEnabled(true))
        {
            self->syncControls(true);
            self->_biz.showAlert("GPS unavailable", 2000);
            return;
        }
        if (mode == 0)
        {
            self->syncFriendDropdown(true);
#if HELTEC_TEAM_FEATURE
            const biz::IBizFacade::TeamStatus team = self->_biz.teamStatus();
            if (!team.is_leader && !team.has_team)
            {
                self->startTeamSearch();
                return;
            }
#endif
        }
        else
        {
            /* Pairing has no purpose in waypoint mode and must not run invisibly. */
            self->_focus_team_when_offer_available = false;
            self->_focus_member_when_team_active = false;
            self->_biz.teamCancel();
        }
        self->updateConditionalVisibility();
        if (mode == 0 && self->_biz.teamStatus().has_team &&
            !self->_biz.locationShareEnabled())
        {
            self->_biz.showAlert("Find friend; location share is OFF", 2500);
        }
        else
        {
            self->_biz.showAlert(mode == 0 ? "Find friend" : "Find waypoint", 1500);
        }
    }
    else if (dropdown == self->_choice_friend.dropdown)
    {
        self->_biz.setFindFriendTargetContactIndex(self->friendMeshIndexForSelection());
        self->syncFriendDropdown(true);
        self->updateConditionalVisibility();
        self->_biz.showAlert("Friend selected", 2000);
    }
    else if (dropdown == self->_choice_team.dropdown)
    {
        const int selected = static_cast<int>(lv_dropdown_get_selected(dropdown));
        const biz::IBizFacade::TeamStatus team = self->_biz.teamStatus();
        if (!self->_biz.findFriendEnabled() || self->_biz.findFriendMode() != 0)
        {
            return;
        }
        if (team.has_team)
        {
            if (selected != 1)
            {
                self->setDropdownIndex(dropdown, 0, true);
                return;
            }

            self->_focus_team_when_offer_available = false;
            self->_focus_member_when_team_active = false;
            self->_biz.teamCancel();
            if (!self->_biz.teamLeave())
            {
                self->setDropdownIndex(dropdown, 0, true);
                self->_biz.showAlert("Leave team failed", 2000);
                return;
            }

            if (self->_open_dropdown == dropdown)
            {
                self->_open_dropdown = nullptr;
            }
            lv_obj_clear_state(dropdown, LV_STATE_EDITED);
            if (self->group())
            {
                lv_group_set_editing(self->group(), false);
            }
            self->_biz.setFindFriendTargetContactIndex(-1);
            self->_friend_window_start = 0;
            self->syncFriendDropdown(true);
            self->syncControls(true);
            self->_biz.showAlert("Left team", 2000);
            return;
        }
        if (
            static_cast<JoinState>(self->_biz.teamStatus().pair_state) == JoinState::WaitingForApproval)
        {
            return;
        }
        if (selected == lv_dropdown_get_option_cnt(dropdown) - 1)
        {
            self->startTeamSearch();
            return;
        }
        if (selected == 0)
        {
            return;
        }
        if (selected < 0)
            return;
        const bool ok = self->_biz.teamSelectOffer(selected - 1);
        self->_focus_member_when_team_active = ok;
        char status[32] = {};
        if (ok)
            lv_snprintf(status, sizeof(status), "Confirm %04u",
                        static_cast<unsigned>(self->_biz.teamStatus().pairing_code));
        self->_biz.showAlert(ok ? status : "Join request failed", 3000);
        self->setDropdownIndex(dropdown, 0, true);
        self->updateConditionalVisibility();
        if (ok && self->group() && lv_obj_get_group(dropdown) == self->group())
        {
            lv_group_focus_obj(dropdown);
            self->scrollFocusedIntoView(dropdown);
        }
    }
}

void FindFriendScreen::startTeamSearch()
{
    closeOpenDropdown();
    _focus_team_when_offer_available = true;
    _focus_member_when_team_active = false;
    const bool started = _biz.teamStartSearch();
    syncControls(true);
    if (!started)
    {
        _focus_team_when_offer_available = false;
        _biz.showAlert("Team search failed", 2000);
        return;
    }
}

void FindFriendScreen::focusTeamDeferred(void *user_data)
{
    auto *self = static_cast<FindFriendScreen *>(user_data);
    if (!self || !self->_biz.findFriendEnabled() || self->_biz.findFriendMode() != 0 ||
        self->_biz.teamStatus().has_team || !self->_choice_team.dropdown)
    {
        return;
    }
    lv_obj_t *const target = focus_object_available(self->_choice_team.dropdown)
                                 ? self->_choice_team.dropdown
                                 : self->_choice_mode.dropdown;
    if (focus_object_available(target) && lv_obj_get_group(target) == self->group())
    {
        lv_group_focus_obj(target);
        self->scrollFocusedIntoView(self->_choice_team.dropdown);
    }
}

void FindFriendScreen::focusMemberDeferred(void *user_data)
{
    auto *self = static_cast<FindFriendScreen *>(user_data);
    if (!self || !self->_biz.findFriendEnabled() || self->_biz.findFriendMode() != 0 ||
        !self->_biz.teamStatus().has_team || !self->_choice_friend.dropdown)
    {
        return;
    }
    lv_obj_t *const target = self->_choice_friend.dropdown;
    if (focus_object_available(target) && lv_obj_get_group(target) == self->group())
    {
        lv_group_focus_obj(target);
        self->scrollFocusedIntoView(target);
    }
}

void FindFriendScreen::onDropdownStateEvent(lv_event_t *e)
{
    auto *self = static_cast<FindFriendScreen *>(lv_event_get_user_data(e));
    _lv_obj_t *const dropdown = lv_event_get_target(e);
    if (!self ||
        (dropdown != self->_choice_mode.dropdown && dropdown != self->_choice_friend.dropdown &&
         dropdown != self->_choice_team.dropdown))
    {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)
    {
        if (self->_open_dropdown && self->_open_dropdown != dropdown)
        {
            self->closeOpenDropdown();
        }
        self->_open_dropdown = dropdown;
        self->_open_dropdown_original_index = lv_dropdown_get_selected(dropdown);
        if (dropdown == self->_choice_friend.dropdown)
        {
            self->_friend_open_original_mesh_idx = self->friendMeshIndexForSelection();
        }
        lv_obj_add_state(dropdown, LV_STATE_EDITED);
        if (self->group())
            lv_group_set_editing(self->group(), true);
        if (lv_async_call(realignDropdownListAsync, dropdown) != LV_RES_OK)
        {
            self->realignDropdownList(dropdown);
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        if (self->_open_dropdown == dropdown)
            self->_open_dropdown = nullptr;
        lv_obj_clear_state(dropdown, LV_STATE_EDITED);
        if (self->group())
            lv_group_set_editing(self->group(), false);
    }
}

void FindFriendScreen::onDropdownReleasedPre(lv_event_t *e)
{
    auto *self = static_cast<FindFriendScreen *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *const dropdown = lv_event_get_target(e);
    if (self->_suppress_released_dropdown == dropdown)
    {
        self->_suppress_released_dropdown = nullptr;
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    if (dropdown == self->_choice_team.dropdown &&
        static_cast<JoinState>(self->_biz.teamStatus().pair_state) ==
            JoinState::WaitingForApproval)
    {
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    self->realignDropdownList(dropdown);
}

void FindFriendScreen::syncFriendDropdown(bool force)
{
    if (!_choice_friend.dropdown)
        return;
    int total = 0;
    int selected_rank = -1;
    (void)_biz.fillFindFriendContacts(_friend_window_start, _biz.findFriendTargetContactIndex(),
                                      nullptr, 0, &total, &selected_rank);

    const int max_start = total > kFriendWindowSize ? total - kFriendWindowSize : 0;
    int start = _friend_window_start;
    if (start > max_start)
        start = max_start;
    if (selected_rank >= 0)
    {
        const int local = selected_rank - start;
        if (local < 0 || local >= kFriendWindowSize)
        {
            start = (selected_rank / kFriendWindowStep) * kFriendWindowStep;
        }
        else if (local >= kFriendWindowSize - 2 && start < max_start)
        {
            start += kFriendWindowStep;
        }
        else if (local <= 1 && start > 0)
        {
            start -= kFriendWindowStep;
        }
    }
    if (start < 0)
        start = 0;
    if (start > max_start)
        start = max_start;
    if (!force && _friend_mesh_map_count_applied >= 0 && start == _friend_window_start &&
        selected_rank == _friend_selected_rank && total == _friend_total)
        return;
    loadFriendDropdownWindow(start, selected_rank, force);
}

void FindFriendScreen::loadFriendDropdownWindow(int start, int selected_rank, bool force)
{
    if (!_choice_friend.dropdown)
        return;
    biz::IBizFacade::FindFriendContactItem items[kFriendWindowSize]{};
    int total = 0;
    _friend_mesh_map_count =
        _biz.fillFindFriendContacts(start, _biz.findFriendTargetContactIndex(), items,
                                    kFriendWindowSize, &total, nullptr);
    _friend_total = total;
    const int max_start = total > kFriendWindowSize ? total - kFriendWindowSize : 0;
    if (start < 0)
        start = 0;
    if (start > max_start)
        start = max_start;
    _friend_window_start = start;
    _friend_selected_rank = selected_rank;

    lv_snprintf(_friend_options, sizeof(_friend_options), "(none)");
    for (int i = 0; i < _friend_mesh_map_count; ++i)
    {
        _friend_mesh_map[i] = items[i].contact_index;
        const size_t used = strlen(_friend_options);
        if (used >= sizeof(_friend_options) - 1)
            break;
        lv_snprintf(_friend_options + used, sizeof(_friend_options) - used, "\n%s",
                    items[i].label[0] ? items[i].label : "?");
    }

    const uint32_t hash = options_hash(_friend_options);
    const bool changed = force || _friend_mesh_map_count != _friend_mesh_map_count_applied ||
                         hash != _friend_options_hash;
    if (changed)
    {
        _friend_options_hash = hash;
        _friend_mesh_map_count_applied = _friend_mesh_map_count;
        lv_dropdown_set_options_static(_choice_friend.dropdown, _friend_options);
    }

    int local_selection = 0;
    if (selected_rank >= _friend_window_start &&
        selected_rank < _friend_window_start + _friend_mesh_map_count)
    {
        local_selection = selected_rank - _friend_window_start + 1;
    }
    setDropdownIndex(_choice_friend.dropdown, static_cast<uint16_t>(local_selection), changed);
}

int FindFriendScreen::friendMeshIndexForSelection() const
{
    if (!_choice_friend.dropdown)
        return -1;
    const int selected = static_cast<int>(lv_dropdown_get_selected(_choice_friend.dropdown));
    if (selected <= 0 || selected - 1 >= _friend_mesh_map_count)
        return -1;
    return _friend_mesh_map[selected - 1];
}

bool FindFriendScreen::moveFriendDropdownSelection(int direction)
{
    if (!_choice_friend.dropdown || direction == 0 || _friend_total <= 0)
        return false;
    const int selected = static_cast<int>(lv_dropdown_get_selected(_choice_friend.dropdown));
    int position = 0;
    if (selected > 0 && selected - 1 < _friend_mesh_map_count)
    {
        position = _friend_window_start + selected;
    }

    const int option_total = _friend_total + 1;
    position = (position + (direction > 0 ? 1 : -1) + option_total) % option_total;
    const int rank = position - 1;
    if (rank < 0)
    {
        _friend_selected_rank = -1;
        setDropdownIndex(_choice_friend.dropdown, 0, true);
        return true;
    }

    int start = _friend_window_start;
    const int max_start = _friend_total > kFriendWindowSize ? _friend_total - kFriendWindowSize : 0;
    const int local = rank - start;
    if (rank == 0 && direction > 0)
    {
        start = 0;
    }
    else if (rank == _friend_total - 1 && direction < 0)
    {
        start = max_start;
    }
    else if ((local < 0 || (direction < 0 && local <= 1)) && start > 0)
    {
        start -= kFriendWindowStep;
    }
    else if ((local >= kFriendWindowSize || (direction > 0 && local >= kFriendWindowSize - 2)) &&
             start < max_start)
    {
        start += kFriendWindowStep;
    }
    if (start < 0)
        start = 0;
    if (start > max_start)
        start = max_start;

    _friend_selected_rank = rank;
    if (start != _friend_window_start)
    {
        loadFriendDropdownWindow(start, rank, true);
    }
    else
    {
        setDropdownIndex(_choice_friend.dropdown,
                         static_cast<uint16_t>(rank - _friend_window_start + 1), true);
    }
    return true;
}

void FindFriendScreen::closeOpenDropdown()
{
    if (!_open_dropdown)
        return;
    _lv_obj_t *const closing = _open_dropdown;
    _open_dropdown = nullptr;
    if (lv_obj_is_valid(closing) && lv_dropdown_is_open(closing))
    {
        if (closing == _choice_friend.dropdown)
        {
            syncFriendDropdown(true);
        }
        else
        {
            setDropdownIndex(closing, _open_dropdown_original_index, true);
        }
        lv_dropdown_close(closing);
    }
    if (lv_obj_is_valid(closing))
        lv_obj_clear_state(closing, LV_STATE_EDITED);
    if (group())
        lv_group_set_editing(group(), false);
}

void FindFriendScreen::realignDropdownListAsync(void *user_data)
{
    _lv_obj_t *const dropdown = static_cast<_lv_obj_t *>(user_data);
    if (!dropdown || !lv_obj_is_valid(dropdown))
        return;
    for (_lv_obj_t *obj = dropdown; obj; obj = lv_obj_get_parent(obj))
    {
        if (!lv_obj_get_user_data(obj))
            continue;
        auto *self = static_cast<FindFriendScreen *>(lv_obj_get_user_data(obj));
        if (self)
            self->realignDropdownList(dropdown);
        return;
    }
}

void FindFriendScreen::realignDropdownList(_lv_obj_t *dropdown)
{
    if (!dropdown || !_root || !lv_obj_is_valid(dropdown) || !lv_dropdown_is_open(dropdown))
    {
        return;
    }
    _lv_obj_t *const list = lv_dropdown_get_list(dropdown);
    if (!list || !lv_obj_is_valid(list))
        return;

    lv_obj_update_layout(_root);
    lv_obj_update_layout(lv_obj_get_parent(dropdown));
    lv_obj_update_layout(dropdown);
    const lv_coord_t width = lv_obj_get_width(dropdown);
    if (width > 0)
        lv_obj_set_width(list, width);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    ui_theme_match_dropdown_list_padding(dropdown, list);
    _lv_obj_t *const viewport = lv_obj_get_parent(_root);
    // Position from the anchor's current screen coordinates; do not scroll an
    // unrelated parent while fitting the popup (which can move the anchor again).
    ui_dropdown_fit_list_to_viewport(dropdown, viewport ? viewport : _root, nullptr);
}

void FindFriendScreen::onControlKeyPreprocess(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY)
        return;
    auto *self = static_cast<FindFriendScreen *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *const target = lv_event_get_target(e);
    const uint32_t key = lv_event_get_key(e);
    const bool is_dropdown = target == self->_choice_mode.dropdown ||
                             target == self->_choice_friend.dropdown ||
                             target == self->_choice_team.dropdown;
    if (is_dropdown)
    {
    }

    if (target == self->_choice_team.dropdown && key == LV_KEY_ENTER &&
        static_cast<JoinState>(self->_biz.teamStatus().pair_state) ==
            JoinState::WaitingForApproval)
    {
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }

    /* Recover the open-list state if keyboard input precedes the READY handler. */
    if (is_dropdown && lv_dropdown_is_open(target) && self->_open_dropdown != target)
    {
        if (self->_open_dropdown)
            self->closeOpenDropdown();
        self->_open_dropdown = target;
        self->_open_dropdown_original_index = lv_dropdown_get_selected(target);
        if (target == self->_choice_friend.dropdown)
            self->_friend_open_original_mesh_idx = self->friendMeshIndexForSelection();
        lv_obj_add_state(target, LV_STATE_EDITED);
        if (self->group())
            lv_group_set_editing(self->group(), true);
    }

    if (target == self->_choice_friend.dropdown && self->_open_dropdown == target &&
        lv_dropdown_is_open(target))
    {
        int direction = 0;
        if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT)
            direction = 1;
        if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV)
            direction = -1;
        if (direction != 0 && self->moveFriendDropdownSelection(direction))
        {
            if (lv_async_call(realignDropdownListAsync, target) != LV_RES_OK)
                self->realignDropdownList(target);
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
            return;
        }
    }

    if (self->_open_dropdown == target && cycle_dropdown(target, key))
    {
        if (lv_async_call(realignDropdownListAsync, target) != LV_RES_OK)
            self->realignDropdownList(target);
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }

    if (key == LV_KEY_ENTER && self->_open_dropdown == target && lv_dropdown_is_open(target))
    {
        bool changed = lv_dropdown_get_selected(target) != self->_open_dropdown_original_index;
        if (target == self->_choice_friend.dropdown)
        {
            changed = self->friendMeshIndexForSelection() != self->_friend_open_original_mesh_idx;
        }
        self->_open_dropdown = nullptr;
#if defined(HELTEC_T1)
        /* T1 long-press emits KEY followed by RELEASED. Without suppressing that
           release, LVGL closes here and immediately reopens the dropdown. */
        self->_suppress_released_dropdown = target;
#endif
        lv_dropdown_close(target);
        lv_obj_clear_state(target, LV_STATE_EDITED);
        if (self->group())
            lv_group_set_editing(self->group(), false);
        if (changed)
            lv_event_send(target, LV_EVENT_VALUE_CHANGED, nullptr);
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }

    if (is_dropdown && key == LV_KEY_ESC && self->_open_dropdown == target)
    {
        self->closeOpenDropdown();
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    if (is_dropdown && !self->_open_dropdown)
    {
        if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT)
        {
            if (self->group())
                lv_group_focus_next(self->group());
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
            return;
        }
        if (key == LV_KEY_UP || key == LV_KEY_LEFT)
        {
            if (self->group())
                lv_group_focus_prev(self->group());
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
            return;
        }
    }

    if (key != LV_KEY_ESC)
        return;
    if (self->_open_dropdown)
        self->closeOpenDropdown();
    (void)self->onKey(LV_KEY_ESC);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

void FindFriendScreen::handleAction(Action action, _lv_obj_t *source)
{
    if (action == Action::TeamMessage)
    {
        UiSendMessageTarget target{};
        const int member = _biz.findFriendTargetContactIndex();
        /* A selected member defaults to Personal; -1 defaults to the Team group. */
        if (!_biz.teamMessageTarget(member, target))
        {
            _biz.showAlert("No Team recipient", 2000);
            return;
        }
        (void)emitEvent(EventType::SendMessageOpen, &target);
        return;
    }
    if (action == Action::Team)
    {
        if (!_biz.teamStatus().is_leader)
        {
            startTeamSearch();
            return;
        }
        const biz::IBizFacade::TeamStatus team = _biz.teamStatus();
        const bool ok = !team.has_team ? _biz.teamSetActive(true) :
                       _biz.teamSetJoining(!team.join_window_open);
        syncControls(true);
        if (!ok)
        {
            _biz.showAlert("Team action failed", 2000);
        }
        return;
    }
    if (action == Action::WaypointManual)
    {
        _waypoint_keyboard_return_focus = source;
        emitEvent(EventType::WaypointKeyboardOpen);
    }
}

void FindFriendScreen::onActionClicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    auto *self = static_cast<FindFriendScreen *>(lv_event_get_user_data(e));
    _lv_obj_t *const target = lv_event_get_target(e);
    if (!self || !target)
        return;
    Action action = Action::None;
    if (target == self->_row_team)
        action = Action::Team;
    if (target == self->_row_team_message)
        action = Action::TeamMessage;
    if (target == self->_row_wp_manual)
        action = Action::WaypointManual;
    if (action == Action::None)
        return;
    self->handleAction(action, target);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
}

void FindFriendScreen::restoreWaypointKeyboardFocus()
{
    _lv_obj_t *const target = _waypoint_keyboard_return_focus;
    _waypoint_keyboard_return_focus = nullptr;
    if (!target || !lv_obj_is_valid(target) || hidden_in_tree(target) || !group() ||
        lv_obj_get_group(target) != group())
    {
        return;
    }
    lv_group_focus_obj(target);
    scrollFocusedIntoView(target);
}

} // namespace heltec::meshcore::ui

#endif // ENV_INCLUDE_COMPASS
