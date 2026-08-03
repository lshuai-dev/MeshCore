#include "recent_screen.hpp"

#include <lvgl.h>

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui {
namespace {

void formatAge(int32_t secs, char* buf, size_t buf_size) {
  if (!buf || buf_size == 0) return;
  if (secs < 0) secs = 0;
  if (secs < 60) {
    lv_snprintf(buf, buf_size, "%ds", static_cast<int>(secs));
  } else if (secs < 3600) {
    lv_snprintf(buf, buf_size, "%dm", static_cast<int>(secs / 60));
  } else {
    lv_snprintf(buf, buf_size, "%dh", static_cast<int>(secs / 3600));
  }
}

}  // namespace

_lv_obj_t* RecentScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::RecentScreenRoot);
}

_lv_obj_t* RecentScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(_root, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);

  // Measure one fully styled row first. The viewport and font metrics differ
  // between variants, so the number of reusable controls must be derived from
  // the actual LVGL layout instead of a device-specific constant.
  if (!createRowControl(0)) return nullptr;
  lv_snprintf(_name_text[0], sizeof(_name_text[0]), "Ag");
  lv_snprintf(_age_text[0], sizeof(_age_text[0]), "00h");
  lv_label_set_text_static(_names[0], _name_text[0]);
  lv_label_set_text_static(_ages[0], _age_text[0]);
  lv_obj_update_layout(_root);

  _row_control_count = calculateRowControlCount();
  for (int i = 1; i < _row_control_count; ++i) {
    if (!createRowControl(i)) {
      // Keep the screen usable with the controls already allocated. A Recent
      // row is a reusable window slot, so reduced capacity only means smaller
      // pages; it must not abort the complete UI initialization at Splash.
      _row_control_count = i;
      break;
    }
  }

  _refresh_timer = lv_timer_create(refreshTimerCallback, 1000U, this);
  if (_refresh_timer) {
    lv_timer_set_repeat_count(_refresh_timer, -1);
    lv_timer_pause(_refresh_timer);
  }
  refreshRows();
  return _root;
}

bool RecentScreen::createRowControl(int index) {
  if (index < 0 || index >= kAdvertCapacity) return false;

  _rows[index] = ht_obj_create(_root, meta_id::RecentRow);
  if (!_rows[index]) return false;
  lv_obj_set_size(_rows[index], lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(_rows[index], LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(_rows[index], LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_rows[index], 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(_rows[index], 4, LV_PART_MAIN);
  lv_obj_clear_flag(_rows[index], LV_OBJ_FLAG_SCROLLABLE);

  _names[index] = ht_label_create(_rows[index], meta_id::RecentName, "");
  _ages[index] = ht_label_create(_rows[index], meta_id::RecentAge, "");
  if (!_names[index] || !_ages[index]) {
    lv_obj_del(_rows[index]);
    _rows[index] = nullptr;
    _names[index] = nullptr;
    _ages[index] = nullptr;
    return false;
  }
  lv_label_set_text_static(_names[index], _name_text[index]);
  lv_label_set_text_static(_ages[index], _age_text[index]);
  lv_obj_set_width(_names[index], 1);
  lv_obj_set_flex_grow(_names[index], 1);
  lv_label_set_long_mode(_names[index], LV_LABEL_LONG_DOT);
  lv_obj_set_width(_ages[index], LV_SIZE_CONTENT);
  lv_obj_set_style_text_align(_ages[index], LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_add_event_cb(_rows[index], onRowFocused, LV_EVENT_FOCUSED, this);
  addFocusItem(_rows[index], nullptr, true, FocusVisual::Row);
  return true;
}

int RecentScreen::calculateRowControlCount() {
  lv_coord_t available_height = lv_obj_get_content_height(_root);
  if (available_height <= 0) available_height = lv_obj_get_height(_root);

  // During boot some parent layouts can still be resolving percentages. Walk
  // upward to the first usable viewport instead of falling back to a fixed
  // device-specific row count.
  for (lv_obj_t* obj = lv_obj_get_parent(_root);
       available_height <= 0 && obj; obj = lv_obj_get_parent(obj)) {
    lv_obj_update_layout(obj);
    available_height = lv_obj_get_content_height(obj);
    if (available_height <= 0) available_height = lv_obj_get_height(obj);
  }

  lv_coord_t row_height = lv_obj_get_height(_rows[0]);
  if (row_height <= 0) row_height = lv_obj_get_height(_names[0]);
  if (available_height <= 0 || row_height <= 0) return 1;

  lv_coord_t gap = lv_obj_get_style_pad_row(_root, LV_PART_MAIN);
  if (gap < 0) gap = 0;
  const int count = static_cast<int>((available_height + gap) /
                                     (row_height + gap));
  if (count < 1) return 1;
  return count < kAdvertCapacity ? count : kAdvertCapacity;
}

void RecentScreen::onEnter() {
  refreshRows();
  if (_refresh_timer) {
    lv_timer_reset(_refresh_timer);
    lv_timer_resume(_refresh_timer);
  }
  AbstractScreen::onEnter();
  focusSelection();
}

void RecentScreen::onExit() {
  if (_refresh_timer) lv_timer_pause(_refresh_timer);
  AbstractScreen::onExit();
}

void RecentScreen::refreshRows() {
  _item_count = _biz.fillRecentlyHeard(_items, kAdvertCapacity);
  if (_item_count < 0) _item_count = 0;
  if (_item_count > kAdvertCapacity) _item_count = kAdvertCapacity;

  if (_item_count == 0) {
    _selected_index = 0;
    _window_start = 0;
  } else {
    if (_selected_index >= _item_count) _selected_index = _item_count - 1;
    if (_selected_index < 0) _selected_index = 0;
    _window_start = (_selected_index / _row_control_count) * _row_control_count;
  }
  renderWindow();
}

void RecentScreen::renderWindow() {
  for (int row = 0; row < _row_control_count; ++row) {
    const int item = _window_start + row;
    if (item < _item_count) {
      lv_snprintf(_name_text[row], sizeof(_name_text[row]), "%s", _items[item].name);
      formatAge(_items[item].age_seconds, _age_text[row], sizeof(_age_text[row]));
      lv_obj_clear_state(_rows[row], LV_STATE_DISABLED);
      lv_obj_clear_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
    } else if (row == 0 && _item_count == 0) {
      lv_snprintf(_name_text[row], sizeof(_name_text[row]), "(no recent adverts)");
      _age_text[row][0] = '\0';
      lv_obj_clear_state(_rows[row], LV_STATE_DISABLED);
      lv_obj_clear_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
    } else {
      _name_text[row][0] = '\0';
      _age_text[row][0] = '\0';
      lv_obj_add_state(_rows[row], LV_STATE_DISABLED);
      lv_obj_add_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text_static(_names[row], _name_text[row]);
    lv_label_set_text_static(_ages[row], _age_text[row]);
  }
}

void RecentScreen::moveSelection(int delta) {
  if (_item_count <= 0 || delta == 0) return;
  int next = _selected_index + (delta > 0 ? 1 : -1);
  if (next >= _item_count) next = 0;
  if (next < 0) next = _item_count - 1;
  _selected_index = next;

  const int next_window = (_selected_index / _row_control_count) *
                          _row_control_count;
  if (next_window != _window_start) {
    _window_start = next_window;
    renderWindow();
  }
  focusSelection();
}

void RecentScreen::focusSelection() {
  if (_row_control_count <= 0) return;
  int row = _item_count > 0 ? _selected_index - _window_start : 0;
  if (row < 0 || row >= _row_control_count || !_rows[row]) return;
  lv_group_focus_obj(_rows[row]);
  lv_obj_scroll_to_view(_rows[row], LV_ANIM_OFF);
}

bool RecentScreen::onKey(uint32_t key) {
  if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
    moveSelection(1);
    return true;
  }
  if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV) {
    moveSelection(-1);
    return true;
  }
  return AbstractScreen::onKey(key);
}

void RecentScreen::onRowFocused(lv_event_t* event) {
  auto* self = event
                   ? static_cast<RecentScreen*>(lv_event_get_user_data(event))
                   : nullptr;
  lv_obj_t* target = event ? lv_event_get_target(event) : nullptr;
  if (!self || !target) return;
  for (int row = 0; row < self->_row_control_count; ++row) {
    if (self->_rows[row] != target) continue;
    const int item = self->_window_start + row;
    if (item < self->_item_count) self->_selected_index = item;
    return;
  }
}

void RecentScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::RecentHeardChanged) refreshRows();
}

void RecentScreen::onRefreshRequested() {
  refreshRows();
}

void RecentScreen::refreshTimerCallback(lv_timer_t* timer) {
  auto* self = timer ? static_cast<RecentScreen*>(timer->user_data) : nullptr;
  if (self) self->refreshRows();
}

}  // namespace heltec::meshcore::ui
