#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "find_friend_screen.hpp"
#include "app/team_join_service.hpp"
#include "compass_common.hpp"
#include "ui/core/ht_event.h"

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

namespace heltec::meshcore::ui {

namespace {
#if defined(HELTEC_T1)
// T1: 80px panel height - 12px top pane - 2px tile padding on each side.
constexpr lv_coord_t kT1FindFriendContentHeight = 64;
constexpr lv_coord_t kT1FindFriendDialPadding = 4;
constexpr lv_coord_t kT1FindFriendDialColumnWidth = kT1FindFriendContentHeight;
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

void format_nav_distance(char *buf, size_t len, const biz::FindFriendUi &u) {
    if (!buf || len == 0) return;
    if (u.distance_m < 0.0) {
        lv_snprintf(buf, len, "--");
        return;
    }
    const char *approximate = u.confidence == biz::FindFriendConfidence::Low ? "~" : "";
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
    _dial.setVisible(false);
    setInfoText(info);
}

void FindFriendScreen::showHeadingOnly(const biz::FindFriendUi &ui, const char *info) {
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

    if (_dial_row) lv_obj_update_layout(_dial_row);
    _dial.layoutSize();

    render(_biz.findFriendUi());
}

bool FindFriendScreen::buildSurface() {
    if (!AbstractScreen::buildSurface()) return false;

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
    _dial_row = lv_obj_create(_root);
    if (!_dial_row) return false;
    ui_theme_apply_find_friend_dial_host(_dial_row);
    // Keep the dial fixed in the left column. Only the settings/info column on
    // the right scrolls when its rows no longer fit on screen.
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(_dial_row, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_grow(_dial_row, 1);
#if defined(HELTEC_T1)
    compass_init_dial_row(_dial_row, 0);
#else
    compass_init_dial_row(_dial_row, (lv_coord_t)FF_INFO_COL_MARGIN_RIGHT);
#endif

    _dial.min_side = 16;
#if defined(HELTEC_T1)
    // 64px column minus 4px padding on every side gives a 56px dial.
    _dial.edge_margin = kT1FindFriendDialPadding;
#endif
    _dial.needle_kind = CompassDialNeedleKind::FriendTurn;
    if (!_dial.create(_dial_row)) return false;
#if defined(HELTEC_T1)
    // Give the graphic only the square it needs; the settings column receives
    // every remaining horizontal pixel.
    lv_obj_set_width(_dial.center_col, kT1FindFriendDialColumnWidth);
    lv_obj_set_flex_grow(_dial.center_col, 0);
#endif

    _right_column = compass_create_info_column(_dial_row, (lv_coord_t)FF_INFO_COL_WIDTH_PCT,
                                               (lv_coord_t)FF_INFO_COL_PAD_LEFT,
#if defined(HELTEC_T1)
                                               0
#else
                                               1
#endif
    );
    if (!_right_column) return false;
#if defined(HELTEC_T1)
    lv_obj_set_width(_right_column, 0);
    lv_obj_set_flex_grow(_right_column, 1);
#endif
    lv_obj_clear_flag(_right_column, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_flag(_right_column, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_right_column, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_right_column, LV_SCROLLBAR_MODE_AUTO);
    return true;
}

void FindFriendScreen::createInfoRows() {
    if (!_right_column) return;

    _lbl_info = lv_label_create(_right_column);
    compass_style_info_label(_lbl_info, "starting...", LV_LABEL_LONG_CLIP);
    if (_lbl_info) {
        lv_label_set_text_static(_lbl_info, _info_text);
    }

    _lbl_status = lv_label_create(_right_column);
    compass_style_info_label(_lbl_status, "", LV_LABEL_LONG_WRAP);
    if (_lbl_status) lv_label_set_text_static(_lbl_status, _status_text);
}

void FindFriendScreen::render(const biz::FindFriendUi &u) {
    _dial.setCenterMarkerVisible(!_biz.findFriendEnabled());

    const biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    const auto team_state =
        static_cast<heltec::meshcore::team::JoinState>(team.pair_state);
    if (_biz.findFriendEnabled() && _biz.findFriendMode() == 0 && !team.has_team) {
        if (team_state == heltec::meshcore::team::JoinState::Searching) {
            setStatusText("Searching Team...");
            showHeadingOnly(u, "Waiting for Team offers");
            return;
        }
        if (team_state == heltec::meshcore::team::JoinState::TimedOut ||
            team_state == heltec::meshcore::team::JoinState::Error) {
            setStatusText("Team search timed out");
            showHeadingOnly(u, "Select search again");
            return;
        }
    }
    if (static_cast<heltec::meshcore::team::JoinState>(team.pair_state) ==
        heltec::meshcore::team::JoinState::WaitingForApproval) {
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

    char gps_age[12];
    format_age_short(gps_age, sizeof(gps_age), u.gps_fix, u.gps_age_ms);
    const char *compass_status =
        u.compass_quality < 2 ? " calibrate" : (u.compass_quality == 2 ? " compass low" : "");
    const char *gps_status = u.gps_low_accuracy ? " GPS low" : "";
    const char *target_status =
        u.target_freshness == biz::FindFriendTargetFreshness::Aging ? " target aging" : "";
    const char *declination_status = u.declination_configured ? "" : " declination unset";
    if (u.mode == 0 && u.target_selected) {
        char target_age[12];
        format_age_short(target_age, sizeof(target_age), u.target_age_known, u.target_age_ms);
        lv_snprintf(_status_text, sizeof(_status_text), "GPS %s %usat\nfriend %s\nQ%d%s%s%s%s",
                    gps_age, (unsigned)u.gps_satellites, target_age, u.compass_quality,
                    compass_status, gps_status, target_status, declination_status);
    } else {
        lv_snprintf(_status_text, sizeof(_status_text), "GPS %s %usat\nQ%d%s%s%s", gps_age,
                    (unsigned)u.gps_satellites, u.compass_quality, compass_status, gps_status,
                    declination_status);
    }
    setStatusText(_status_text);

    if (!u.compass_hw) {
        showInfoOnly("no compass");
        return;
    }
    if (!u.heading_valid) {
        if (!u.target_selected) {
            showInfoOnly("");
        } else if (!u.target_valid) {
            showInfoOnly("no location");
        } else if (u.target_freshness == biz::FindFriendTargetFreshness::Unknown ||
                   u.target_freshness == biz::FindFriendTargetFreshness::Stale) {
            showInfoOnly("target stale");
        } else if (u.near_target) {
            _dial.setVisible(true);
            _dial.setFriendNeedle(0, u.gps_fix, false, false, true);
            _near_target = true;
            _direction_valid = false;
            setInfoText("nearby");
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
    } else if (u.target_freshness == biz::FindFriendTargetFreshness::Unknown ||
               u.target_freshness == biz::FindFriendTargetFreshness::Stale) {
        setInfoText("target stale");
    } else if (u.near_target) {
        setInfoText("nearby");
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
    (void)lv_async_call_cancel(focusTeamDeferred, this);
    (void)lv_async_call_cancel(focusMemberDeferred, this);
    _defer_cycle_target = false;
    _focus_team_when_offer_available = false;
    _focus_member_when_team_active = false;
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
    if (_dial.side == 0) {
        if (_dial_row) lv_obj_update_layout(_dial_row);
        _dial.layoutSize();
    }

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
    if (event.type == AppStateEventType::CompassChanged ||
        event.type == AppStateEventType::GpsChanged)
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

void FindFriendScreen::onUIEvent(const UiEvent &event) {
    if (event.type == EventType::WaypointKeyboardClosed) {
        restoreWaypointKeyboardFocus();
        return;
    }
    if (event.type != EventType::WaypointKeyboardSubmit || !event.payload) return;

    const auto *submit = static_cast<const UiWaypointKeyboardSubmit *>(event.payload);
    if (_biz.setFindFriendWaypoint(submit->lat, submit->lon)) {
        _biz.showAlert("Waypoint saved", 2000);
        syncControls();
    } else {
        _biz.showAlert("Save failed", 2000);
    }
}

} // namespace heltec::meshcore::ui

#endif // ENV_INCLUDE_COMPASS
