#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

#include "find_friend_screen.hpp"
#include "app/team_join_service.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_container.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ht_event.h"
#include "ui/theme/ui_widget_theme.hpp"
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
    if (action == Action::WaypointGps)
        _row_wp_gps = row;
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
        lv_obj_add_event_cb(
            _right_column,
            +[](lv_event_t *event)
            {
                auto *self = static_cast<FindFriendScreen *>(lv_event_get_user_data(event));
                if (self && self->_open_dropdown)
                {
                    if (!defer(realignDropdownListAsync, self->_open_dropdown))
                        self->realignDropdownList(self->_open_dropdown);
                }
            },
            LV_EVENT_SCROLL, this);
    }
    addDropdownRow(_choice_mode, "mode", "friend\nwaypoint\nnone");
#if HELTEC_TEAM_FEATURE
    if (_biz.teamIsLeader())
    {
        _row_team = addActionRow("> create team", Action::Team);
    }
    addDropdownRow(_choice_team, "team", "(none)");
#endif
    addDropdownRow(_choice_friend, "member", "(none)");
#if HELTEC_TEAM_FEATURE
    _row_group_message = addActionRow("> message team", Action::TeamGroupMessage);
    _row_direct_message = addActionRow("> message friend", Action::TeamDirectMessage);
#endif
    addActionRow("> use current gps", Action::WaypointGps);
    addActionRow("> enter lat,lon", Action::WaypointManual);
}

void FindFriendScreen::configureFocusItems()
{
    /* Keep keypad/encoder traversal identical to the visual row order. */
    addFocusItem(_choice_mode.dropdown, _choice_mode.row);
    addFocusItem(_row_team);
    addFocusItem(_choice_team.dropdown, _choice_team.row);
    addFocusItem(_choice_friend.dropdown, _choice_friend.row);
    addFocusItem(_row_group_message);
    addFocusItem(_row_direct_message);
    addFocusItem(_row_wp_gps);
    addFocusItem(_row_wp_manual);
    if (_lbl_info)
    {
        addFocusItem(_lbl_info, nullptr, false);
        // The read-only row takes part in keypad focus scrolling without intercepting
        // touch gestures intended to scroll the right column.
        lv_obj_clear_flag(_lbl_info, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    }

    // Match System Screen's deterministic focus scrolling. Pointer focus must
    // not snap the column after a drag or tap; keypad/encoder traversal owns it.
    _lv_obj_t *const focus_controls[] = {
        _row_team, _choice_team.dropdown,
        _choice_mode.dropdown,
        _choice_friend.dropdown,
        _row_group_message, _row_direct_message,
        _row_wp_gps, _row_wp_manual,        _lbl_info,
    };
    for (_lv_obj_t *control : focus_controls)
    {
        if (control)
            lv_obj_clear_flag(control, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    }

    lv_obj_set_user_data(_root, this);
    if (group())
    {
        lv_group_set_focus_cb(
            group(),
            +[](lv_group_t *focus_group)
            {
                _lv_obj_t *const focused = lv_group_get_focused(focus_group);
                if (!focused)
                    return;
                for (_lv_obj_t *obj = focused; obj; obj = lv_obj_get_parent(obj))
                {
                    if (!lv_obj_get_user_data(obj))
                        continue;
                    auto *const self = static_cast<FindFriendScreen *>(lv_obj_get_user_data(obj));
                    if (self)
                        self->applyGroupFocus(focused);
                    return;
                }
            });
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
    if (_open_dropdown != _choice_mode.dropdown)
    {
        const uint16_t mode = enabled ? static_cast<uint16_t>(_biz.findFriendMode()) : 2U;
        setDropdownIndex(_choice_mode.dropdown, mode);
    }
    if (enabled && _biz.findFriendMode() == 0 && _open_dropdown != _choice_friend.dropdown)
    {
        _biz.syncFindFriendContactList();
        syncFriendDropdown(force_friend_options);
    }
    if (_choice_team.dropdown && _open_dropdown != _choice_team.dropdown)
    {
        char options[sizeof(_team_options)] = {};
        const auto state = static_cast<JoinState>(_biz.teamPairState());
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
        const int count = _biz.teamHasTeam() ? 0 : _biz.teamOfferCount();
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
        if (_biz.teamHasTeam())
        {
            UiSendMessageTarget target{};
            if (_biz.teamMessageTarget(-1, target))
            {
                lv_snprintf(options, sizeof(options), "%s", target.label);
            }
            else
            {
                lv_snprintf(options, sizeof(options), "team active");
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
    }
    updateConditionalVisibility();
}

void FindFriendScreen::updateConditionalVisibility()
{
    const bool enabled = _biz.findFriendEnabled();
    const bool friend_mode = _biz.findFriendMode() == 0;
    const auto state = static_cast<JoinState>(_biz.teamPairState());
    const bool team_mode = enabled && friend_mode;
    const bool has_team = _biz.teamHasTeam();
    set_hidden(_row_team, !team_mode);
    if (_row_team && lv_obj_get_child(_row_team, 0))
    {
        const char *text = !has_team ? "> create team" :
            (_biz.teamJoinWindowOpen() ? "> close joining" : "> open joining");
        _lv_obj_t *label = lv_obj_get_child(_row_team, 0);
        if (strcmp(lv_label_get_text(label), text) != 0)
        {
            lv_label_set_text_static(label, text);
        }
    }
    set_hidden(_choice_team.row, !enabled || (!has_team && (!friend_mode || _biz.teamIsLeader())));
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
        if (has_team || !friend_mode || state == JoinState::WaitingForApproval)
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
    set_hidden(_row_group_message, !team_mode || !_biz.teamHasTeam());
    set_hidden(_row_direct_message, !team_mode || !_biz.teamHasTeam());
    const bool can_message = _biz.teamHasTeam();
    if (_row_group_message)
    {
        if (can_message)
            lv_obj_clear_state(_row_group_message, LV_STATE_DISABLED);
        else
            lv_obj_add_state(_row_group_message, LV_STATE_DISABLED);
    }
    if (_row_direct_message)
    {
        if (can_message && _biz.findFriendTargetContactIndex() >= 0)
            lv_obj_clear_state(_row_direct_message, LV_STATE_DISABLED);
        else
            lv_obj_add_state(_row_direct_message, LV_STATE_DISABLED);
    }
    set_hidden(_row_wp_gps, !enabled || friend_mode);
    set_hidden(_row_wp_manual, !enabled || friend_mode);

    if (_open_dropdown && !focusObjectAvailable(_open_dropdown))
        closeOpenDropdown();
    if (group())
    {
        _lv_obj_t *const focused = lv_group_get_focused(group());
        if (focused && !focusObjectAvailable(focused) && _choice_mode.dropdown)
        {
            lv_group_focus_obj(_choice_mode.dropdown);
            scrollFocusedIntoView(_choice_mode.dropdown);
        }
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

    if (dropdown == self->_choice_mode.dropdown)
    {
        (void)defer_cancel(focusTeamDeferred, self);
        const int mode = static_cast<int>(lv_dropdown_get_selected(dropdown));
        if (mode == 2)
        {
            self->_biz.teamCancel();
            if (self->_biz.teamHasTeam() && !self->_biz.teamLeave())
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
            if (!self->_biz.teamIsLeader() && !self->_biz.teamHasTeam())
            {
                self->startTeamSearch();
                return;
            }
#endif
        }
        else
        {
            /* Pairing has no purpose in waypoint mode and must not run invisibly. */
            self->_biz.teamCancel();
        }
        self->updateConditionalVisibility();
        if (mode == 0 && self->_biz.teamHasTeam() &&
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
        if (!self->_biz.findFriendEnabled() || self->_biz.findFriendMode() != 0 ||
            self->_biz.teamHasTeam() ||
            static_cast<JoinState>(self->_biz.teamPairState()) == JoinState::WaitingForApproval)
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
        char status[32] = {};
        if (ok)
            lv_snprintf(status, sizeof(status), "Confirm %04u",
                        static_cast<unsigned>(self->_biz.teamPairingCode()));
        self->_biz.showAlert(ok ? status : "Join request failed", 3000);
        self->setDropdownIndex(dropdown, 0, true);
        self->updateConditionalVisibility();
    }
}

void FindFriendScreen::startTeamSearch()
{
    closeOpenDropdown();
    const bool started = _biz.teamStartSearch();
    syncControls(true);
    if (!started)
    {
        _biz.showAlert("Team search failed", 2000);
        return;
    }
    if (!defer(focusTeamDeferred, this))
    {
        focusTeamDeferred(this);
    }
}

void FindFriendScreen::focusTeamDeferred(void *user_data)
{
    auto *self = static_cast<FindFriendScreen *>(user_data);
    if (!self || !self->_biz.findFriendEnabled() || self->_biz.findFriendMode() != 0 ||
        self->_biz.teamHasTeam() || !self->_choice_team.dropdown)
    {
        return;
    }
    if (self->restoreFocus(self->_choice_team.dropdown, self->_choice_mode.dropdown))
    {
        self->scrollFocusedIntoView(self->_choice_team.dropdown);
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
        if (!defer(realignDropdownListAsync, dropdown))
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
    if (self)
        self->realignDropdownList(lv_event_get_target(e));
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
    _lv_obj_t *const viewport = tile();
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
            if (!defer(realignDropdownListAsync, target))
                self->realignDropdownList(target);
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
            return;
        }
    }

    if (self->_open_dropdown == target && cycle_dropdown(target, key))
    {
        if (!defer(realignDropdownListAsync, target))
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
    if (action == Action::TeamGroupMessage || action == Action::TeamDirectMessage)
    {
        UiSendMessageTarget target{};
        const int member = action == Action::TeamGroupMessage ? -1 :
                           _biz.findFriendTargetContactIndex();
        // -1 denotes Group, not an unselected personal recipient.
        if ((action == Action::TeamDirectMessage && member < 0) ||
            !_biz.teamMessageTarget(member, target))
        {
            _biz.showAlert("No Team recipient", 2000);
            return;
        }
        (void)emit(EventType::SEND_MESSAGE_OPEN, &target);
        return;
    }
    if (action == Action::Team)
    {
        if (!_biz.teamIsLeader())
        {
            startTeamSearch();
            return;
        }
        const bool ok = !_biz.teamHasTeam() ? _biz.teamSetActive(true) :
                       _biz.teamSetJoining(!_biz.teamJoinWindowOpen());
        syncControls(true);
        if (!ok)
        {
            _biz.showAlert("Team action failed", 2000);
        }
        return;
    }
    if (action == Action::WaypointGps)
    {
        const biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
        if (gps.fix_valid && _biz.setFindFriendWaypoint(gps.lat_deg, gps.lon_deg))
        {
            char text[48];
            _biz.formatFindFriendWaypointInput(text, sizeof(text));
            _biz.showAlert(text[0] ? text : "Saved", 3000);
        }
        else
        {
            _biz.showAlert("Need GPS fix", 3000);
        }
        return;
    }
    if (action == Action::WaypointManual)
    {
        _waypoint_keyboard_return_focus = source;
        emit(EventType::WAYPOINT_KEYBOARD_OPEN);
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
    if (target == self->_row_group_message)
        action = Action::TeamGroupMessage;
    if (target == self->_row_direct_message)
        action = Action::TeamDirectMessage;
    if (target == self->_row_wp_gps)
        action = Action::WaypointGps;
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
