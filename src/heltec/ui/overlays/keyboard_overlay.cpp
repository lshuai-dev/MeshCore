#include "keyboard_overlay.hpp"
#include "../core/biz_facade.hpp"
#include "MeshCore.h"
#include "ui/app/theme.hpp"
#include "ui/core/ht_event.h"
#include "ui/core/widget_classes.hpp"
#include "ui/theme/widget_theme.hpp"
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
#include "heltec/drivers/display/e213_port.hpp"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lvgl.h>

namespace heltec::meshcore::ui
{
KeyboardOverlay::KeyboardOverlay(biz::IBizFacade &biz)
    : AbstractOverlay(biz, ht_keyboard_overlay_class)
{
}

#if LV_USE_KEYBOARD

namespace
{

constexpr lv_btnmatrix_ctrl_t kWaypointActionCtrl =
    static_cast<lv_btnmatrix_ctrl_t>(2U | LV_BTNMATRIX_CTRL_CLICK_TRIG);

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
static const char *kMessageCompactKbMap[] = {
    "1",          "2", "3", "4", "5",  "6", "7", "8",   "9",  "0",
    "\n",         "a", "b", "c", "d",  "e", "f", "g",   "h",  "i",
    "j",          "k", "l", "m", "\n", "n", "o", "p",   "q",  "r",
    "s",          "t", "u", "v", "w",  "x", "y", "z",   "\n", ".",
    ",",          "-", "/", "@", "#",  "?", "!", "ABC", "SP", LV_SYMBOL_BACKSPACE,
    LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2,
};

static const char *kMessageCompactUpperKbMap[] = {
    "1",          "2", "3", "4", "5",  "6", "7", "8",   "9",  "0",
    "\n",         "A", "B", "C", "D",  "E", "F", "G",   "H",  "I",
    "J",          "K", "L", "M", "\n", "N", "O", "P",   "Q",  "R",
    "S",          "T", "U", "V", "W",  "X", "Y", "Z",   "\n", ".",
    ",",          "-", "/", "@", "#",  "?", "!", "abc", "SP", LV_SYMBOL_BACKSPACE,
    LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactUpperKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2,
};

void install_message_compact_keyboard_map(lv_obj_t *kb)
{
    if (!kb)
        return;
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_2, kMessageCompactKbMap, kMessageCompactKbCtrl);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_3, kMessageCompactUpperKbMap,
                        kMessageCompactUpperKbCtrl);
}
#else
void install_message_compact_keyboard_map(lv_obj_t *)
{
}
#endif

/** Dedicated numeric pad for the two waypoint coordinate editors. */
static const char *kWaypointKbMap[] = {
    "7", "8", "9", LV_SYMBOL_BACKSPACE, "\n",
    "4", "5", "6", "LAT", "\n",
    "1", "2", "3", "LON", "\n",
    "-", "0", ".", "GPS", "\n",
    LV_SYMBOL_CLOSE, LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kWaypointKbCtrl[] = {
    1, 1, 1, 2,
    1, 1, 1, kWaypointActionCtrl,
    1, 1, 1, kWaypointActionCtrl,
    1, 1, 1, kWaypointActionCtrl,
    kWaypointActionCtrl, kWaypointActionCtrl,
};

/** Four-row keypad for 80-pixel-high devices such as T1. */
static const char *kWaypointCompactKbMap[] = {
    "1", "2", "3", "4", "5", "\n",
    "6", "7", "8", "9", "0", "\n",
    "-", ".", "LAT", "LON", "\n",
    "GPS", LV_SYMBOL_BACKSPACE, LV_SYMBOL_CLOSE, LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kWaypointCompactKbCtrl[] = {
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, kWaypointActionCtrl, kWaypointActionCtrl,
    kWaypointActionCtrl, 2, kWaypointActionCtrl, kWaypointActionCtrl,
};

void install_waypoint_keyboard_map(lv_obj_t *kb)
{
    if (!kb)
        return;
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kWaypointKbMap, kWaypointKbCtrl);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_4, kWaypointCompactKbMap,
                        kWaypointCompactKbCtrl);
}

bool compact_keyboard_display()
{
    lv_disp_t *disp = lv_disp_get_default();
    return disp && lv_disp_get_ver_res(disp) <= 122;
}

bool tiny_keyboard_display()
{
    lv_disp_t *disp = lv_disp_get_default();
    return disp && lv_disp_get_ver_res(disp) <= 80;
}

lv_keyboard_mode_t waypoint_keyboard_mode()
{
    return tiny_keyboard_display() ? LV_KEYBOARD_MODE_USER_4 : LV_KEYBOARD_MODE_USER_1;
}

lv_keyboard_mode_t message_keyboard_mode()
{
    return compact_keyboard_display() ? LV_KEYBOARD_MODE_USER_2 : LV_KEYBOARD_MODE_TEXT_LOWER;
}

void set_keyboard_layout_state(lv_obj_t *root, lv_obj_t *kb, lv_obj_t *latitude,
                               lv_obj_t *longitude, bool waypoint)
{
    const lv_state_t layout_states = LV_STATE_USER_1;
    lv_obj_t *objs[] = { root, kb, latitude, longitude };
    for (lv_obj_t *obj : objs)
    {
        if (!obj)
            continue;
        lv_obj_clear_state(obj, layout_states);
        if (waypoint)
        {
            lv_obj_add_state(obj, LV_STATE_USER_1);
        }
    }
}

void apply_message_keyboard_layout(lv_obj_t *root, lv_obj_t *kb, lv_obj_t *textarea,
                                   lv_obj_t *longitude)
{
    set_keyboard_layout_state(root, kb, textarea, longitude, false);
    if (textarea)
    {
        lv_textarea_set_one_line(textarea, false);
    }
    if (longitude)
        lv_obj_add_flag(longitude, LV_OBJ_FLAG_HIDDEN);
    if (root)
    {
        lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_scroll_to_y(root, 0, LV_ANIM_OFF);
    }
}

void apply_waypoint_keyboard_layout(lv_obj_t *root, lv_obj_t *kb, lv_obj_t *latitude,
                                    lv_obj_t *longitude)
{
    set_keyboard_layout_state(root, kb, latitude, longitude, true);
    if (latitude)
    {
        lv_textarea_set_one_line(latitude, true);
    }
    if (longitude)
    {
        lv_textarea_set_one_line(longitude, true);
        lv_obj_clear_flag(longitude, LV_OBJ_FLAG_HIDDEN);
    }
    if (root)
    {
        lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_scroll_to_y(root, 0, LV_ANIM_OFF);
    }
}

void reset_keyboard_interaction(lv_obj_t *kb)
{
    if (!kb)
        return;
    lv_btnmatrix_clear_btn_ctrl_all(kb, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_clear_state(kb, LV_STATE_PRESSED | LV_STATE_CHECKED);
    lv_obj_invalidate(kb);
}

/* LVGL's built-in text maps label the space key with a literal space.  That
 * is functional but invisible on the e-paper display, so expose it as SP. */
void label_message_space_key(lv_obj_t *kb)
{
    constexpr size_t kMaxKeyboardMapEntries = 64;
    static const char *s_visible_space_map[kMaxKeyboardMapEntries] = {};

    if (!kb)
        return;

    const char **map = lv_keyboard_get_map_array(kb);
    if (!map)
        return;

    size_t index = 0;
    bool has_space_key = false;
    while (index < (kMaxKeyboardMapEntries - 1))
    {
        const char *label = map[index];
        s_visible_space_map[index] = label;
        if (!label || label[0] == '\0')
            break;
        if (strcmp(label, " ") == 0)
        {
            s_visible_space_map[index] = "SP";
            has_space_key = true;
        }
        ++index;
    }

    if (index >= (kMaxKeyboardMapEntries - 1))
        s_visible_space_map[kMaxKeyboardMapEntries - 1] = "";
    if (has_space_key)
        lv_btnmatrix_set_map(kb, s_visible_space_map);
}

#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
void request_keyboard_refresh(lv_obj_t *root, const char *reason)
{
    if (!root || lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN))
        return;
    lv_obj_invalidate(root);
    heltec::meshcore::dal::e213::requestSelectionRefresh(reason);
}
#endif

bool parse_waypoint_text(const char *txt, double &lat, double &lon, const char **error)
{
    if (error)
        *error = "Enter valid coordinates";
    if (!txt || !*txt)
    {
        if (error)
            *error = "Enter latitude and longitude";
        return false;
    }

    char work[64];
    strncpy(work, txt, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char *start = work;
    while (*start == ' ')
        ++start;
    char *end = start + strlen(start);
    while (end > start && end[-1] == ' ')
        --end;
    *end = '\0';
    if (*start == '\0')
    {
        if (error)
            *error = "Enter latitude and longitude";
        return false;
    }

    bool saw_numeric_pair = false;

    auto parse_token = [](const char *token, double &out) -> bool
    {
        if (!token || !*token)
            return false;
        char buf[24];
        strncpy(buf, token, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *b = buf;
        while (*b == ' ')
            ++b;
        char *e = b + strlen(b);
        while (e > b && e[-1] == ' ')
            --e;
        *e = '\0';
        if (*b == '\0')
            return false;

        int commas = 0;
        int dots = 0;
        for (char *p = b; *p; ++p)
        {
            if (*p == ',')
                ++commas;
            else if (*p == '.')
                ++dots;
        }
        if (commas == 1 && dots == 0)
        {
            char *comma = strchr(b, ',');
            if (comma)
                *comma = '.';
        }

        char *endptr = nullptr;
        out = strtod(b, &endptr);
        while (endptr && *endptr == ' ')
            ++endptr;
        return endptr != b && endptr != nullptr && *endptr == '\0';
    };

    auto in_range = [](double la, double lo) -> bool
    { return la >= -90.0 && la <= 90.0 && lo >= -180.0 && lo <= 180.0; };

    auto assign_lat_lon = [&](double a, double b) -> bool
    {
        saw_numeric_pair = true;
        if (in_range(a, b))
        {
            lat = a;
            lon = b;
            if (error)
                *error = nullptr;
            return true;
        }
        return false;
    };

    char *sp = strchr(start, ' ');
    if (sp)
    {
        *sp = '\0';
        double a = 0;
        double b = 0;
        if (parse_token(start, a) && parse_token(sp + 1, b) && assign_lat_lon(a, b))
        {
            return true;
        }
        *sp = ' ';
    }

    {
        double a = 0;
        double b = 0;
        if (sscanf(start, "%lf,%lf", &a, &b) == 2 && assign_lat_lon(a, b))
        {
            return true;
        }
    }

    int comma_pos[8];
    int comma_count = 0;
    for (char *p = start; *p && comma_count < 8; ++p)
    {
        if (*p == ',')
            comma_pos[comma_count++] = static_cast<int>(p - start);
    }

    for (int split = comma_count - 1; split >= 1; --split)
    {
        const char saved = start[comma_pos[split]];
        start[comma_pos[split]] = '\0';
        double a = 0;
        double b = 0;
        const bool ok = parse_token(start, a) && parse_token(start + comma_pos[split] + 1, b) &&
                        assign_lat_lon(a, b);
        start[comma_pos[split]] = saved;
        if (ok)
            return true;
    }

    if (comma_count >= 1)
    {
        start[comma_pos[0]] = '\0';
        double a = 0;
        double b = 0;
        if (parse_token(start, a) && parse_token(start + comma_pos[0] + 1, b) &&
            assign_lat_lon(a, b))
        {
            return true;
        }
    }

    if (error && saw_numeric_pair)
    {
        *error = "Latitude or longitude out of range";
    }
    return false;
}

} // namespace

void KeyboardOverlay::on_keyboard_value_pre(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
        return;
    KeyboardOverlay *self = static_cast<KeyboardOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    lv_obj_t *kb = lv_event_get_target(e);
    const uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
    if (btn_id == LV_BTNMATRIX_BTN_NONE)
        return;

    const char *txt = lv_btnmatrix_get_btn_text(kb, btn_id);
    if (!txt)
        return;

    if (self->_compose_mode == ComposeMode::Message)
    {
        if (strcmp(txt, "ABC") == 0)
        {
            lv_keyboard_set_mode(kb, compact_keyboard_display() ? LV_KEYBOARD_MODE_USER_3
                                                                  : LV_KEYBOARD_MODE_TEXT_UPPER);
            label_message_space_key(kb);
            reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
            request_keyboard_refresh(self->_root, "keyboard-layout");
#endif
            lv_event_stop_processing(e);
        }
        else if (strcmp(txt, "abc") == 0)
        {
            lv_keyboard_set_mode(kb, compact_keyboard_display() ? LV_KEYBOARD_MODE_USER_2
                                                                  : LV_KEYBOARD_MODE_TEXT_LOWER);
            label_message_space_key(kb);
            reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
            request_keyboard_refresh(self->_root, "keyboard-layout");
#endif
            lv_event_stop_processing(e);
        }
        else if (strcmp(txt, "SP") == 0)
        {
            if (self->_textarea)
                lv_textarea_add_char(self->_textarea, ' ');
            reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
            request_keyboard_refresh(self->_root, "keyboard-value");
#endif
            lv_event_stop_processing(e);
        }
        return;
    }

    if (strcmp(txt, LV_SYMBOL_OK) == 0)
    {
        if (self->_skip_next_ok_value && lv_tick_elaps(self->_skip_next_ok_value_ms) < 250)
        {
            self->_skip_next_ok_value = false;
            self->_skip_next_ok_value_ms = 0;
            reset_keyboard_interaction(kb);
            lv_event_stop_processing(e);
            return;
        }
        self->_skip_next_ok_value = false;
        self->_skip_next_ok_value_ms = 0;
        reset_keyboard_interaction(kb);
        self->submitWaypointInput();
        lv_event_stop_processing(e);
        return;
    }
    if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0)
    {
        if (self->_active_waypoint_editor)
        {
            self->selectWaypointEditor(self->_active_waypoint_editor);
            lv_textarea_del_char(self->_active_waypoint_editor);
            ui_theme_center_single_line_textarea(self->_active_waypoint_editor);
        }
        reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        request_keyboard_refresh(self->_root, "keyboard-value");
#endif
        lv_event_stop_processing(e);
        return;
    }
    if (strcmp(txt, "LAT") == 0)
    {
        self->selectWaypointEditor(self->_textarea);
        reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        request_keyboard_refresh(self->_root, "keyboard-field");
#endif
        lv_event_stop_processing(e);
        return;
    }
    if (strcmp(txt, "LON") == 0)
    {
        self->selectWaypointEditor(self->_longitude_textarea);
        reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        request_keyboard_refresh(self->_root, "keyboard-field");
#endif
        lv_event_stop_processing(e);
        return;
    }
    if (strcmp(txt, LV_SYMBOL_CLOSE) == 0)
    {
        /* Let LVGL emit LV_EVENT_CANCEL after the click is released. */
        return;
    }
    if (strcmp(txt, "GPS") == 0)
    {
        self->fillWaypointFromGps();
        reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        request_keyboard_refresh(self->_root, "keyboard-gps");
#endif
        lv_event_stop_processing(e);
        return;
    }
    if (self->_active_waypoint_editor && txt[0] != '\0')
    {
        self->selectWaypointEditor(self->_active_waypoint_editor);
        lv_textarea_add_text(self->_active_waypoint_editor, txt);
        ui_theme_center_single_line_textarea(self->_active_waypoint_editor);
        reset_keyboard_interaction(kb);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        request_keyboard_refresh(self->_root, "keyboard-value");
#endif
        lv_event_stop_processing(e);
    }
}

void KeyboardOverlay::on_waypoint_editor_clicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;
    KeyboardOverlay *self = static_cast<KeyboardOverlay *>(lv_event_get_user_data(e));
    if (!self || self->_compose_mode != ComposeMode::Waypoint)
        return;
    self->selectWaypointEditor(lv_event_get_target(e));
}

void KeyboardOverlay::selectWaypointEditor(lv_obj_t *editor)
{
    if (_compose_mode != ComposeMode::Waypoint ||
        (editor != _textarea && editor != _longitude_textarea))
        return;

    _active_waypoint_editor = editor;
    lv_keyboard_set_textarea(_keyboard, editor);
    const bool latitude = editor == _textarea;
    lv_snprintf(_title_text, sizeof(_title_text), "Manual waypoint: %s",
                latitude ? "LAT" : "LON");
    lv_label_set_text_static(_hint, _title_text);

    lv_obj_t *editors[] = { _textarea, _longitude_textarea };
    for (lv_obj_t *item : editors)
    {
        if (!item)
            continue;
        const bool active = item == editor;
        if (tiny_keyboard_display())
        {
            if (active)
                lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_style_border_width(item, active ? 2 : 1, LV_PART_MAIN);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        lv_obj_set_style_border_color(item, ui_color_fg(), LV_PART_MAIN);
#else
        lv_obj_set_style_border_color(item, lv_color_white(), LV_PART_MAIN);
#endif
    }
    ui_theme_center_single_line_textarea(_textarea);
    ui_theme_center_single_line_textarea(_longitude_textarea);
    lv_obj_invalidate(_root);
}

void KeyboardOverlay::setWaypointHint(const char *text)
{
    if (!_hint)
        return;
    lv_snprintf(_title_text, sizeof(_title_text), "%s", text ? text : "");
    lv_label_set_text_static(_hint, _title_text);
    lv_obj_invalidate(_hint);
}

void KeyboardOverlay::fillWaypointFromGps()
{
    const biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
    if (!gps.fix_valid)
    {
        setWaypointHint("GPS fix unavailable");
        return;
    }

    char latitude[20] = {};
    char longitude[20] = {};
    lv_snprintf(latitude, sizeof(latitude), "%.6f", gps.lat_deg);
    lv_snprintf(longitude, sizeof(longitude), "%.6f", gps.lon_deg);
    lv_textarea_set_text(_textarea, latitude);
    lv_textarea_set_text(_longitude_textarea, longitude);
    ui_theme_center_single_line_textarea(_textarea);
    ui_theme_center_single_line_textarea(_longitude_textarea);
    selectWaypointEditor(_longitude_textarea);
    setWaypointHint("GPS coordinates loaded");
}

void KeyboardOverlay::on_keyboard_key_pre(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY)
        return;
    KeyboardOverlay *self = static_cast<KeyboardOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    const uint32_t key = lv_event_get_key(e);
    if (self->_compose_mode == ComposeMode::Waypoint && key == LV_KEY_ENTER)
    {
        const uint16_t btn_id = self->_keyboard ? lv_btnmatrix_get_selected_btn(self->_keyboard)
                                                : LV_BTNMATRIX_BTN_NONE;
        const char *btn_txt = (self->_keyboard && btn_id != LV_BTNMATRIX_BTN_NONE)
                                  ? lv_btnmatrix_get_btn_text(self->_keyboard, btn_id)
                                  : nullptr;
        if (btn_txt && strcmp(btn_txt, LV_SYMBOL_OK) == 0)
        {
            self->_skip_next_ok_value = true;
            self->_skip_next_ok_value_ms = lv_tick_get();
            reset_keyboard_interaction(self->_keyboard);
            self->submitWaypointInput();
            lv_event_stop_processing(e);
            lv_event_stop_bubbling(e);
            return;
        }
    }
    if (key == LV_KEY_ESC)
    {
        self->emitEvent(EventType::KeyboardClose);
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
    }
}

void KeyboardOverlay::on_keyboard_key_post(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY)
        return;
    KeyboardOverlay *self = static_cast<KeyboardOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT || key == LV_KEY_UP || key == LV_KEY_DOWN)
    {
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        if (self->_root && !lv_obj_has_flag(self->_root, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_invalidate(self->_keyboard);
            heltec::meshcore::dal::e213::requestSelectionRefresh("keyboard-selection");
        }
#else
        reset_keyboard_interaction(self->_keyboard);
#endif
    }
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
    else if (key == LV_KEY_ENTER && self->_root &&
             !lv_obj_has_flag(self->_root, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_invalidate(self->_root);
        heltec::meshcore::dal::e213::requestSelectionRefresh("keyboard-value");
    }
#endif
}

void KeyboardOverlay::on_keyboard_events(lv_event_t *e)
{
    auto *self = static_cast<KeyboardOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)
    {
        if (self->_compose_mode == ComposeMode::Waypoint)
            self->submitWaypointInput();
        else
            self->submitMessageInput();
    }
    else if (code == LV_EVENT_CANCEL)
    {
        self->emitEvent(EventType::KeyboardClose);
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
        reset_keyboard_interaction(self->_keyboard);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        request_keyboard_refresh(self->_root, "keyboard-value");
#endif
    }
}

bool KeyboardOverlay::buildSurface()
{
    if (!AbstractOverlay::buildSurface())
        return false;
    return ensureContent();
}

bool KeyboardOverlay::ensureContent()
{
    if (!_root)
        return false;
    if (_keyboard)
        return true;
    auto create_object = [](const lv_obj_class_t * class_p, lv_obj_t * parent) -> lv_obj_t * {
        lv_obj_t *const obj = lv_obj_class_create_obj(class_p, parent);
        if (!obj) {
            return nullptr;
        }

        lv_obj_class_init_obj(obj);

        return obj;
    };

    _hint = create_object(&ht_overlay_bar_class, _root);
    if (!_hint)
        return false;
    lv_label_set_long_mode(_hint, LV_LABEL_LONG_CLIP);
    lv_snprintf(_title_text, sizeof(_title_text), "#broadcast");
    lv_label_set_text_static(_hint, _title_text);

    _textarea = create_object(&ht_keyboard_editor_class, _root);
    if (!_textarea)
        return false;
    lv_textarea_set_one_line(_textarea, false);
    lv_textarea_set_max_length(_textarea, kMaxText);
    lv_textarea_set_text_buffer(_textarea, _text_buffer, sizeof(_text_buffer));
    lv_obj_set_scrollbar_mode(_textarea, LV_SCROLLBAR_MODE_OFF);

    _longitude_textarea = create_object(&ht_keyboard_editor_class, _root);
    if (!_longitude_textarea)
        return false;
    lv_textarea_set_one_line(_longitude_textarea, true);
    lv_textarea_set_max_length(_longitude_textarea, sizeof(_longitude_buffer) - 1U);
    lv_textarea_set_text_buffer(_longitude_textarea, _longitude_buffer,
                                sizeof(_longitude_buffer));
    lv_obj_set_scrollbar_mode(_longitude_textarea, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(_longitude_textarea, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(_textarea, &KeyboardOverlay::on_waypoint_editor_clicked,
                        LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(_longitude_textarea, &KeyboardOverlay::on_waypoint_editor_clicked,
                        LV_EVENT_CLICKED, this);

    _keyboard = create_object(&ht_keyboard_matrix_class, _root);
    if (!_keyboard)
        return false;
    lv_keyboard_set_textarea(_keyboard, _textarea);
    lv_keyboard_set_popovers(_keyboard, false);
    install_message_compact_keyboard_map(_keyboard);
    install_waypoint_keyboard_map(_keyboard);
    lv_keyboard_set_mode(_keyboard, message_keyboard_mode());
    label_message_space_key(_keyboard);
    reset_keyboard_interaction(_keyboard);
    lv_obj_add_event_cb(_keyboard, &KeyboardOverlay::on_keyboard_events, LV_EVENT_READY, this);
    lv_obj_add_event_cb(_keyboard, &KeyboardOverlay::on_keyboard_events, LV_EVENT_CANCEL, this);
    lv_obj_add_event_cb(_keyboard, &KeyboardOverlay::on_keyboard_events, LV_EVENT_VALUE_CHANGED,
                        this);
    lv_obj_add_event_cb(_keyboard, &KeyboardOverlay::on_keyboard_value_pre,
                        static_cast<lv_event_code_t>(LV_EVENT_VALUE_CHANGED | LV_EVENT_PREPROCESS),
                        this);
    lv_obj_add_event_cb(_keyboard, &KeyboardOverlay::on_keyboard_key_pre,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
    lv_obj_add_event_cb(_keyboard, &KeyboardOverlay::on_keyboard_key_post, LV_EVENT_KEY, this);
    apply_message_keyboard_layout(_root, _keyboard, _textarea, _longitude_textarea);
    lv_obj_update_layout(_root);
    return true;
}

bool KeyboardOverlay::prepareMessageInput(const UiMessageKeyboardRequest &request)
{
    if (!_root || !_hint || !_textarea || !_keyboard)
        return false;
    _compose_mode = ComposeMode::Message;
    _active_waypoint_editor = nullptr;
    _message_purpose = request.purpose;
    _preset_index = request.preset_index;
    _message_max_bytes = request.max_bytes > 0 && request.max_bytes <= kMaxText
                             ? request.max_bytes
                             : kMaxText;
    _skip_next_ok_value = false;
    _skip_next_ok_value_ms = 0;
    _submitted_text[0] = '\0';
    lv_snprintf(_title_text, sizeof(_title_text), "#%s",
                request.title ? request.title : "message");
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
    lv_obj_set_style_bg_color(_hint, ui_color_bg(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_hint, ui_color_fg(), LV_PART_MAIN);
#else
    lv_obj_set_style_bg_color(_hint, ui_color_highlight_bg(), LV_PART_MAIN);
    lv_obj_set_style_text_color(_hint, ui_color_highlight_fg(), LV_PART_MAIN);
#endif
    lv_obj_set_style_bg_opa(_hint, LV_OPA_COVER, LV_PART_MAIN);
    lv_textarea_set_accepted_chars(_textarea, nullptr);
    lv_textarea_set_max_length(_textarea, _message_max_bytes);
    lv_textarea_set_text(_textarea, request.initial_text ? request.initial_text : "");
    lv_label_set_text_static(_hint, _title_text);
    lv_keyboard_set_textarea(_keyboard, _textarea);
    lv_keyboard_set_mode(_keyboard, message_keyboard_mode());
    label_message_space_key(_keyboard);
    reset_keyboard_interaction(_keyboard);
    apply_message_keyboard_layout(_root, _keyboard, _textarea, _longitude_textarea);
    lv_obj_update_layout(_root);
    return true;
}

bool KeyboardOverlay::prepareWaypointInput()
{
    if (!_root || !_hint || !_textarea || !_longitude_textarea || !_keyboard)
        return false;
    _compose_mode = ComposeMode::Waypoint;
    _skip_next_ok_value = false;
    _skip_next_ok_value_ms = 0;
    _submitted_text[0] = '\0';
    lv_snprintf(_title_text, sizeof(_title_text), "Manual waypoint: LAT");
    lv_obj_set_style_bg_opa(_hint, LV_OPA_TRANSP, LV_PART_MAIN);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
    lv_obj_set_style_text_color(_hint, ui_color_fg(), LV_PART_MAIN);
#else
    lv_obj_set_style_text_color(_hint, lv_color_white(), LV_PART_MAIN);
#endif
    lv_label_set_text_static(_hint, _title_text);
    lv_textarea_set_max_length(_textarea, sizeof(_longitude_buffer) - 1U);
    lv_textarea_set_text(_textarea, "");
    lv_textarea_set_text(_longitude_textarea, "");
    lv_textarea_set_accepted_chars(_textarea, "0123456789.-");
    lv_textarea_set_accepted_chars(_longitude_textarea, "0123456789.-");
    lv_keyboard_set_mode(_keyboard, waypoint_keyboard_mode());
    reset_keyboard_interaction(_keyboard);
    apply_waypoint_keyboard_layout(_root, _keyboard, _textarea, _longitude_textarea);
    selectWaypointEditor(_textarea);
    ui_theme_center_single_line_textarea(_textarea);
    ui_theme_center_single_line_textarea(_longitude_textarea);
    lv_obj_update_layout(_root);
    return true;
}

void KeyboardOverlay::onEnter()
{
    if (!_root || !_textarea || !_keyboard)
        return;
    lv_obj_clear_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
    setFocusObject(_keyboard);
    if (_compose_mode == ComposeMode::Waypoint)
    {
        apply_waypoint_keyboard_layout(_root, _keyboard, _textarea, _longitude_textarea);
        selectWaypointEditor(_active_waypoint_editor ? _active_waypoint_editor : _textarea);
    }
    lv_obj_update_layout(_root);
    if (_compose_mode == ComposeMode::Waypoint)
        lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
    if (focusGroup())
        lv_group_set_editing(focusGroup(), true);
    lv_obj_invalidate(_root);
}

void KeyboardOverlay::onExit()
{
    AbstractOverlay::onExit();
    if (_textarea)
    {
        lv_textarea_set_text(_textarea, "");
        lv_textarea_set_accepted_chars(_textarea, nullptr);
        lv_textarea_set_max_length(_textarea, kMaxText);
    }
    if (_longitude_textarea)
    {
        lv_textarea_set_text(_longitude_textarea, "");
        lv_textarea_set_accepted_chars(_longitude_textarea, nullptr);
    }
    lv_obj_t *editors[] = { _textarea, _longitude_textarea };
    for (lv_obj_t *editor : editors)
    {
        if (!editor)
            continue;
        lv_obj_set_style_border_width(editor, 1, LV_PART_MAIN);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
        lv_obj_set_style_border_color(editor, ui_color_fg(), LV_PART_MAIN);
#else
        lv_obj_set_style_border_color(editor, ui_color_overlay_fg(), LV_PART_MAIN);
#endif
    }
    if (_keyboard)
    {
        lv_keyboard_set_mode(_keyboard, message_keyboard_mode());
        apply_message_keyboard_layout(_root, _keyboard, _textarea, _longitude_textarea);
    }
    if (focusGroup())
        lv_group_set_editing(focusGroup(), false);
    if (_root)
        lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    _compose_mode = ComposeMode::Message;
    _active_waypoint_editor = nullptr;
    _message_purpose = UiMessageKeyboardPurpose::SendCustom;
    _preset_index = 0;
    _message_max_bytes = kMaxText;
    _skip_next_ok_value = false;
    _skip_next_ok_value_ms = 0;
}

bool KeyboardOverlay::onKey(uint32_t key)
{
    if (key == LV_KEY_ESC)
    {
        emitEvent(EventType::KeyboardClose);
        return true;
    }
    return false;
}

void KeyboardOverlay::submitMessageInput()
{
    if (!_textarea)
        return;

    const char *txt = lv_textarea_get_text(_textarea);
    const size_t len = txt ? strlen(txt) : 0;
    if (len == 0)
    {
        _biz.showAlert("Empty message", 800);
        return;
    }
    if (len > _message_max_bytes)
    {
        _biz.showAlert("Text too long", 800);
        return;
    }

    strncpy(_submitted_text, txt, sizeof(_submitted_text) - 1);
    _submitted_text[sizeof(_submitted_text) - 1] = '\0';

    const UiMessageKeyboardPurpose purpose = _message_purpose;
    const uint8_t preset_index = _preset_index;
    emitEvent(EventType::KeyboardClose);

    UiMessageKeyboardSubmit submit{};
    submit.text = _submitted_text;
    submit.purpose = purpose;
    submit.preset_index = preset_index;
    emitEvent(EventType::MessageKeyboardSubmit, &submit);
}

void KeyboardOverlay::submitWaypointInput()
{
    if (!_textarea || !_longitude_textarea)
        return;

    const char *latitude = lv_textarea_get_text(_textarea);
    const char *longitude = lv_textarea_get_text(_longitude_textarea);
    if (!latitude || latitude[0] == '\0')
    {
        selectWaypointEditor(_textarea);
        setWaypointHint("Latitude is required");
        return;
    }
    if (!longitude || longitude[0] == '\0')
    {
        selectWaypointEditor(_longitude_textarea);
        setWaypointHint("Longitude is required");
        return;
    }
    lv_snprintf(_submitted_text, sizeof(_submitted_text), "%s,%s", latitude, longitude);
    double lat = 0;
    double lon = 0;
    const char *parse_error = nullptr;
    if (!parse_waypoint_text(_submitted_text, lat, lon, &parse_error))
    {
        setWaypointHint(parse_error ? parse_error : "Invalid coordinates");
        return;
    }

    // Close before returning the parsed value so the owning screen is active.
    emitEvent(EventType::KeyboardClose);

    UiWaypointKeyboardSubmit submit;
    submit.lat = lat;
    submit.lon = lon;
    emitEvent(EventType::WaypointKeyboardSubmit, &submit);
}

#else /* !LV_USE_KEYBOARD */

bool KeyboardOverlay::buildSurface()
{
    if (!AbstractOverlay::buildSurface())
        return false;
    return false;
}

bool KeyboardOverlay::prepareMessageInput(const UiMessageKeyboardRequest & /*request*/)
{
    return false;
}

bool KeyboardOverlay::prepareWaypointInput()
{
    return false;
}

void KeyboardOverlay::onEnter()
{
}

void KeyboardOverlay::onExit()
{
}

bool KeyboardOverlay::onKey(uint32_t /*key*/)
{
    return false;
}

void KeyboardOverlay::submitMessageInput()
{
}

void KeyboardOverlay::submitWaypointInput()
{
}

void KeyboardOverlay::on_keyboard_events(lv_event_t * /*e*/)
{
}

void KeyboardOverlay::on_keyboard_value_pre(lv_event_t * /*e*/)
{
}

void KeyboardOverlay::on_keyboard_key_pre(lv_event_t * /*e*/)
{
}

void KeyboardOverlay::on_keyboard_key_post(lv_event_t * /*e*/)
{
}

void KeyboardOverlay::on_waypoint_editor_clicked(lv_event_t * /*e*/)
{
}

void KeyboardOverlay::selectWaypointEditor(lv_obj_t * /*editor*/)
{
}

void KeyboardOverlay::setWaypointHint(const char * /*text*/)
{
}

void KeyboardOverlay::fillWaypointFromGps()
{
}

#endif /* LV_USE_KEYBOARD */

} // namespace heltec::meshcore::ui
