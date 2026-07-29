#include "radio_pram_sync_overlay.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ui_events.h"
#if defined(HELTEC_V4_R8_TFT)
#include "ui/app/ui_theme.hpp"
#endif

#include <lvgl.h>

#include "heltec/ui/core/biz_facade.hpp"
#include "config/LoRaBandPresets.h"

namespace heltec::meshcore::ui {

_lv_obj_t* RadioParamSyncOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::RadioParamSyncOverlayRoot);
}

namespace {

static int wrapIndex(int idx, int count) {
  if (count <= 0) return 0;
  idx %= count;
  if (idx < 0) idx += count;
  return idx;
}

}  // namespace

_lv_obj_t* RadioParamSyncOverlay::create(lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;
  const lv_coord_t gap =
#if defined(HELTEC_V4_R8_TFT)
      LV_DPX(10);
#else
      3;
#endif
  lv_obj_set_style_pad_hor(_root, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(_root, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, gap, LV_PART_MAIN);

  _title = ht_label_create(_root, meta_id::RadioParamSyncTitle);
  if (!_title) return nullptr;
  lv_obj_set_width(_title, lv_pct(100));
  lv_label_set_text_static(_title, "Radio param preset");

  _list = ht_obj_create(_root, meta_id::RadioParamSyncList);
  if (!_list) return nullptr;
  lv_obj_set_width(_list, lv_pct(100));
  lv_obj_set_flex_grow(_list, 1);
  lv_obj_set_flex_flow(_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_list, gap, LV_PART_MAIN);
  lv_obj_clear_flag(_list, LV_OBJ_FLAG_SCROLLABLE);

  const int preset_count = _biz.loRaBandPresetCount();
  _count = static_cast<uint8_t>(preset_count > 0 ? (preset_count > 255 ? 255 : preset_count) : 0);

#if defined(HELTEC_V4_R8_TFT)
#if LV_USE_ROLLER == 0
#error "HELTEC_V4_R8_TFT radio preset overlay requires LV_USE_ROLLER=1"
#endif
  _roller = lv_roller_create(_list);
  if (!_roller) return nullptr;
  lv_obj_remove_style_all(_roller);
  lv_obj_set_width(_roller, lv_pct(100));
  lv_obj_set_style_bg_opa(_roller, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(_roller, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(_roller, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(_roller, 0, LV_PART_MAIN);
  lv_obj_set_style_text_color(_roller, ui_color_overlay_fg(), LV_PART_MAIN);
  lv_obj_set_style_text_align(_roller, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(_roller, LV_DPX(18), LV_PART_MAIN);
  lv_obj_set_style_anim_time(_roller, 220, LV_PART_MAIN);
  lv_obj_set_style_bg_color(_roller, ui_color_highlight_bg(), LV_PART_SELECTED);
  lv_obj_set_style_bg_opa(_roller, LV_OPA_COVER, LV_PART_SELECTED);
  lv_obj_set_style_text_color(_roller, ui_color_highlight_fg(), LV_PART_SELECTED);
  lv_obj_set_style_radius(_roller, LV_DPX(4), LV_PART_SELECTED);
  lv_obj_add_event_cb(
      _roller,
      [](lv_event_t* e) {
        auto* ovl = static_cast<RadioParamSyncOverlay*>(lv_event_get_user_data(e));
        lv_obj_t* roller = lv_event_get_target(e);
        if (!ovl || !roller) return;
        const lv_event_code_t code = lv_event_get_code(e);
        const uint8_t selected = static_cast<uint8_t>(lv_roller_get_selected(roller));
        if (code == LV_EVENT_PRESSED) {
          ovl->_roller_press_selected = selected;
        } else if (code == LV_EVENT_VALUE_CHANGED) {
          ovl->_select = static_cast<int8_t>(selected);
        } else if (code == LV_EVENT_CLICKED) {
          const auto* state = reinterpret_cast<const lv_roller_t*>(roller);
          if (state && !state->moved && selected == ovl->_roller_press_selected) {
            ovl->applySelection();
          }
        }
      },
      LV_EVENT_ALL, this);
#else
  for (uint8_t i = 0; i < _count; i++) {
    lv_obj_t* row = ht_label_create(_list, meta_id::RadioParamSyncRow);
    if (!row) return nullptr;
    lv_obj_set_width(row, lv_pct(100));
    lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
  }
#endif

  _footer = ht_label_create(_root, meta_id::RadioParamSyncFooter);
  if (!_footer) return nullptr;
  lv_obj_set_width(_footer, lv_pct(100));
  lv_label_set_long_mode(_footer, LV_LABEL_LONG_CLIP);
  lv_label_set_text_static(_footer, operation_hint::kRadioPreset);

#if !defined(HELTEC_V4_R8_TFT)
  lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_root, [](lv_event_t* e) {
    if (LV_EVENT_CLICKED != lv_event_get_code(e)) return;
    RadioParamSyncOverlay* ovl = static_cast<RadioParamSyncOverlay*>(lv_event_get_user_data(e));
    if (ovl) ovl->applySelection();
  }, LV_EVENT_CLICKED, this);
#endif

  _select = 0;
  configureListLayout();
  return _root;
}

_lv_obj_t* RadioParamSyncOverlay::focusTarget() const {
  return _root;
}

#if defined(HELTEC_V4_R8_TFT)
bool RadioParamSyncOverlay::hitRoller(int16_t x, int16_t y) const {
  if (!_roller || !lv_obj_is_valid(_roller) || lv_obj_has_flag(_roller, LV_OBJ_FLAG_HIDDEN)) {
    return false;
  }
  lv_area_t area{};
  lv_obj_get_coords(_roller, &area);
  return x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2;
}
#endif

bool RadioParamSyncOverlay::onKey(uint32_t key) {
  if (!_root) return false;
  if (key == LV_KEY_PREV || key == LV_KEY_LEFT) {
    stepSelection(-1);
    return true;
  }
  if (key == LV_KEY_NEXT || key == LV_KEY_RIGHT) {
    stepSelection(1);
    return true;
  }
  if (key == LV_KEY_ESC) {
    emitEvent(UiEventType::RadioSyncClose);
    return true;
  }
  return false;
}

void RadioParamSyncOverlay::stepSelection(int8_t dir) {
  if (dir == 0 || _count == 0) return;
  _select = (int8_t)wrapIndex((int)_select + (int)dir, (int)_count);
#if defined(HELTEC_V4_R8_TFT)
  if (_roller) lv_roller_set_selected(_roller, static_cast<uint16_t>(_select), LV_ANIM_ON);
#else
  renderRows();
#endif
}

void RadioParamSyncOverlay::syncFromPrefs() {
  _count = (uint8_t)_biz.loRaBandPresetCount();
  const int idx = _biz.currentLoRaBandPresetIndex();
  _select = (int8_t)wrapIndex(idx, (int)_count);
}

void RadioParamSyncOverlay::configureListLayout() {
  if (!_root || !_list || _count == 0) return;

  // Measure from the actual laid-out list area. The vertical breathing room
  // scales with the display instead of being tied to a particular board size.
  lv_obj_set_style_pad_top(_list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(_list, 0, LV_PART_MAIN);
  lv_obj_update_layout(_root);

  const lv_coord_t list_height = lv_obj_get_height(_list);
  const lv_coord_t margin = list_height > 0 ? list_height / 14 : 0;
  lv_obj_set_style_pad_top(_list, margin, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(_list, margin, LV_PART_MAIN);
  lv_obj_update_layout(_root);

  const lv_coord_t usable_height = lv_obj_get_content_height(_list);
  int visible = 1;

#if defined(HELTEC_V4_R8_TFT)
  if (_roller) {
    const lv_font_t* font = lv_obj_get_style_text_font(_roller, LV_PART_MAIN);
    const lv_coord_t line_height = font ? lv_font_get_line_height(font) : 1;
    const lv_coord_t line_space =
        lv_obj_get_style_text_line_space(_roller, LV_PART_MAIN);
    const lv_coord_t row_height = line_height + line_space;
    if (usable_height > 0 && row_height > 0) visible = usable_height / row_height;
  }
#else
  lv_obj_t* first_row = lv_obj_get_child(_list, 0);
  if (first_row) {
    const lv_font_t* font = lv_obj_get_style_text_font(first_row, LV_PART_MAIN);
    const lv_coord_t line_height = font ? lv_font_get_line_height(font) : 1;
    const lv_coord_t row_gap = lv_obj_get_style_pad_row(_list, LV_PART_MAIN);
    const lv_coord_t row_height = line_height + row_gap;
    if (usable_height > 0 && row_height > 0) {
      visible = (usable_height + row_gap) / row_height;
    }
  }
#endif

  if (visible < 1) visible = 1;
  if (visible > static_cast<int>(_count)) visible = _count;
  // A centered selector needs the same number of neighbours above and below.
  if (visible > 1 && (visible & 1) == 0) --visible;
  _visible_rows = static_cast<uint8_t>(visible);

#if defined(HELTEC_V4_R8_TFT)
  if (_roller) lv_roller_set_visible_row_count(_roller, _visible_rows);
#else
  for (uint8_t i = 0; i < _count; ++i) {
    lv_obj_t* row = lv_obj_get_child(_list, i);
    if (!row) break;
    if (i < _visible_rows) {
      lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    }
  }
#endif
}

void RadioParamSyncOverlay::renderRows() {
  if (!_root) return;
#if defined(HELTEC_V4_R8_TFT)
  if (!_roller) return;
  rebuildRollerOptions();
  lv_roller_set_selected(_roller, static_cast<uint16_t>(_select), LV_ANIM_OFF);
#else
  const int cur_idx = _biz.currentLoRaBandPresetIndex();
  const int center = static_cast<int>(_visible_rows) / 2;

  for (int vr = 0; vr < static_cast<int>(_visible_rows); vr++) {
    lv_obj_t* row = lv_obj_get_child(_list, vr);
    if (!row) break;
    const int item = wrapIndex((int)_select + (vr - center), (int)_count);
    const char* name = _biz.loRaBandPresetName(item);
    const bool is_cur = (item == cur_idx);

    if (is_cur) {
      lv_snprintf(_current_row_text, sizeof(_current_row_text), "*%s",
                  name ? name : "");
      lv_label_set_text_static(row, _current_row_text);
    } else {
      lv_label_set_text_static(row, name ? name : "");
    }

    if (vr == center) {
      lv_obj_add_state(row, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(row, LV_STATE_CHECKED);
    }
  }
#endif
}

#if defined(HELTEC_V4_R8_TFT)
void RadioParamSyncOverlay::rebuildRollerOptions() {
  if (!_roller || kRadioParamPresetUiScratchSize < 2) return;
  char* const roller_options = radioParamPresetUiScratch();

  const int cur_idx = _biz.currentLoRaBandPresetIndex();
  size_t pos = 0;
  roller_options[0] = '\0';
  for (int i = 0; i < static_cast<int>(_count); ++i) {
    const char* name = _biz.loRaBandPresetName(i);
    if (!name) name = "";
    const int written =
        lv_snprintf(roller_options + pos, kRadioParamPresetUiScratchSize - pos,
                    "%s%s%s", i > 0 ? "\n" : "", i == cur_idx ? "*" : "", name);
    if (written < 0) break;
    const size_t available = kRadioParamPresetUiScratchSize - pos;
    if (static_cast<size_t>(written) >= available) {
      pos = kRadioParamPresetUiScratchSize - 1;
      break;
    }
    pos += static_cast<size_t>(written);
  }
  roller_options[pos] = '\0';
  lv_roller_set_options(_roller, roller_options, LV_ROLLER_MODE_NORMAL);
}
#endif

void RadioParamSyncOverlay::onEnter() {
  if (!_root) return;
  _applying = false;
#if defined(HELTEC_V4_R8_TFT)
  if (_roller) lv_obj_clear_state(_roller, LV_STATE_DISABLED);
#endif
  AbstractOverlay::onEnter();
  syncFromPrefs();
  configureListLayout();
  renderRows();
}

void RadioParamSyncOverlay::onExit() {
  AbstractOverlay::onExit();
}

void RadioParamSyncOverlay::applySelection() {
  if (_count == 0 || _applying) return;
  _applying = true;
#if defined(HELTEC_V4_R8_TFT)
  if (_roller) lv_obj_add_state(_roller, LV_STATE_DISABLED);
#endif
  _biz.setLoRaBandPresetIndex((int)_select);
  _biz.showAlert("LoRa band set", 800);
  emitEvent(UiEventType::RadioSyncClose);
}

}  // namespace heltec::meshcore::ui
