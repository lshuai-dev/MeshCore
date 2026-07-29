#include "choice_picker_overlay.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ui_events.h"

#include <lvgl.h>

namespace heltec::meshcore::ui {

_lv_obj_t* ChoicePickerOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::ChoicePickerOverlayRoot);
}

_lv_obj_t* ChoicePickerOverlay::create(_lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;

  const lv_coord_t gap =
#if defined(HELTEC_V4_R8_TFT)
      LV_DPX(8);
#else
      2;
#endif
  lv_obj_set_style_pad_hor(_root, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(_root, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, gap, LV_PART_MAIN);

  _title = ht_label_create(_root, meta_id::ChoicePickerTitle);
  if (!_title) return nullptr;
  lv_obj_set_width(_title, lv_pct(100));
  lv_label_set_long_mode(_title, LV_LABEL_LONG_DOT);
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  lv_obj_add_flag(_title, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_align(_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_obj_add_event_cb(_title, onTitleClicked, LV_EVENT_CLICKED, this);
#endif

  _list = ht_obj_create(_root, meta_id::ChoicePickerList);
  if (!_list) return nullptr;
  lv_obj_set_width(_list, lv_pct(100));
  lv_obj_set_flex_grow(_list, 1);
  lv_obj_set_flex_flow(_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_list, gap, LV_PART_MAIN);
  lv_obj_clear_flag(_list, LV_OBJ_FLAG_SCROLLABLE);

  for (uint8_t i = 0; i < kMaxVisibleRows; ++i) {
    _rows[i] = ht_label_create(_list, meta_id::ChoicePickerRow);
    if (!_rows[i]) return nullptr;
    lv_obj_set_width(_rows[i], lv_pct(100));
    lv_obj_set_height(_rows[i], ui_settings_row_height());
    lv_label_set_long_mode(_rows[i], LV_LABEL_LONG_DOT);
    lv_label_set_text_static(_rows[i], " ");
    lv_obj_add_flag(_rows[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(_rows[i], LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_event_cb(_rows[i], onRowClicked, LV_EVENT_CLICKED, this);
  }

  _footer = ht_label_create(_root, meta_id::ChoicePickerFooter);
  if (!_footer) return nullptr;
  lv_obj_set_width(_footer, lv_pct(100));
  lv_label_set_long_mode(_footer, LV_LABEL_LONG_CLIP);
  lv_label_set_text_static(_footer, operation_hint::kChoicePicker);
  return _root;
}

bool ChoicePickerOverlay::prepare(IChoicePickerSource* source) {
  if (!source || _source || _finishing) return false;
  _source = source;
  return true;
}

_lv_obj_t* ChoicePickerOverlay::focusTarget() const { return _root; }

int ChoicePickerOverlay::wrapIndex(int index) const {
  if (_count <= 0) return 0;
  index %= _count;
  if (index < 0) index += _count;
  return index;
}

void ChoicePickerOverlay::configureVisibleRows() {
  if (!_root || !_list) return;
  lv_obj_update_layout(_root);

  lv_coord_t row_height = lv_obj_get_height(_rows[0]);
  if (row_height <= 0) {
    const lv_font_t* const font = lv_obj_get_style_text_font(_rows[0], LV_PART_MAIN);
    row_height = font ? lv_font_get_line_height(font) : 1;
  }
  const lv_coord_t row_gap = lv_obj_get_style_pad_row(_list, LV_PART_MAIN);
  const lv_coord_t usable = lv_obj_get_content_height(_list);
  int visible = row_height > 0 ? (usable + row_gap) / (row_height + row_gap) : 1;
  if (visible < 1) visible = 1;
  if (visible > kMaxVisibleRows) visible = kMaxVisibleRows;
  if (visible > _count) visible = _count;
  if (visible > 1 && (visible & 1) == 0) --visible;
  if (visible < 1) visible = 1;
  _visible_rows = static_cast<uint8_t>(visible);

  for (uint8_t i = 0; i < kMaxVisibleRows; ++i) {
    if (i < _visible_rows) {
      lv_obj_clear_flag(_rows[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(_rows[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void ChoicePickerOverlay::renderRows() {
  if (!_source || _count <= 0) return;
  const int center = static_cast<int>(_visible_rows) / 2;
  for (uint8_t row = 0; row < _visible_rows; ++row) {
    const int option = wrapIndex(_selected + static_cast<int>(row) - center);
    _row_option[row] = option;
    char label[kRowTextSize] = {};
    if (!_source->choicePickerOptionLabel(option, label, sizeof(label))) {
      lv_snprintf(label, sizeof(label), "?");
    }
    lv_snprintf(_row_text[row], sizeof(_row_text[row]), "%s%s",
                option == _original ? "*" : "", label);
    lv_label_set_text_static(_rows[row], _row_text[row]);
    if (row == center) {
      lv_obj_add_state(_rows[row], LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(_rows[row], LV_STATE_CHECKED);
    }
  }
}

void ChoicePickerOverlay::onEnter() {
  if (!_root || !_source) return;
  _finishing = false;
  AbstractOverlay::onEnter();
  _count = _source->choicePickerOptionCount();
  if (_count < 1) _count = 1;
  _selected = wrapIndex(_source->choicePickerSelectedIndex());
  _original = _selected;
  const char* const title = _source->choicePickerTitle() ? _source->choicePickerTitle() : "Select";
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  lv_snprintf(_title_text, sizeof(_title_text), "< %s", title);
#else
  lv_snprintf(_title_text, sizeof(_title_text), "%s", title);
#endif
  lv_label_set_text_static(_title, _title_text);
  configureVisibleRows();
  renderRows();
}

void ChoicePickerOverlay::onExit() {
  if (_source && !_finishing) _source->choicePickerClosed(false);
  _source = nullptr;
  _finishing = false;
  _count = 0;
  AbstractOverlay::onExit();
}

void ChoicePickerOverlay::stepSelection(int8_t direction) {
  if (!_source || _finishing || _count <= 1 || direction == 0) return;
  _selected = wrapIndex(_selected + (direction > 0 ? 1 : -1));
  renderRows();
}

bool ChoicePickerOverlay::onKey(uint32_t key) {
  if (key == LV_KEY_NEXT || key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
    stepSelection(1);
    return true;
  }
  if (key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP) {
    stepSelection(-1);
    return true;
  }
  if (key == LV_KEY_ENTER) {
    finish(true);
    return true;
  }
  if (key == LV_KEY_ESC) {
    finish(false);
    return true;
  }
  return false;
}

void ChoicePickerOverlay::onRowClicked(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  auto* self = static_cast<ChoicePickerOverlay*>(lv_event_get_user_data(event));
  if (!self || self->_finishing) return;
  _lv_obj_t* const target = lv_event_get_target(event);
  for (uint8_t i = 0; i < self->_visible_rows; ++i) {
    if (self->_rows[i] != target) continue;
    self->finish(true, self->_row_option[i]);
    lv_event_stop_processing(event);
    lv_event_stop_bubbling(event);
    return;
  }
}

void ChoicePickerOverlay::onTitleClicked(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  auto* self = static_cast<ChoicePickerOverlay*>(lv_event_get_user_data(event));
  if (!self) return;
  self->finish(false);
  lv_event_stop_processing(event);
  lv_event_stop_bubbling(event);
}

void ChoicePickerOverlay::finish(bool commit, int selected_override) {
  if (!_source || _finishing) return;
  _finishing = true;
  if (selected_override >= 0) _selected = wrapIndex(selected_override);
  IChoicePickerSource* const source = _source;
  if (commit && _selected != _original) source->choicePickerCommit(_selected);
  source->choicePickerClosed(commit);
  emitEvent(UiEventType::ChoicePickerClose);
}

}  // namespace heltec::meshcore::ui
