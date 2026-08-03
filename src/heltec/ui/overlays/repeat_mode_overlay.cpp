#include "repeat_mode_overlay.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/biz_facade.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_events.h"

#include <lvgl.h>

namespace heltec::meshcore::ui {
namespace {

const char* apply_error_text(biz::IBizFacade::ForwardingApplyResult result) {
  switch (result) {
    case biz::IBizFacade::ForwardingApplyResult::InvalidSelection:
      return "Invalid frequency";
    case biz::IBizFacade::ForwardingApplyResult::UnsupportedFrequency:
      return "Frequency unsupported";
    case biz::IBizFacade::ForwardingApplyResult::InvalidRadioParams:
      return "Invalid radio params";
    case biz::IBizFacade::ForwardingApplyResult::Unavailable:
      return "Repeat mode unavailable";
    case biz::IBizFacade::ForwardingApplyResult::Ok:
      return nullptr;
  }
  return "Repeat mode failed";
}

void format_frequency(char* buffer, size_t capacity, uint32_t khz) {
  const uint32_t whole = khz / 1000u;
  const uint32_t fraction = khz % 1000u;
  if (fraction == 0) {
    lv_snprintf(buffer, capacity, "%u.0", (unsigned)whole);
  } else {
    lv_snprintf(buffer, capacity, "%u.%03u", (unsigned)whole,
                (unsigned)fraction);
  }
}

bool format_frequency_item(const biz::IBizFacade& biz, int index, char* buffer,
                           size_t capacity) {
  uint32_t lower_khz = 0;
  uint32_t upper_khz = 0;
  if (!buffer || capacity == 0 ||
      !biz.forwardingFrequencyRange(index, &lower_khz, &upper_khz)) {
    return false;
  }

  char lower[16]{};
  format_frequency(lower, sizeof(lower), lower_khz);
  if (lower_khz == upper_khz) {
    lv_snprintf(buffer, capacity, "%s MHz", lower);
    return true;
  }

  char upper[16]{};
  format_frequency(upper, sizeof(upper), upper_khz);
  lv_snprintf(buffer, capacity, "%s-%s MHz", lower, upper);
  return true;
}

#if !defined(HELTEC_V4_R8_TFT)
void style_item(_lv_obj_t* item) {
  if (!item) return;
  static const lv_state_t selected_states[] = {
      LV_STATE_CHECKED,
      LV_STATE_FOCUSED,
      LV_STATE_FOCUS_KEY,
      LV_STATE_PRESSED,
      static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUSED),
      static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
  };

  lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(item, 0, LV_PART_MAIN);
  lv_obj_set_style_text_color(item, ui_color_overlay_fg(), LV_PART_MAIN);
  for (lv_state_t state : selected_states) {
    const lv_style_selector_t selector = LV_PART_MAIN | state;
    lv_obj_set_style_bg_color(item, ui_color_highlight_bg(), selector);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, selector);
    lv_obj_set_style_text_color(item, ui_color_highlight_fg(), selector);
  }

  _lv_obj_t* const label = lv_obj_get_child(item, 0);
  if (!label) return;
  lv_obj_set_style_text_color(label, ui_color_overlay_fg(), LV_PART_MAIN);
  for (lv_state_t state : selected_states) {
    lv_obj_set_style_text_color(label, ui_color_highlight_fg(),
                                LV_PART_MAIN | state);
  }
  lv_obj_center(label);
}
#endif

}  // namespace

_lv_obj_t* RepeatModeOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::RepeatModeOverlayRoot);
}

_lv_obj_t* RepeatModeOverlay::create(_lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;

  lv_obj_set_style_pad_hor(_root, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(_root, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, LV_DPX(4), LV_PART_MAIN);

  _lv_obj_t* const title =
      ht_label_create(_root, meta_id::RepeatModeTitle, "Repeat frequency");
  if (!title) return nullptr;
  lv_obj_set_width(title, lv_pct(100));
  lv_obj_set_style_text_color(title, ui_color_overlay_fg(), LV_PART_MAIN);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  _lv_obj_t* const list = ht_obj_create(_root, meta_id::RepeatModeList);
  if (!list) return nullptr;
  lv_obj_set_width(list, lv_pct(100));
  lv_obj_set_flex_grow(list, 1);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
  lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(list, LV_OBJ_FLAG_EVENT_BUBBLE);

#if defined(HELTEC_V4_R8_TFT)
#if LV_USE_ROLLER == 0
#error "HELTEC_V4_R8_TFT repeat mode overlay requires LV_USE_ROLLER=1"
#endif
  _touch_roller = lv_roller_create(list);
  if (!_touch_roller) return nullptr;
  lv_obj_remove_style_all(_touch_roller);
  lv_obj_set_width(_touch_roller, lv_pct(100));
  lv_obj_set_style_bg_opa(_touch_roller, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(_touch_roller, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(_touch_roller, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(_touch_roller, 0, LV_PART_MAIN);
  lv_obj_set_style_text_color(_touch_roller, ui_color_overlay_fg(), LV_PART_MAIN);
  lv_obj_set_style_text_align(_touch_roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  // The native roller uses text line spacing as the distance between frequency rows.
  lv_obj_set_style_text_line_space(_touch_roller, LV_DPX(18), LV_PART_MAIN);
  lv_obj_set_style_anim_time(_touch_roller, 220, LV_PART_MAIN);
  lv_obj_set_style_bg_color(_touch_roller, ui_color_highlight_bg(), LV_PART_SELECTED);
  lv_obj_set_style_bg_opa(_touch_roller, LV_OPA_COVER, LV_PART_SELECTED);
  lv_obj_set_style_text_color(_touch_roller, ui_color_highlight_fg(), LV_PART_SELECTED);
  lv_obj_set_style_radius(_touch_roller, LV_DPX(4), LV_PART_SELECTED);
  lv_obj_add_event_cb(_touch_roller, onRollerEvent, LV_EVENT_ALL, this);
#else
  if (!_roller.create(list, group())) return nullptr;
  lv_obj_add_flag(_roller.root(), LV_OBJ_FLAG_EVENT_BUBBLE);
#endif

  const int available = _biz.forwardingFrequencyCount();
  for (int index = 0;
       index < available && _frequency_count < kMaxFrequencyItems; ++index) {
    char* const label = _frequency_labels[_frequency_count];
    if (!format_frequency_item(_biz, index, label,
                               sizeof(_frequency_labels[_frequency_count]))) {
      continue;
    }

#if !defined(HELTEC_V4_R8_TFT)
    _lv_obj_t* const button = _roller.addItem(label);
    if (!button) return nullptr;
    _frequency_buttons[_frequency_count] = button;
    style_item(button);
    lv_obj_add_event_cb(button, onItemClicked, LV_EVENT_CLICKED, this);
#endif
    _frequency_indices[_frequency_count] = static_cast<int8_t>(index);
    ++_frequency_count;
  }

#if defined(HELTEC_V4_R8_TFT)
  size_t used = 0;
  for (uint8_t i = 0; i < _frequency_count && used < sizeof(_roller_options); ++i) {
    const int written = lv_snprintf(
        _roller_options + used, sizeof(_roller_options) - used, "%s%s",
        _frequency_labels[i], i + 1u < _frequency_count ? "\n" : "");
    if (written < 0 || static_cast<size_t>(written) >= sizeof(_roller_options) - used) {
      break;
    }
    used += static_cast<size_t>(written);
  }
  if (_frequency_count == 0) {
    lv_snprintf(_roller_options, sizeof(_roller_options), "No frequency");
  }
  // Keep the three choices visible around the current one and preserve the
  // previous ButtonRoller's wrap-around navigation.
  lv_roller_set_options(_touch_roller, _roller_options, LV_ROLLER_MODE_INFINITE);
  lv_roller_set_visible_row_count(
      _touch_roller, _frequency_count > 0 ? _frequency_count : 1);
#endif

  return _root;
}

_lv_obj_t* RepeatModeOverlay::focusTarget() const {
#if defined(HELTEC_V4_R8_TFT)
  return _touch_roller;
#else
  return nullptr;
#endif
}

void RepeatModeOverlay::onEnter() {
  _applying = false;
  AbstractOverlay::onEnter();
  setApplying(false);

  int selected_item = 0;
  const int current_frequency = _biz.currentForwardingFrequencyIndex();
  for (uint8_t i = 0; i < _frequency_count; ++i) {
    if (_frequency_indices[i] == current_frequency) {
      selected_item = i;
      break;
    }
  }
  if (_frequency_count > 0) {
#if defined(HELTEC_V4_R8_TFT)
    if (_touch_roller) {
      lv_roller_set_selected(_touch_roller, static_cast<uint16_t>(selected_item),
                             LV_ANIM_OFF);
    }
#else
    (void)_roller.focusItem(static_cast<uint8_t>(selected_item), true);
#endif
  } else {
#if !defined(HELTEC_V4_R8_TFT)
    (void)_roller.focusItem(0, true);
#endif
  }
}

bool RepeatModeOverlay::onKey(uint32_t key) {
  if (key != LV_KEY_ESC) return false;
  cancelSelection();
  return true;
}

#if defined(HELTEC_V4_R8_TFT)

bool RepeatModeOverlay::hitRoller(int16_t x, int16_t y) const {
  if (!_touch_roller || !lv_obj_is_valid(_touch_roller) ||
      lv_obj_has_flag(_touch_roller, LV_OBJ_FLAG_HIDDEN)) {
    return false;
  }
  lv_area_t area{};
  lv_obj_get_coords(_touch_roller, &area);
  return x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2;
}

void RepeatModeOverlay::onRollerEvent(lv_event_t* event) {
  if (!event) return;
  auto* const self =
      static_cast<RepeatModeOverlay*>(lv_event_get_user_data(event));
  _lv_obj_t* const roller = lv_event_get_target(event);
  if (!self || !roller || self->_applying || self->_frequency_count == 0) return;

  const lv_event_code_t code = lv_event_get_code(event);
  if (code != LV_EVENT_CLICKED) return;

  const auto* const state = reinterpret_cast<const lv_roller_t*>(roller);
  if (state && !state->moved) {
    self->applySelection(
        static_cast<uint8_t>(lv_roller_get_selected(roller)));
  }
}

#else

void RepeatModeOverlay::onItemClicked(lv_event_t* event) {
  if (!event || lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  auto* const self =
      static_cast<RepeatModeOverlay*>(lv_event_get_user_data(event));
  _lv_obj_t* const target = lv_event_get_target(event);
  if (!self || !target || self->_applying) return;

  for (uint8_t i = 0; i < self->_frequency_count; ++i) {
    if (target == self->_frequency_buttons[i]) {
      self->applySelection(i);
      return;
    }
  }
}

#endif

void RepeatModeOverlay::applySelection(uint8_t item_index) {
  if (_applying || item_index >= _frequency_count) return;
  setApplying(true);

  const biz::IBizFacade::ForwardingApplyResult result =
      _biz.setForwardingEnabled(true, _frequency_indices[item_index]);
  const char* const error = apply_error_text(result);
  if (!error && _biz.forwardingEnabled()) {
    _biz.showAlert("Repeat mode enabled", 1200);
    (void)emitEvent(UiEventType::RepeatModeClose);
    return;
  }

  setApplying(false);
  _biz.showAlert(error ? error : "Repeat mode failed", 2000);
}

void RepeatModeOverlay::cancelSelection() {
  if (_applying) return;
  (void)emitEvent(UiEventType::RepeatModeClose);
}

void RepeatModeOverlay::setApplying(bool applying) {
  _applying = applying;
#if defined(HELTEC_V4_R8_TFT)
  if (_touch_roller) {
    if (applying || _frequency_count == 0) {
      lv_obj_add_state(_touch_roller, LV_STATE_DISABLED);
    } else {
      lv_obj_clear_state(_touch_roller, LV_STATE_DISABLED);
    }
  }
#else
  for (uint8_t i = 0; i < _frequency_count; ++i) {
    if (!_frequency_buttons[i]) continue;
    if (applying) {
      lv_obj_add_state(_frequency_buttons[i], LV_STATE_DISABLED);
    } else {
      lv_obj_clear_state(_frequency_buttons[i], LV_STATE_DISABLED);
    }
  }
#endif
}

}  // namespace heltec::meshcore::ui
