#include "keyboard_overlay.hpp"

#include <lvgl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "MeshCore.h"
#include "../core/biz_facade.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_events.h"

namespace heltec::meshcore::ui {

_lv_obj_t* KeyboardOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::KeyboardOverlayRoot);
}

#if LV_USE_KEYBOARD

namespace {

constexpr int kWaypointInvalidAlertMs = 3000;

static const char* kMessageCompactKbMap[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "\n",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "\n",
    ".", ",", "-", "/", "@", "#", "?", "!", "ABC", "SP", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2,
};

static const char* kMessageCompactUpperKbMap[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "\n",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "\n",
    ".", ",", "-", "/", "@", "#", "?", "!", "abc", "SP", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactUpperKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 2,
};

/** Two-row lat/lon pad; row 2 uses hidden spacers so keys stay centered under row 1. */
static const char* kWaypointKbMap[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    " ", " ", ".", ",", "<-", "OK", " ", " ", " ", " ", "",
};

static const lv_btnmatrix_ctrl_t kWaypointKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    LV_BTNMATRIX_CTRL_HIDDEN | 1, LV_BTNMATRIX_CTRL_HIDDEN | 1, 2, 2, 2, 2,
    LV_BTNMATRIX_CTRL_HIDDEN | 1, LV_BTNMATRIX_CTRL_HIDDEN | 1, LV_BTNMATRIX_CTRL_HIDDEN | 1,
    LV_BTNMATRIX_CTRL_HIDDEN | 1,
};

void install_message_compact_keyboard_map(lv_obj_t* kb) {
  if (!kb) return;
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_2, kMessageCompactKbMap, kMessageCompactKbCtrl);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_3,
                      kMessageCompactUpperKbMap, kMessageCompactUpperKbCtrl);
}

void install_waypoint_keyboard_map(lv_obj_t* kb) {
  if (!kb) return;
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kWaypointKbMap, kWaypointKbCtrl);
}

const lv_font_t* waypoint_keyboard_font() {
#if defined(LV_FONT_MONTSERRAT_12) && LV_FONT_MONTSERRAT_12
  return &lv_font_montserrat_12;
#elif defined(LV_FONT_UNSCII_16) && LV_FONT_UNSCII_16
  return &lv_font_unscii_16;
#else
  return LV_FONT_DEFAULT;
#endif
}

bool compact_keyboard_display() {
  lv_disp_t* disp = lv_disp_get_default();
  return disp && lv_disp_get_ver_res(disp) <= 80;
}

lv_keyboard_mode_t message_keyboard_mode() {
  return compact_keyboard_display() ? LV_KEYBOARD_MODE_USER_2 : LV_KEYBOARD_MODE_TEXT_LOWER;
}

static lv_style_t s_kb_root_message_style;
static lv_style_t s_kb_root_waypoint_style;
static lv_style_t s_kb_message_main_style;
static lv_style_t s_kb_message_items_style;
static lv_style_t s_kb_waypoint_main_style;
static lv_style_t s_kb_waypoint_items_style;
static lv_style_t s_textarea_message_style;
static lv_style_t s_textarea_waypoint_style;
static bool s_keyboard_layout_styles_ready = false;

void init_keyboard_layout_styles() {
  if (s_keyboard_layout_styles_ready) return;
  const bool compact = compact_keyboard_display();

  lv_style_init(&s_kb_root_message_style);
  lv_style_set_flex_main_place(&s_kb_root_message_style, LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&s_kb_root_message_style, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&s_kb_root_message_style, LV_FLEX_ALIGN_START);
  if (compact) {
    lv_style_set_pad_ver(&s_kb_root_message_style, 1);
    lv_style_set_pad_row(&s_kb_root_message_style, 1);
  }

  lv_style_init(&s_kb_root_waypoint_style);
  lv_style_set_flex_main_place(&s_kb_root_waypoint_style, LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&s_kb_root_waypoint_style, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&s_kb_root_waypoint_style, LV_FLEX_ALIGN_START);
  lv_style_set_pad_ver(&s_kb_root_waypoint_style, compact ? 1 : 2);
  lv_style_set_pad_row(&s_kb_root_waypoint_style, compact ? 1 : 2);

  lv_style_init(&s_kb_message_main_style);
  lv_style_set_flex_grow(&s_kb_message_main_style, compact ? 0 : 1);
  lv_style_set_width(&s_kb_message_main_style, lv_pct(100));
  lv_style_set_height(&s_kb_message_main_style, compact ? 36 : LV_SIZE_CONTENT);
  lv_style_set_min_height(&s_kb_message_main_style, compact ? 36 : 40);
  if (compact) {
    lv_style_set_pad_all(&s_kb_message_main_style, 0);
    lv_style_set_pad_row(&s_kb_message_main_style, 0);
    lv_style_set_pad_column(&s_kb_message_main_style, 0);
  }

  lv_style_init(&s_kb_message_items_style);
  lv_style_set_text_font(&s_kb_message_items_style, LV_FONT_DEFAULT);
  lv_style_set_pad_all(&s_kb_message_items_style, compact ? 0 : 1);

  lv_style_init(&s_kb_waypoint_main_style);
  lv_style_set_flex_grow(&s_kb_waypoint_main_style, 0);
  lv_style_set_width(&s_kb_waypoint_main_style, lv_pct(100));
  lv_style_set_height(&s_kb_waypoint_main_style, compact ? 36 : 40);
  lv_style_set_min_height(&s_kb_waypoint_main_style, compact ? 36 : 40);

  lv_style_init(&s_kb_waypoint_items_style);
  lv_style_set_text_font(&s_kb_waypoint_items_style, waypoint_keyboard_font());
  lv_style_set_pad_all(&s_kb_waypoint_items_style, compact ? 1 : 2);

  lv_style_init(&s_textarea_message_style);
  lv_style_set_text_font(&s_textarea_message_style, LV_FONT_DEFAULT);
  lv_style_set_height(&s_textarea_message_style, compact ? 16 : 18);
  if (compact) {
    lv_style_set_min_height(&s_textarea_message_style, 16);
    lv_style_set_pad_hor(&s_textarea_message_style, 1);
    lv_style_set_pad_ver(&s_textarea_message_style, 1);
  }

  lv_style_init(&s_textarea_waypoint_style);
  lv_style_set_text_font(&s_textarea_waypoint_style, LV_FONT_DEFAULT);
  lv_style_set_height(&s_textarea_waypoint_style, compact ? 16 : 18);
  if (compact) {
    lv_style_set_min_height(&s_textarea_waypoint_style, 16);
    lv_style_set_pad_hor(&s_textarea_waypoint_style, 1);
    lv_style_set_pad_ver(&s_textarea_waypoint_style, 1);
  }

  s_keyboard_layout_styles_ready = true;
}

void install_keyboard_layout_styles(lv_obj_t* root, lv_obj_t* kb, lv_obj_t* textarea) {
  init_keyboard_layout_styles();
  if (root) {
    lv_obj_add_style(root, &s_kb_root_message_style, LV_PART_MAIN);
    lv_obj_add_style(root, &s_kb_root_waypoint_style,
                     LV_PART_MAIN | LV_STATE_USER_1);
  }
  if (kb) {
    lv_obj_add_style(kb, &s_kb_message_main_style, LV_PART_MAIN);
    lv_obj_add_style(kb, &s_kb_waypoint_main_style,
                     LV_PART_MAIN | LV_STATE_USER_1);
    lv_obj_add_style(kb, &s_kb_message_items_style, LV_PART_ITEMS);
    lv_obj_add_style(kb, &s_kb_waypoint_items_style,
                     LV_PART_ITEMS | LV_STATE_USER_1);
  }
  if (textarea) {
    lv_obj_add_style(textarea, &s_textarea_message_style, LV_PART_MAIN);
    lv_obj_add_style(textarea, &s_textarea_waypoint_style,
                     LV_PART_MAIN | LV_STATE_USER_1);
  }
}

void set_keyboard_layout_state(lv_obj_t* root, lv_obj_t* kb, lv_obj_t* textarea,
                               bool waypoint) {
  const lv_state_t layout_states = LV_STATE_USER_1;
  lv_obj_t* objs[] = {root, kb, textarea};
  for (lv_obj_t* obj : objs) {
    if (!obj) continue;
    lv_obj_clear_state(obj, layout_states);
    if (waypoint) {
      lv_obj_add_state(obj, LV_STATE_USER_1);
    }
  }
}

void apply_message_keyboard_layout(lv_obj_t* root, lv_obj_t* kb, lv_obj_t* textarea) {
  init_keyboard_layout_styles();
  set_keyboard_layout_state(root, kb, textarea, false);
  if (root) {
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_scroll_to_y(root, 0, LV_ANIM_OFF);
  }
}

void apply_waypoint_keyboard_layout(lv_obj_t* root, lv_obj_t* kb, lv_obj_t* textarea) {
  init_keyboard_layout_styles();
  set_keyboard_layout_state(root, kb, textarea, true);
  if (root) {
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_scroll_to_y(root, 0, LV_ANIM_OFF);
  }
}

void reset_keyboard_interaction(lv_obj_t* kb) {
  if (!kb) return;
  lv_btnmatrix_clear_btn_ctrl_all(kb, LV_BTNMATRIX_CTRL_CHECKED);
  lv_obj_clear_state(kb, LV_STATE_PRESSED | LV_STATE_CHECKED);
  lv_obj_invalidate(kb);
}

bool parse_waypoint_text(const char* txt, double& lat, double& lon, const char** error) {
  if (error) *error = "Use: lat,lon";
  if (!txt || !*txt) {
    if (error) *error = "Enter latitude,longitude";
    return false;
  }

  char work[64];
  strncpy(work, txt, sizeof(work) - 1);
  work[sizeof(work) - 1] = '\0';

  char* start = work;
  while (*start == ' ') ++start;
  char* end = start + strlen(start);
  while (end > start && end[-1] == ' ') --end;
  *end = '\0';
  if (*start == '\0') {
    if (error) *error = "Enter latitude,longitude";
    return false;
  }

  bool saw_numeric_pair = false;

  auto parse_token = [](const char* token, double& out) -> bool {
    if (!token || !*token) return false;
    char buf[24];
    strncpy(buf, token, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* b = buf;
    while (*b == ' ') ++b;
    char* e = b + strlen(b);
    while (e > b && e[-1] == ' ') --e;
    *e = '\0';
    if (*b == '\0') return false;

    int commas = 0;
    int dots = 0;
    for (char* p = b; *p; ++p) {
      if (*p == ',') ++commas;
      else if (*p == '.') ++dots;
    }
    if (commas == 1 && dots == 0) {
      char* comma = strchr(b, ',');
      if (comma) *comma = '.';
    }

    char* endptr = nullptr;
    out = strtod(b, &endptr);
    while (endptr && *endptr == ' ') ++endptr;
    return endptr != b && endptr != nullptr && *endptr == '\0';
  };

  auto in_range = [](double la, double lo) -> bool {
    return la >= -90.0 && la <= 90.0 && lo >= -180.0 && lo <= 180.0;
  };

  auto assign_lat_lon = [&](double a, double b) -> bool {
    saw_numeric_pair = true;
    if (in_range(a, b)) {
      lat = a;
      lon = b;
      if (error) *error = nullptr;
      return true;
    }
    if (in_range(b, a)) {
      lat = b;
      lon = a;
      if (error) *error = nullptr;
      return true;
    }
    return false;
  };

  char* sp = strchr(start, ' ');
  if (sp) {
    *sp = '\0';
    double a = 0;
    double b = 0;
    if (parse_token(start, a) && parse_token(sp + 1, b) && assign_lat_lon(a, b)) {
      return true;
    }
    *sp = ' ';
  }

  {
    double a = 0;
    double b = 0;
    if (sscanf(start, "%lf,%lf", &a, &b) == 2 && assign_lat_lon(a, b)) {
      return true;
    }
  }

  int comma_pos[8];
  int comma_count = 0;
  for (char* p = start; *p && comma_count < 8; ++p) {
    if (*p == ',') comma_pos[comma_count++] = static_cast<int>(p - start);
  }

  for (int split = comma_count - 1; split >= 1; --split) {
    const char saved = start[comma_pos[split]];
    start[comma_pos[split]] = '\0';
    double a = 0;
    double b = 0;
    const bool ok = parse_token(start, a) && parse_token(start + comma_pos[split] + 1, b) && assign_lat_lon(a, b);
    start[comma_pos[split]] = saved;
    if (ok) return true;
  }

  if (comma_count >= 1) {
    start[comma_pos[0]] = '\0';
    double a = 0;
    double b = 0;
    if (parse_token(start, a) && parse_token(start + comma_pos[0] + 1, b) && assign_lat_lon(a, b)) {
      return true;
    }
  }

  if (error && saw_numeric_pair) {
    *error = "Out of range: lat -90..90, lon -180..180";
  }
  return false;
}

}  // namespace

void KeyboardOverlay::on_keyboard_value_pre(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  KeyboardOverlay* self = static_cast<KeyboardOverlay*>(lv_event_get_user_data(e));
  if (!self) return;

  lv_obj_t* kb = lv_event_get_target(e);
  const uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
  if (btn_id == LV_BTNMATRIX_BTN_NONE) return;

  const char* txt = lv_btnmatrix_get_btn_text(kb, btn_id);
  if (!txt) return;

  if (self->_compose_mode == ComposeMode::Message) {
    if (strcmp(txt, "ABC") == 0) {
      lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_3);
      reset_keyboard_interaction(kb);
      lv_event_stop_processing(e);
    } else if (strcmp(txt, "abc") == 0) {
      lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_2);
      reset_keyboard_interaction(kb);
      lv_event_stop_processing(e);
    } else if (strcmp(txt, "SP") == 0) {
      if (self->_textarea) lv_textarea_add_char(self->_textarea, ' ');
      reset_keyboard_interaction(kb);
      lv_event_stop_processing(e);
    }
    return;
  }

  if (strcmp(txt, "OK") == 0) {
    if (self->_skip_next_ok_value && lv_tick_elaps(self->_skip_next_ok_value_ms) < 250) {
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
  if (strcmp(txt, "<-") == 0) {
    if (self->_textarea) lv_textarea_del_char(self->_textarea);
    reset_keyboard_interaction(kb);
    lv_event_stop_processing(e);
    return;
  }
  if (txt[0] == ' ' && txt[1] == '\0') {
    reset_keyboard_interaction(kb);
    lv_event_stop_processing(e);
    return;
  }
  if (self->_textarea && txt[0] != '\0') {
    lv_textarea_add_text(self->_textarea, txt);
    reset_keyboard_interaction(kb);
    lv_event_stop_processing(e);
  }
}

void KeyboardOverlay::on_keyboard_key_pre(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) return;
  KeyboardOverlay* self = static_cast<KeyboardOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  const uint32_t key = lv_event_get_key(e);
  if (self->_compose_mode == ComposeMode::Waypoint && key == LV_KEY_ENTER) {
    const uint16_t btn_id = self->_keyboard
                                ? lv_btnmatrix_get_selected_btn(self->_keyboard)
                                : LV_BTNMATRIX_BTN_NONE;
    const char* btn_txt = (self->_keyboard && btn_id != LV_BTNMATRIX_BTN_NONE)
                              ? lv_btnmatrix_get_btn_text(self->_keyboard, btn_id)
                              : nullptr;
    if (btn_txt && strcmp(btn_txt, "OK") == 0) {
      self->_skip_next_ok_value = true;
      self->_skip_next_ok_value_ms = lv_tick_get();
      reset_keyboard_interaction(self->_keyboard);
      self->submitWaypointInput();
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
      return;
    }
  }
  if (key == LV_KEY_ESC) {
    self->emitEvent(UiEventType::KeyboardClose);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
  }
}

void KeyboardOverlay::on_keyboard_key_post(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) return;
  KeyboardOverlay* self = static_cast<KeyboardOverlay*>(lv_event_get_user_data(e));
  if (!self) return;

  const uint32_t key = lv_event_get_key(e);
  if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT || key == LV_KEY_UP || key == LV_KEY_DOWN) {
    reset_keyboard_interaction(self->_keyboard);
  }
}

void KeyboardOverlay::on_keyboard_events(lv_event_t* e) {
  auto* self = static_cast<KeyboardOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    if (self->_compose_mode == ComposeMode::Waypoint) self->submitWaypointInput();
    else self->submitMessageInput();
  } else if (code == LV_EVENT_CANCEL) {
    self->emitEvent(UiEventType::KeyboardClose);
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    reset_keyboard_interaction(self->_keyboard);
  }
}

_lv_obj_t* KeyboardOverlay::create(lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;
  if (!ensureContent()) {
    lv_obj_del(_root);
    _root = nullptr;
  }

  return _root;
}

bool KeyboardOverlay::ensureContent() {
  if (!_root) return false;
  if (_keyboard) return true;
  _title = ht_label_create(_root, meta_id::KeyboardTitle);
  if (!_title) return false;
  lv_obj_set_width(_title, lv_pct(100));
  lv_label_set_long_mode(_title, LV_LABEL_LONG_CLIP);
  lv_snprintf(_title_text, sizeof(_title_text), "#broadcast");
  lv_label_set_text_static(_title, _title_text);
  _textarea = ht_textarea_create(_root, meta_id::KeyboardTextarea);
  if (!_textarea) return false;
  lv_obj_set_size(_textarea, lv_pct(100), 18);
  lv_textarea_set_one_line(_textarea, true);
  lv_textarea_set_max_length(_textarea, kMaxText);
  lv_textarea_set_text_buffer(_textarea, _text_buffer, sizeof(_text_buffer));
  lv_obj_set_scrollbar_mode(_textarea, LV_SCROLLBAR_MODE_OFF);
  _vertical_spacer = lv_obj_create(_root);
  if (!_vertical_spacer) return false;
  lv_obj_set_width(_vertical_spacer, lv_pct(100));
  lv_obj_set_height(_vertical_spacer, 0);
  lv_obj_set_flex_grow(_vertical_spacer, 1);
  lv_obj_set_style_bg_opa(_vertical_spacer, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(_vertical_spacer, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(_vertical_spacer, 0, LV_PART_MAIN);
  lv_obj_clear_flag(_vertical_spacer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_vertical_spacer, LV_OBJ_FLAG_HIDDEN);
  _keyboard = ht_keyboard_create(_root, meta_id::KeyboardKeyboard);
  if (!_keyboard) return false;
  lv_obj_set_width(_keyboard, lv_pct(100));
  lv_obj_set_flex_grow(_keyboard, 1);
  lv_keyboard_set_textarea(_keyboard, _textarea);
  lv_keyboard_set_popovers(_keyboard, false);
  install_message_compact_keyboard_map(_keyboard);
  install_waypoint_keyboard_map(_keyboard);
  lv_keyboard_set_mode(_keyboard, message_keyboard_mode());
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
  install_keyboard_layout_styles(_root, _keyboard, _textarea);
  apply_message_keyboard_layout(_root, _keyboard, _textarea);
  lv_obj_update_layout(_root);
  return true;
}

bool KeyboardOverlay::prepareMessageInput(const char* title_text) {
  if (!_root || !_title || !_textarea || !_keyboard) return false;
  _compose_mode = ComposeMode::Message;
  _skip_next_ok_value = false;
  _skip_next_ok_value_ms = 0;
  _submitted_text[0] = '\0';
  if (_vertical_spacer) lv_obj_add_flag(_vertical_spacer, LV_OBJ_FLAG_HIDDEN);

  lv_snprintf(_title_text, sizeof(_title_text), "#%s",
              title_text ? title_text : "message");
  lv_textarea_set_accepted_chars(_textarea, nullptr);
  lv_textarea_set_max_length(_textarea, kMaxText);
  lv_textarea_set_text(_textarea, "");
  lv_label_set_text_static(_title, _title_text);
  lv_keyboard_set_textarea(_keyboard, _textarea);
  lv_keyboard_set_mode(_keyboard, message_keyboard_mode());
  reset_keyboard_interaction(_keyboard);
  apply_message_keyboard_layout(_root, _keyboard, _textarea);
  lv_obj_update_layout(_root);
  return true;
}

bool KeyboardOverlay::prepareWaypointInput() {
  if (!_root || !_title || !_textarea || !_keyboard) return false;
  _compose_mode = ComposeMode::Waypoint;
  _skip_next_ok_value = false;
  _skip_next_ok_value_ms = 0;
  _submitted_text[0] = '\0';
  if (_vertical_spacer) lv_obj_clear_flag(_vertical_spacer, LV_OBJ_FLAG_HIDDEN);

  lv_snprintf(_title_text, sizeof(_title_text), "lat,lon");
  lv_label_set_text_static(_title, _title_text);
  lv_textarea_set_max_length(_textarea, 48);
  lv_textarea_set_text(_textarea, "");
  lv_textarea_set_accepted_chars(_textarea, "0123456789.,- ");
  lv_keyboard_set_textarea(_keyboard, _textarea);
  lv_keyboard_set_mode(_keyboard, LV_KEYBOARD_MODE_USER_1);
  reset_keyboard_interaction(_keyboard);
  apply_waypoint_keyboard_layout(_root, _keyboard, _textarea);
  lv_obj_update_layout(_root);
  return true;
}

void KeyboardOverlay::onEnter() {
  if (!_root || !_textarea || !_keyboard) return;
  lv_obj_clear_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
  setFocusObject(_keyboard);
  if (_compose_mode == ComposeMode::Waypoint) {
    apply_waypoint_keyboard_layout(_root, _keyboard, _textarea);
  }
  lv_obj_update_layout(_root);
  if (_compose_mode == ComposeMode::Waypoint) lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
  if (_focus_group) lv_group_set_editing(_focus_group, true);
  lv_obj_invalidate(_root);
}

void KeyboardOverlay::onExit() {
  AbstractOverlay::onExit();
  if (_textarea) {
    lv_textarea_set_text(_textarea, "");
    lv_textarea_set_accepted_chars(_textarea, nullptr);
    lv_textarea_set_max_length(_textarea, kMaxText);
  }
  if (_keyboard) {
    lv_keyboard_set_mode(_keyboard, message_keyboard_mode());
    apply_message_keyboard_layout(_root, _keyboard, _textarea);
  }
  if (_vertical_spacer) lv_obj_add_flag(_vertical_spacer, LV_OBJ_FLAG_HIDDEN);
  if (_focus_group) lv_group_set_editing(_focus_group, false);
  if (_root) lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  _compose_mode = ComposeMode::Message;
  _skip_next_ok_value = false;
  _skip_next_ok_value_ms = 0;
}

bool KeyboardOverlay::onKey(uint32_t key) {
  if (key == LV_KEY_ESC) {
    emitEvent(UiEventType::KeyboardClose);
    return true;
  }
  return false;
}

void KeyboardOverlay::submitMessageInput() {
  if (!_textarea) return;

  const char* txt = lv_textarea_get_text(_textarea);
  const size_t len = txt ? strlen(txt) : 0;
  if (len == 0) {
    _biz.showAlert("Empty message", 800);
    return;
  }

  strncpy(_submitted_text, txt, sizeof(_submitted_text) - 1);
  _submitted_text[sizeof(_submitted_text) - 1] = '\0';

  emitEvent(UiEventType::KeyboardClose);

  UiMessageKeyboardSubmit submit;
  submit.text = _submitted_text;
  emitEvent(UiEventType::MessageKeyboardSubmit, &submit);
}

void KeyboardOverlay::submitWaypointInput() {
  if (!_textarea) return;

  const char* txt = lv_textarea_get_text(_textarea);
  double lat = 0;
  double lon = 0;
  const char* parse_error = nullptr;
  if (!parse_waypoint_text(txt, lat, lon, &parse_error)) {
    _biz.showAlert(parse_error ? parse_error : "Invalid lat,lon", kWaypointInvalidAlertMs);
    return;
  }

  // Close before returning the parsed value so the owning screen is active.
  emitEvent(UiEventType::KeyboardClose);

  UiWaypointKeyboardSubmit submit;
  submit.lat = lat;
  submit.lon = lon;
  emitEvent(UiEventType::WaypointKeyboardSubmit, &submit);
}

#else  /* !LV_USE_KEYBOARD */

_lv_obj_t* KeyboardOverlay::create(_lv_obj_t*) { return nullptr; }

bool KeyboardOverlay::prepareMessageInput(const char* /*title*/) { return false; }

bool KeyboardOverlay::prepareWaypointInput() { return false; }

void KeyboardOverlay::onEnter() {}

void KeyboardOverlay::onExit() {}

bool KeyboardOverlay::onKey(uint32_t /*key*/) { return false; }

void KeyboardOverlay::submitMessageInput() {}

void KeyboardOverlay::submitWaypointInput() {}

void KeyboardOverlay::on_keyboard_events(lv_event_t* /*e*/) {}

void KeyboardOverlay::on_keyboard_value_pre(lv_event_t* /*e*/) {}

void KeyboardOverlay::on_keyboard_key_pre(lv_event_t* /*e*/) {}

void KeyboardOverlay::on_keyboard_key_post(lv_event_t* /*e*/) {}

#endif  /* LV_USE_KEYBOARD */

}  // namespace heltec::meshcore::ui
