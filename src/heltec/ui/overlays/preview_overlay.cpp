#include "preview_overlay.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

_lv_obj_t* PreviewOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::PreviewOverlayRoot);
}

_lv_obj_t* PreviewOverlay::create(lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;
  const lv_coord_t gap =
#if defined(HELTEC_V4_R8_TFT)
      LV_DPX(10);
#else
      3;
#endif
  lv_obj_set_style_pad_all(_root, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, gap, LV_PART_MAIN);

  lv_obj_t* row = ht_obj_create(_root, meta_id::PreviewHeader);
  if (!row) return nullptr;
  lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  _title = ht_label_create(row, meta_id::PreviewTitle);
  _age = ht_label_create(row, meta_id::PreviewAge);
  if (!_title || !_age) return nullptr;

  _origin = ht_label_create(_root, meta_id::PreviewOrigin);
  _text = ht_label_create(_root, meta_id::PreviewText);
  if (!_origin || !_text) return nullptr;
  lv_obj_set_width(_origin, lv_pct(100));
  lv_label_set_long_mode(_origin, LV_LABEL_LONG_DOT);
  lv_obj_set_width(_text, lv_pct(100));
  lv_obj_set_flex_grow(_text, 1);
  lv_label_set_long_mode(_text, LV_LABEL_LONG_WRAP);
  lv_obj_add_event_cb(_text, [](lv_event_t* e) {
    auto* self = static_cast<PreviewOverlay*>(lv_event_get_user_data(e));
    if (!self) return;
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
    self->dismissByUser();
  }, LV_EVENT_CLICKED, this);

  _footer = ht_label_create(_root, meta_id::PreviewFooter);
  if (!_footer) return nullptr;
  lv_obj_set_width(_footer, lv_pct(100));
  lv_label_set_long_mode(_footer, LV_LABEL_LONG_WRAP);
  lv_label_set_text(_footer, "Short press: dismiss  Long press: dismiss");

  return _root;
}

_lv_obj_t* PreviewOverlay::focusTarget() const {
  return _text;
}

bool PreviewOverlay::onKey(uint32_t key) {
  if (key == LV_KEY_ENTER) return true;
  dismissByUser();
  return true;
}

void PreviewOverlay::applyContent(uint8_t unread, uint32_t age_sec, const char* origin, const char* text) {
  if (!_root) return;
  char title[16];
  char age[12];
  lv_snprintf(title, sizeof(title), "Unread:%u", (unsigned)unread);
  if (age_sec < 60) {
    lv_snprintf(age, sizeof(age), "%us", (unsigned)age_sec);
  } else if (age_sec < 3600) {
    lv_snprintf(age, sizeof(age), "%um", (unsigned)(age_sec / 60));
  } else {
    lv_snprintf(age, sizeof(age), "%uh", (unsigned)(age_sec / 3600));
  }

  lv_label_set_text(_title, title);
  lv_label_set_text(_age, age);
  lv_label_set_text(_origin, origin ? origin : "");
  lv_label_set_text(_text, text ? text : "");
}

void PreviewOverlay::dismissByUser() {
  emitEvent(UiEventType::PreviewClose);
}

}  // namespace heltec::meshcore::ui
