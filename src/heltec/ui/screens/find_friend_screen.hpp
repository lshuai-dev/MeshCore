#pragma once
#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"
#include "compass_dial_widget.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
struct FindFriendUi;
} // namespace heltec::meshcore::biz

namespace heltec::meshcore::ui {

class FindFriendScreen : public AbstractScreen {
  public:
    FindFriendScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon) {
    }
    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;

  private:
    struct ChoiceRow {
        _lv_obj_t *row = nullptr;
        _lv_obj_t *label = nullptr;
        _lv_obj_t *dropdown = nullptr;
    };

    enum class Action : uint8_t {
        None,
        Team,
        TeamMessage,
        WaypointManual
    };

    enum class DeferredFocus : uint8_t {
        None,
        Role,
        Team,
        Member
    };

    void onUIEvent(const UiEvent &event) override;
    void onAppStateChanged(const AppStateEvent &event) override;
    void onActivated() override;
    void render(const biz::FindFriendUi &u);
    void refresh();
    void refreshNavigation();
    void showInfoOnly(const char *info);
    void showHeadingOnly(const biz::FindFriendUi &ui, const char *info);
    void setInfoText(const char *info);
    void setStatusText(const char *status);
    void runDeferredEnterActions();
    bool createDial();
    void layoutDialColumns();
    void createInfoRows();
    void createControls();
    void startTeamSearch();
    void requestDeferredFocus(DeferredFocus target);
    static void focusDeferred(void *user_data);
    void configureFocusItems();
    _lv_obj_t *addDropdownRow(ChoiceRow &choice, const char *title, const char *options);
    _lv_obj_t *addActionRow(const char *title, Action action);
    void bindControl(_lv_obj_t *control);
    void syncControls(bool force_friend_options = false);
    void updateConditionalVisibility();
    void syncFocusGroupMembership();
    void setDropdownIndex(_lv_obj_t *dropdown, uint16_t index, bool force = false);
    void syncFriendDropdown(bool force = false);
    void loadFriendDropdownWindow(int start, int selected_rank, bool force);
    bool moveFriendDropdownSelection(int direction);
    int friendMeshIndexForSelection() const;
    void beginDropdownEdit(_lv_obj_t *dropdown);
    void endDropdownEdit(_lv_obj_t *dropdown);
    void closeOpenDropdown();
    void scrollFocusedIntoView(_lv_obj_t *focused) const;
    void applyGroupFocus(_lv_obj_t *focused);
    void restoreWaypointKeyboardFocus();
    void handleAction(Action action, _lv_obj_t *source);

    static void onDropdownValueChanged(lv_event_t *e);
    static void onDropdownStateEvent(lv_event_t *e);
    static void onDropdownReleasedPre(lv_event_t *e);
    static void onControlKeyPreprocess(lv_event_t *e);
    static void onRightColumnScrolled(lv_event_t *event);
    static void onActionClicked(lv_event_t *e);
    static void onGroupFocusChanged(lv_group_t *group);

    CompassDialWidget _dial;
    _lv_obj_t *_dial_row = nullptr;
    _lv_obj_t *_right_column = nullptr;
    lv_obj_t *_lbl_info = nullptr;
    lv_obj_t *_lbl_status = nullptr;
    ChoiceRow _choice_mode;
    ChoiceRow _choice_role;
    ChoiceRow _choice_team;
    ChoiceRow _choice_friend;
    _lv_obj_t *_row_wp_manual = nullptr;
    _lv_obj_t *_row_team = nullptr;
    _lv_obj_t *_row_team_message = nullptr;
    _lv_obj_t *_open_dropdown = nullptr;
    _lv_obj_t *_suppress_released_dropdown = nullptr;
    _lv_obj_t *_waypoint_keyboard_return_focus = nullptr;
    uint16_t _open_dropdown_original_index = 0;
    int _friend_open_original_mesh_idx = -1;
    bool _syncing_dropdown = false;
    char _info_text[24] = "starting...";
    char _status_text[80] = {};
    // '(none)' + ten (newline + up to 31 label bytes) + terminator.
    static constexpr int kFriendWindowSize = 10;
    static constexpr int kFriendWindowStep = 5;
    char _friend_options[7 + kFriendWindowSize * 32] = {};
    char _team_options[164] = {};
    int16_t _ring_heading_tenths = 0;
    int16_t _turn_show_tenths = 0;
    bool _gps_fix = false;
    bool _on_target = false;
    bool _direction_valid = false;
    bool _near_target = false;
    bool _defer_cycle_target = false;
    DeferredFocus _pending_focus = DeferredFocus::None;
    DeferredFocus _scheduled_focus = DeferredFocus::None;
    uint16_t _focus_membership_mask = 0xFFFFU;
    int16_t _friend_mesh_map[kFriendWindowSize] = {};
    uint32_t _friend_options_hash = 0;
    int _friend_mesh_map_count = 0;
    int _friend_mesh_map_count_applied = -1;
    int _friend_total = 0;
    int _friend_window_start = 0;
    int _friend_selected_rank = -1;
};

} // namespace heltec::meshcore::ui
