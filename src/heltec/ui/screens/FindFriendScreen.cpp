#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
#include "FindFriendScreen.hpp"
#include "app/TeamJoinService.hpp"
#include "CompassCommon.hpp"
#include "ui/core/UiMessage.hpp"

#include <string.h>

#ifndef FF_INFO_COL_WIDTH_PCT
#define FF_INFO_COL_WIDTH_PCT 40
#endif

#ifndef FF_INFO_COL_MARGIN_RIGHT
#define FF_INFO_COL_MARGIN_RIGHT 6
#endif

#ifndef FF_INFO_COL_PAD_LEFT
#define FF_INFO_COL_PAD_LEFT 2
#endif

#ifndef DISPLAY_ORIENTATION_PORTRAIT
#define DISPLAY_ORIENTATION_PORTRAIT 0
#endif

namespace Heltec::MeshCore::Ui {

namespace {
#if DISPLAY_ORIENTATION_PORTRAIT
constexpr lv_coord_t kTouchFindFriendDialHeight = 144;
constexpr lv_coord_t kTouchFindFriendDialEdgeMargin = 16;
#endif

void set_static_text(lv_obj_t *label, char *buffer, size_t capacity, const char *text) {
    if (!label || !buffer || capacity == 0) return;
    const char *value = text ? text : "";
    if (strncmp(buffer, value, capacity) == 0) return;
    lv_snprintf(buffer, capacity, "%s", value);
    lv_label_set_text_static(label, buffer);
}

void format_age_short(char *buf, size_t len, bool known, uint32_t age_ms) {
    if (!buf || len == 0) return;
    if (!known) {
        lv_snprintf(buf, len, "?");
        return;
    }
    const uint32_t seconds = age_ms / 1000U;
    if (seconds < 60U) {
        lv_snprintf(buf, len, "%lus", (unsigned long)seconds);
    } else if (seconds < 3600U) {
        lv_snprintf(buf, len, "%lum", (unsigned long)(seconds / 60U));
    } else {
        lv_snprintf(buf, len, "%luh", (unsigned long)(seconds / 3600U));
    }
}

void format_nav_distance(char *buf, size_t len, const Biz::FindFriendUi &u) {
    if (!buf || len == 0) return;
    if (u.distance_m < 0.0) {
        lv_snprintf(buf, len, "--");
        return;
    }
    const char *approximate = u.confidence == Biz::FindFriendConfidence::Low ? "~" : "";
    if (u.distance_m >= 10000.0) {
        const uint32_t km = static_cast<uint32_t>(u.distance_m / 1000.0 + 0.5);
        lv_snprintf(buf, len, "%s%lukm", approximate, (unsigned long)km);
        return;
    }
    if (u.distance_m >= 1000.0) {
        const uint32_t km_tenths = static_cast<uint32_t>(u.distance_m / 100.0 + 0.5);
        lv_snprintf(buf, len, "%s%lu.%lukm", approximate, (unsigned long)(km_tenths / 10U),
                    (unsigned long)(km_tenths % 10U));
        return;
    }

    uint32_t step_m = 1;
    if (u.estimated_accuracy_m >= 40.0f) {
        step_m = 10;
    } else if (u.estimated_accuracy_m >= 15.0f) {
        step_m = 5;
    }
    const uint32_t meters = static_cast<uint32_t>(u.distance_m + 0.5);
    const uint32_t rounded = ((meters + step_m / 2U) / step_m) * step_m;
    lv_snprintf(buf, len, "%s%lum", approximate, (unsigned long)rounded);
}

void format_arrival_distance(char *buf, size_t len, const Biz::FindFriendUi &u) {
    if (!buf || len == 0) return;
    char distance[16] = {};
    format_nav_distance(distance, sizeof(distance), u);
    lv_snprintf(buf, len, "arrived %s", distance);
}

void append_status_line(char *buf, size_t len, const char *text) {
    if (!buf || len == 0 || !text || text[0] == '\0') return;

    const size_t used = strlen(buf);
    if (used >= len - 1) return;

    lv_snprintf(buf + used, len - used, "%s%s", used == 0 ? "" : "\n", text);
}
} // namespace

void FindFriendScreen::setInfoText(const char *info) {
    set_static_text(_lbl_info, _info_text, sizeof(_info_text), info);
    if (!_lbl_info) return;
    if (_info_text[0]) {
        lv_obj_clear_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
    }
}

void FindFriendScreen::setStatusText(const char *status) {
    set_static_text(_lbl_status, _status_text, sizeof(_status_text), status);
    if (!_lbl_status) return;
    if (_status_text[0]) {
        lv_obj_clear_flag(_lbl_status, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_lbl_status, LV_OBJ_FLAG_HIDDEN);
    }
}

void FindFriendScreen::showInfoOnly(const char *info) {
    _dial.setVisible(true);
    _dial.setFriendNeedle(0, false, false, false, false);
    _direction_valid = false;
    _near_target = false;
    _on_target = false;
    _gps_fix = false;
    setInfoText(info);
}

void FindFriendScreen::showHeadingOnly(const Biz::FindFriendUi &ui, const char *info) {
    _dial.setVisible(true);
    _dial.setFriendNeedle(0, false, false, false, false);
    _direction_valid = false;
    _near_target = false;
    _on_target = false;
    _gps_fix = false;
    if (ui.heading_valid) {
        const float heading = compass_screen_heading(ui.heading_deg, 0);
        const int16_t ring =
            compass_dial_redraw_tenths((int16_t)compass_heading_tenths(heading));
        if (ring != _ring_heading_tenths) {
            _ring_heading_tenths = ring;
            _dial.setRingHeading(ring);
        }
    } else if (_ring_heading_tenths != 0) {
        _ring_heading_tenths = 0;
        _dial.setRingHeading(0);
    }
    setInfoText(info);
}

void FindFriendScreen::onEnter() {
    AbstractScreen::onEnter();
    if (_right_column) lv_obj_scroll_to_y(_right_column, 0, LV_ANIM_OFF);
    _biz.setFindFriendForegroundActive(true);
    closeOpenDropdown();
    _suppress_released_dropdown = nullptr;
    syncControls(true);
    _defer_cycle_target = _biz.findFriendEnabled();

    layoutDialColumns();

    render(_biz.findFriendUi());
}

bool FindFriendScreen::build() {
    if (!AbstractScreen::build()) return false;

    if (!createDial()) return false;
    // Mode selection owns enable/disable state. One runtime row shows either
    // an actionable state or the current distance.
    createControls();
    createInfoRows();
    configureFocusItems();
    syncControls(true);

    return true;
}

bool FindFriendScreen::createDial() {
    _dial_row = ht_class_create(&ht_find_friend_dial_host_class, _root);
    if (!_dial_row) return false;
    // Keep the dial fixed in the left column. Only the settings/info column on
    // the right scrolls when its rows no longer fit on screen.
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(_dial_row, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(_dial_row, 1);
    compass_init_dial_row(_dial_row, (lv_coord_t)FF_INFO_COL_MARGIN_RIGHT);
#if DISPLAY_ORIENTATION_PORTRAIT
    lv_obj_set_flex_flow(_dial_row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_dial_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_right(_dial_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(_dial_row, LV_DPX(4), LV_PART_MAIN);
#endif

    _dial.min_side = 16;
#if DISPLAY_ORIENTATION_PORTRAIT
    // Reserve enough space for the cardinal labels drawn outside the dial.
    _dial.edge_margin = kTouchFindFriendDialEdgeMargin;
#endif
    _dial.needle_kind = CompassDialNeedleKind::FriendTurn;
    if (!_dial.create(_dial_row)) return false;
    // The left column is a square matching the available content height. The
    // right column receives every remaining horizontal pixel.
#if DISPLAY_ORIENTATION_PORTRAIT
    lv_obj_set_size(_dial.center_col, lv_pct(100), kTouchFindFriendDialHeight);
#else
    lv_obj_set_width(_dial.center_col, _dial.min_side);
#endif
    lv_obj_set_flex_grow(_dial.center_col, 0);

    _right_column = compass_create_info_column(_dial_row, (lv_coord_t)FF_INFO_COL_WIDTH_PCT,
                                               (lv_coord_t)FF_INFO_COL_PAD_LEFT,
                                               0
    );
    if (!_right_column) return false;
#if DISPLAY_ORIENTATION_PORTRAIT
    lv_obj_set_size(_right_column, lv_pct(100), 0);
    lv_obj_set_flex_grow(_right_column, 1);
    lv_obj_set_style_pad_all(_right_column, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(_right_column, LV_DPX(4), LV_PART_MAIN);
    lv_obj_set_style_pad_row(_right_column, LV_DPX(2), LV_PART_MAIN);
#else
    lv_obj_set_width(_right_column, 0);
    lv_obj_set_flex_grow(_right_column, 1);
#endif
    lv_obj_clear_flag(_right_column, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_flag(_right_column, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_right_column, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_right_column, LV_SCROLLBAR_MODE_AUTO);
    return true;
}

void FindFriendScreen::layoutDialColumns() {
    if (!_dial_row || !_dial.center_col || !_right_column) return;

    lv_obj_update_layout(_dial_row);
#if DISPLAY_ORIENTATION_PORTRAIT
    if (lv_obj_get_height(_dial.center_col) != kTouchFindFriendDialHeight) {
        lv_obj_set_height(_dial.center_col, kTouchFindFriendDialHeight);
        lv_obj_update_layout(_dial_row);
    }
#else
    const lv_coord_t content_width = lv_obj_get_content_width(_dial_row);
    const lv_coord_t content_height = lv_obj_get_content_height(_dial_row);
    const lv_coord_t column_gap = lv_obj_get_style_pad_column(_dial_row, LV_PART_MAIN);
    const lv_coord_t usable_width = content_width > column_gap ?
        static_cast<lv_coord_t>(content_width - column_gap) : content_width;
    const lv_coord_t max_square_width = static_cast<lv_coord_t>(usable_width / 2);
    const lv_coord_t dial_column_width = LV_MIN(content_height, max_square_width);
    if (dial_column_width >= _dial.min_side &&
        lv_obj_get_width(_dial.center_col) != dial_column_width) {
        lv_obj_set_width(_dial.center_col, dial_column_width);
        lv_obj_update_layout(_dial_row);
    }
#endif
    _dial.layoutSize();
}

void FindFriendScreen::createInfoRows() {
    if (!_right_column) return;

#if DISPLAY_ORIENTATION_PORTRAIT
    lv_obj_t *const spacer = lv_obj_create(_right_column);
    if (spacer)
    {
        lv_obj_remove_style_all(spacer);
        lv_obj_set_size(spacer, lv_pct(100), 0);
        lv_obj_set_flex_grow(spacer, 1);
        lv_obj_clear_flag(spacer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
#endif

    _lbl_info = ht_label_create(&ht_compass_label_class, _right_column);
    compass_style_info_label(_lbl_info, "starting...", LV_LABEL_LONG_CLIP);
    if (_lbl_info) {
        lv_label_set_text_static(_lbl_info, _info_text);
    }

    _lbl_status = ht_label_create(&ht_compass_label_class, _right_column);
    compass_style_info_label(_lbl_status, "", LV_LABEL_LONG_WRAP);
    if (_lbl_status) lv_label_set_text_static(_lbl_status, _status_text);
}

void FindFriendScreen::render(const Biz::FindFriendUi &u) {
    _dial.setCenterMarkerVisible(false);

    const Biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    const auto team_state =
        static_cast<Heltec::MeshCore::Team::JoinState>(team.pair_state);
    if (_biz.findFriendEnabled() && _biz.findFriendMode() == 0 && !team.has_team) {
        if (team_state == Heltec::MeshCore::Team::JoinState::Searching) {
            setStatusText("Searching Team...");
            showHeadingOnly(u, "Waiting for Team offers");
            return;
        }
        if (team_state == Heltec::MeshCore::Team::JoinState::TimedOut ||
            team_state == Heltec::MeshCore::Team::JoinState::Error) {
            setStatusText("Team search timed out");
            showHeadingOnly(u, "Select search again");
            return;
        }
    }
    if (static_cast<Heltec::MeshCore::Team::JoinState>(team.pair_state) ==
        Heltec::MeshCore::Team::JoinState::WaitingForApproval) {
        char code[24] = {};
        lv_snprintf(code, sizeof(code), "code %04u", static_cast<unsigned>(team.pairing_code));
        setStatusText("Waiting for Leader");
        showHeadingOnly(u, code);
        return;
    }
    if (!_biz.findFriendEnabled()) {
        setStatusText("");
        showHeadingOnly(u, "");
        return;
    }

    char status[sizeof(_status_text)] = {};
    if (u.mode == 0) {
        char gps_age[12] = {};
        char gps_summary[28] = {};
        format_age_short(gps_age, sizeof(gps_age), u.gps_fix, u.gps_age_ms);
        lv_snprintf(gps_summary, sizeof(gps_summary), "GPS %s %usat", gps_age,
                    static_cast<unsigned>(u.gps_satellites));
        append_status_line(status, sizeof(status), gps_summary);

        if (u.target_selected) {
            char target_age[12] = {};
            char target_summary[24] = {};
            format_age_short(target_age, sizeof(target_age), u.target_age_known, u.target_age_ms);
            lv_snprintf(target_summary, sizeof(target_summary), "Friend %s", target_age);
            append_status_line(status, sizeof(status), target_summary);
        }

        if (u.target_freshness == Biz::FindFriendTargetFreshness::Aging) {
            append_status_line(status, sizeof(status), "Target aging");
        }
    }

    if (u.gps_fix && u.gps_low_accuracy) {
        append_status_line(status, sizeof(status), "GPS low accuracy");
    }
    if (!u.compass_hw) {
        append_status_line(status, sizeof(status), "North-up mode");
    } else if (u.compass_quality == 2) {
        append_status_line(status, sizeof(status), "Compass low accuracy");
    }
    setStatusText(status);

    if (!u.compass_hw) {
        if (!u.target_selected) {
            showHeadingOnly(u, "");
        } else if (!u.target_valid) {
            showHeadingOnly(u, "no location");
        } else if (!u.gps_fix) {
            showHeadingOnly(u, "need gps");
        } else if (u.bearing_valid) {
            _dial.setVisible(true);
            _ring_heading_tenths = 0;
            _dial.setRingHeading(0);
            const int16_t bearing = static_cast<int16_t>(
                compass_dial_redraw_tenths(compass_heading_tenths(u.bearing_to_waypoint_deg)));
            _dial.setFriendNeedle(bearing, true, true, false, u.near_target);
            format_nav_distance(_info_text, sizeof(_info_text), u);
            if (_lbl_info) {
                lv_obj_clear_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text_static(_lbl_info, _info_text);
            }
        } else {
            showHeadingOnly(u, "starting...");
        }
        return;
    }
    if (!u.heading_valid) {
        if (!u.target_selected) {
            showInfoOnly("");
        } else if (!u.target_valid) {
            showInfoOnly("no location");
        } else if (u.target_freshness == Biz::FindFriendTargetFreshness::Unknown ||
                   u.target_freshness == Biz::FindFriendTargetFreshness::Stale) {
            showInfoOnly("target stale");
        } else if (u.near_target) {
            _dial.setVisible(true);
            _dial.setFriendNeedle(0, u.gps_fix, false, false, true);
            _near_target = true;
            _direction_valid = false;
            char distance[sizeof(_info_text)] = {};
            format_arrival_distance(distance, sizeof(distance), u);
            setInfoText(distance);
        } else if (!u.gps_fix) {
            showInfoOnly("need gps");
        } else if (u.compass_quality < 2) {
            showInfoOnly("calibrate");
        } else {
            showInfoOnly("starting...");
        }
        return;
    }

    _dial.setVisible(true);

    const float hdg = compass_screen_heading(u.heading_deg, 0);
    const int16_t new_ring =
        compass_dial_redraw_tenths((int16_t)compass_heading_tenths(hdg));
    const int16_t new_turn =
        (u.bearing_valid && u.heading_valid)
            ? (int16_t)(((int32_t)compass_heading_tenths(u.turn_deg) + 5) / 10 * 10)
            : 0;
    const bool new_gps = u.gps_fix;
    const bool new_on_target = u.relative_valid && compass_turn_on_target_deg(u.turn_deg);
    const bool new_direction_valid = u.relative_valid;
    const bool new_near_target = u.near_target;

    if (new_ring != _ring_heading_tenths) {
        _ring_heading_tenths = new_ring;
        _dial.setRingHeading(new_ring);
    }
    if (new_turn != _turn_show_tenths || new_gps != _gps_fix || new_on_target != _on_target ||
        new_direction_valid != _direction_valid || new_near_target != _near_target) {
        _turn_show_tenths = new_turn;
        _gps_fix = new_gps;
        _on_target = new_on_target;
        _direction_valid = new_direction_valid;
        _near_target = new_near_target;
        _dial.setFriendNeedle(new_turn, new_gps, new_direction_valid, new_on_target,
                              new_near_target);
    }

    if (!u.target_selected) {
        setInfoText("");
    } else if (!u.target_valid) {
        setInfoText("no location");
    } else if (u.target_freshness == Biz::FindFriendTargetFreshness::Unknown ||
               u.target_freshness == Biz::FindFriendTargetFreshness::Stale) {
        setInfoText("target stale");
    } else if (u.near_target) {
        char distance[sizeof(_info_text)] = {};
        format_arrival_distance(distance, sizeof(distance), u);
        setInfoText(distance);
    } else if (!u.gps_fix) {
        setInfoText("need gps");
    } else if (u.bearing_valid) {
        format_nav_distance(_info_text, sizeof(_info_text), u);
        if (_lbl_info) {
            lv_obj_clear_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(_lbl_info, _info_text);
        }
    } else {
        setInfoText("starting...");
    }
}

void FindFriendScreen::runDeferredEnterActions() {
    if (_defer_cycle_target) {
        _defer_cycle_target = false;
        if (_biz.findFriendEnabled() && _biz.findFriendMode() == 0 &&
            _biz.findFriendTargetContactIndex() < 0 && _biz.findFriendContactCount() > 0) {
            (void)_biz.tryAutoPickFindFriendTarget();
        }
    }
}

void FindFriendScreen::onExit() {
    (void)lv_async_call_cancel(focusDeferred, this);
    _defer_cycle_target = false;
    _pending_focus = DeferredFocus::None;
    _scheduled_focus = DeferredFocus::None;
    closeOpenDropdown();
    _suppress_released_dropdown = nullptr;
    _waypoint_keyboard_return_focus = nullptr;
    _biz.setFindFriendForegroundActive(false);
    AbstractScreen::onExit();
}

void FindFriendScreen::refresh() {
    syncControls();
    refreshNavigation();
}

void FindFriendScreen::refreshNavigation() {
    layoutDialColumns();

    if (!_biz.findFriendEnabled()) {
        render(_biz.findFriendUi());
        return;
    }

    runDeferredEnterActions();

    render(_biz.findFriendUi());
}

void FindFriendScreen::onAppStateChanged(const AppStateEvent &event) {
    if (event.type == AppStateEventType::ContactLocationChanged) {
        bool target_changed = false;
        if (_biz.findFriendEnabled() && _biz.findFriendMode() == 0 &&
            _biz.findFriendTargetContactIndex() < 0) {
            target_changed = _biz.tryAutoPickFindFriendTarget();
        }
        if (target_changed)
            syncControls();
        refreshNavigation();
        return;
    }
    if (event.type == AppStateEventType::GpsChanged)
    {
        refresh();
        return;
    }
    if (event.type == AppStateEventType::CompassChanged)
    {
        refreshNavigation();
        return;
    }
    if (event.type == AppStateEventType::FindFriendChanged ||
        event.type == AppStateEventType::TeamChanged ||
        event.type == AppStateEventType::ConfigChanged) {
        refresh();
    }
}

void FindFriendScreen::onActivated() {
    refresh();
}

void FindFriendScreen::onAppEvent(const UiMessage &event) {
    if (event.type == UiMessageType::WaypointKeyboardClosed) {
        restoreWaypointKeyboardFocus();
        return;
    }
    if (event.type != UiMessageType::WaypointKeyboardSubmit || !event.payload) return;

    const auto *submit = static_cast<const UiWaypointKeyboardSubmit *>(event.payload);
    if (_biz.setFindFriendWaypoint(submit->lat, submit->lon)) {
        showAlert("Waypoint saved", 2000);
        syncControls();
    } else {
        showAlert("Save failed", 2000);
    }
}

} // namespace Heltec::MeshCore::Ui

#endif // ENV_INCLUDE_GPS
