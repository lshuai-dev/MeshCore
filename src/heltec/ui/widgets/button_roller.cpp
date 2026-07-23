#include "button_roller.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/menus/context_menu_metrics.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "MeshCore.h"

namespace heltec::meshcore::ui {
namespace {

lv_style_t s_roller_root_style;
lv_style_t s_roller_button_style;
lv_style_t s_roller_button_selected_style;
lv_style_t s_roller_label_style;
lv_style_t s_roller_label_selected_style;
bool s_roller_styles_ready = false;

static const lv_style_selector_t kRollerItemSelectedSelectors[] = {
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUSED),
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
    LV_PART_MAIN | LV_STATE_PRESSED,
};

void initRollerStyles(lv_obj_t* obj) {
  if (s_roller_styles_ready) return;
  const UiButtonRollerMetrics& metrics = ui_button_roller_metrics(obj);
  const lv_coord_t border_w = metrics.border_width;

  lv_style_init(&s_roller_root_style);
  lv_style_set_border_width(&s_roller_root_style, border_w);
  lv_style_set_bg_opa(&s_roller_root_style, LV_OPA_TRANSP);

  lv_style_init(&s_roller_button_style);
  lv_style_set_border_width(&s_roller_button_style, border_w);
  lv_style_set_bg_opa(&s_roller_button_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_roller_button_style, ui_color_fg());
  lv_style_set_shadow_width(&s_roller_button_style, 0);
  lv_style_set_radius(&s_roller_button_style, 0);

  lv_style_init(&s_roller_button_selected_style);
  lv_style_set_border_width(&s_roller_button_selected_style, 0);
  lv_style_set_outline_width(&s_roller_button_selected_style, 0);
  lv_style_set_shadow_width(&s_roller_button_selected_style, 0);
  lv_style_set_radius(&s_roller_button_selected_style, 0);
  lv_style_set_bg_color(&s_roller_button_selected_style, ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_roller_button_selected_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_roller_button_selected_style, ui_color_highlight_fg());

  lv_style_init(&s_roller_label_style);
  lv_style_set_text_color(&s_roller_label_style, ui_color_fg());
  lv_style_set_text_line_space(&s_roller_label_style, 0);
  lv_style_set_text_letter_space(&s_roller_label_style, 0);

  lv_style_init(&s_roller_label_selected_style);
  lv_style_set_text_color(&s_roller_label_selected_style, ui_color_highlight_fg());
  lv_style_set_text_line_space(&s_roller_label_selected_style, 0);
  lv_style_set_text_letter_space(&s_roller_label_selected_style, 0);

  s_roller_styles_ready = true;
}

void styleRollerRoot(lv_obj_t* obj) {
  if (!obj) return;
  initRollerStyles(obj);
  lv_obj_add_style(obj, &s_roller_root_style, LV_PART_MAIN);
}

void styleRollerButton(lv_obj_t* obj) {
  if (!obj) return;
  initRollerStyles(obj);
  lv_obj_add_style(obj, &s_roller_button_style, LV_PART_MAIN);
  for (lv_style_selector_t selector : kRollerItemSelectedSelectors) {
    lv_obj_add_style(obj, &s_roller_button_selected_style, selector);
  }
}

void styleRollerLabel(lv_obj_t* obj) {
  if (!obj) return;
  initRollerStyles(obj);
  lv_obj_add_style(obj, &s_roller_label_style, LV_PART_MAIN);
  for (lv_style_selector_t selector : kRollerItemSelectedSelectors) {
    lv_obj_add_style(obj, &s_roller_label_selected_style, selector);
  }
}

lv_obj_t* itemLabel(lv_obj_t* item) {
  return item && lv_obj_get_child_cnt(item) > 0 ? lv_obj_get_child(item, 0) : nullptr;
}

void setItemSelected(lv_obj_t* item, bool selected) {
  if (!item) return;
  if (selected) {
    lv_obj_add_state(item, LV_STATE_CHECKED);
    if (lv_obj_t* label = itemLabel(item)) lv_obj_add_state(label, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(item, LV_STATE_CHECKED);
    if (lv_obj_t* label = itemLabel(item)) lv_obj_clear_state(label, LV_STATE_CHECKED);
  }
}

void clearItemFocusState(lv_obj_t* item) {
  if (!item) return;
  lv_obj_clear_state(item, LV_STATE_CHECKED | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
  if (lv_obj_t* label = itemLabel(item)) {
    lv_obj_clear_state(label, LV_STATE_CHECKED | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
  }
}

}  // namespace

bool ui_button_roller_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;

  switch (ht_id(obj)) {
    case meta_id::ButtonRollerRoot:
      styleRollerRoot(obj);
      return true;
    case meta_id::ButtonRollerItem:
      styleRollerButton(obj);
      return true;
    case meta_id::ButtonRollerLabel:
      styleRollerLabel(obj);
      return true;
    default:
      return false;
  }
}

void ButtonRoller::layoutItems(uint8_t focus_index, lv_anim_enable_t anim , bool is_checked) {
  if (_root && _count > 0 && _group) {
    lv_obj_t* focus_obj = lv_group_get_focused(_group);
    if (focus_obj && lv_obj_get_parent(focus_obj) == _root) {
      focus_index = lv_obj_get_index(focus_obj);
    }
    if (_items[focus_index]) {
      const lv_coord_t btn_px = ui_button_roller_metrics(_root).button_px;
      const uint16_t ms = ui_context_menu_metrics(_root).page_anim_ms;
      auto const anim_set_y = +[](void* obj, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t*>(obj), v);
      };

      const lv_coord_t vp_h = lv_obj_get_content_height(_root);
      const lv_coord_t row_h = btn_px > 0 ? btn_px : vp_h;
      if (row_h <= 0 || vp_h < row_h) {
        if (anim == LV_ANIM_OFF && !is_checked) _last_vp_h = 0;
        return;
      }

      if (anim == LV_ANIM_OFF && !is_checked && vp_h == _last_vp_h) return;

      if (anim != LV_ANIM_OFF && is_checked && focus_index == _last_layout_focus && vp_h == _last_vp_h) {
        setItemSelected(_items[focus_index], true);
        return;
      }

      uint8_t maxVisible = 1;
      if (vp_h > row_h) {
        maxVisible = static_cast<uint8_t>((vp_h - row_h) / row_h + 1);
        if (maxVisible < 1) maxVisible = 1;
      }
      uint8_t visibleItems = LV_MIN(maxVisible, _count);
      if (visibleItems < 1) visibleItems = 1;

      const lv_coord_t center_offset = (vp_h - row_h) / 2;

      uint8_t i_max = (visibleItems - 1) / 2;
      int8_t i_min = (0 != i_max) ? static_cast<int8_t>(-i_max) : 0;
      if ((visibleItems - 1) % 2) --i_min;

      for (int8_t i = i_min; i <= i_max; ++i) {
        int8_t index = static_cast<int8_t>((focus_index + i) % _count);
        if (index < 0) index += static_cast<int8_t>(_count);

        const lv_coord_t target_y = center_offset + i * row_h;
        lv_anim_del(_items[index], anim_set_y);
        if (anim == LV_ANIM_OFF || ms == 0) {
          lv_obj_set_y(_items[index], target_y);
          if (is_checked && 0 == i) setItemSelected(_items[index], true);
          continue;
        }

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, _items[index]);
        if (is_checked && 0 == i) {
          lv_anim_set_ready_cb(&a, [](lv_anim_t* a) {
            lv_obj_t* btn = static_cast<lv_obj_t*>(a->var);
            if (btn) setItemSelected(btn, true);
          });
        }
        lv_anim_set_exec_cb(&a, anim_set_y);
        lv_anim_set_values(&a, lv_obj_get_y(_items[index]), target_y);
        lv_anim_set_time(&a, ms);
        lv_anim_start(&a);
      }
      _last_vp_h = vp_h;
      _last_layout_focus = focus_index;
    }
  }
}

lv_obj_t* ButtonRoller::addItem(const char* label) {
  if (!_root || _count >= kMaxItems || !_group) return nullptr;

  lv_obj_t* const btn = ht_btn_create(_root, meta_id::ButtonRollerItem);
  if (!btn) return nullptr;
  const UiButtonRollerMetrics& metrics = ui_button_roller_metrics(_root);
  lv_obj_set_size(btn, lv_pct(100),
                  metrics.button_px > 0 ? metrics.button_px : LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(btn, metrics.pad, LV_PART_MAIN);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t* const lbl = ht_label_create(btn, meta_id::ButtonRollerLabel, label ? label : "");
  if (lbl) {
    lv_obj_set_size(lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
  }

  lv_obj_add_event_cb(btn, [](lv_event_t* e) {
    ButtonRoller* const roller = static_cast<ButtonRoller*>(lv_event_get_user_data(e));
    lv_obj_t* const target = lv_event_get_target(e);
    if (roller && target && roller->root()) {
      roller->layoutItems(lv_obj_get_index(target), LV_ANIM_ON, true);
    }
  }, LV_EVENT_FOCUSED, this);

  lv_obj_add_event_cb(btn, [](lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    if (btn) setItemSelected(btn, false);
  }, LV_EVENT_DEFOCUSED, this);

  _items[_count++] = btn;
  lv_group_add_obj(_group, btn);
  return btn;
}

bool ButtonRoller::create(lv_obj_t* parent, lv_group_t* group) {
  if (!parent || !group) return false;
  if (_root) return true;

  _group = group;
  _root = ht_obj_create(parent, meta_id::ButtonRollerRoot);
  if (!_root) return false;
  const UiButtonRollerMetrics& metrics = ui_button_roller_metrics(_root);
  lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(_root, metrics.pad, LV_PART_MAIN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

  lv_obj_add_event_cb(_root, [](lv_event_t* e) {
    if (LV_EVENT_SIZE_CHANGED == lv_event_get_code(e)) {
      if (lv_event_get_target(e) == lv_event_get_current_target(e)) {
        ButtonRoller* const roller = static_cast<ButtonRoller*>(lv_event_get_user_data(e));
        if (roller && roller->root() && roller->itemCount() > 0) {
          roller->layoutItems(0, LV_ANIM_OFF, false);
        }
      }
    }
  }, LV_EVENT_SIZE_CHANGED, this);

  return true;
}

void ButtonRoller::resetFocus(bool focus_first_item) {
  if (!_group || _count == 0) return;
  for (uint8_t i = 0; i < _count; ++i) {
    if (_items[i]) {
      clearItemFocusState(_items[i]);
    }
  }
  _last_layout_focus = 0xFF;
  _last_vp_h = 0;
  if (focus_first_item) {
    lv_group_focus_obj(_items[0]);
    layoutItems(0, LV_ANIM_OFF, false);
  } else {
    lv_group_focus_obj(nullptr);
    layoutItems(0, LV_ANIM_OFF, false);
  }
}

bool ButtonRoller::focusItem(uint8_t index, bool checked) {
  if (!_group || index >= _count || !_items[index]) return false;
  for (uint8_t i = 0; i < _count; ++i) {
    if (!_items[i]) continue;
    clearItemFocusState(_items[i]);
  }
  _last_layout_focus = 0xFF;
  _last_vp_h = 0;
  lv_group_focus_obj(_items[index]);
  layoutItems(index, LV_ANIM_OFF, checked);
  return true;
}

}  // namespace heltec::meshcore::ui
