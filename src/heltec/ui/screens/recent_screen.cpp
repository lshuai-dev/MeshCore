#include "recent_screen.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_events.h"
#include <lvgl.h>

namespace heltec::meshcore::ui {

_lv_obj_t* RecentScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::RecentScreenRoot);
}

namespace {

void format_age(int32_t secs, char* buf, size_t buf_size) {
  if (secs < 60) {
    lv_snprintf(buf, buf_size, "%ds", (int)secs);
  } else if (secs < 3600) {
    lv_snprintf(buf, buf_size, "%dm", (int)(secs / 60));
  } else {
    lv_snprintf(buf, buf_size, "%dh", (int)(secs / 3600));
  }
}

}  // namespace

_lv_obj_t* RecentScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  _scroll = _root;
  lv_obj_add_flag(_scroll, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(_scroll, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(_scroll, LV_SCROLLBAR_MODE_OFF);

  for (int i = 0; i < kMaxRows; ++i) {
    _rows[i] = ht_label_create(_scroll, meta_id::RecentRowLabel, "");
    if (!_rows[i]) continue;
    lv_label_set_text_static(_rows[i], _row_text[i]);
    lv_obj_set_width(_rows[i], lv_pct(100));
    lv_obj_clear_flag(_rows[i], LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(_rows[i], LV_LABEL_LONG_CLIP);
  }

  lv_group_set_wrap(group(), true);
  return _scroll;
}

void RecentScreen::refreshRecent() {
  biz::IBizFacade::RecentHeardItem items[kMaxRows];
  const int n = _biz.fillRecentHeard(items, kMaxRows);

  if (n == 0) {
    if (_rows[0]) {
      lv_snprintf(_row_text[0], sizeof(_row_text[0]), "(no recent)");
      lv_label_set_long_mode(_rows[0], LV_LABEL_LONG_DOT);
      lv_label_set_text_static(_rows[0], _row_text[0]);
      lv_obj_clear_flag(_rows[0], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 1; i < kMaxRows; ++i) {
      if (_rows[i]) {
        _row_text[i][0] = '\0';
        lv_label_set_text_static(_rows[i], _row_text[i]);
        lv_obj_add_flag(_rows[i], LV_OBJ_FLAG_HIDDEN);
      }
    }
    return;
  }

  for (int i = 0; i < kMaxRows; ++i) {
    if (!_rows[i]) continue;
    if (i < n) {
      char age[16];
      format_age(items[i].age_seconds, age, sizeof(age));
      lv_snprintf(_row_text[i], sizeof(_row_text[i]), "%s  %s", items[i].name, age);
      lv_label_set_long_mode(_rows[i], LV_LABEL_LONG_DOT);
      lv_label_set_text_static(_rows[i], _row_text[i]);
      lv_obj_clear_flag(_rows[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      _row_text[i][0] = '\0';
      lv_label_set_text_static(_rows[i], _row_text[i]);
      lv_obj_add_flag(_rows[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void RecentScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::RecentHeardChanged) {
    refreshRecent();
  }
}

void RecentScreen::onRefreshRequested() { refreshRecent(); }

}  // namespace heltec::meshcore::ui
