#pragma once

#include <lvgl.h>

#include "ui/app/ui_app_frame_metrics.hpp"
#include "ui/menus/context_menu_metrics.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/widgets/button_roller.hpp"
#include "ui/widgets/top_pane.hpp"

namespace heltec::meshcore::ui {

struct UiQuickPingMetrics {
  lv_coord_t width_pct;
  lv_coord_t height_pct;
  lv_coord_t content_pad;
  lv_coord_t message_list_min_width;
};

struct UiThemeMetrics {
  UiAppFrameMetrics frame;
  UiTopPaneMetrics top_pane;
  UiContextMenuMetrics context_menu;
  UiQuickPingMetrics quick_ping;
  UiButtonRollerMetrics button_roller;
  UiNavigationMetrics navigation;
};

extern lv_style_prop_t UI_PROP_THEME_METRICS;

void ui_theme_metrics_init();
const UiThemeMetrics& ui_theme_metrics(const lv_obj_t* obj,
                                       lv_part_t part = LV_PART_MAIN);

inline const UiAppFrameMetrics& ui_app_frame_metrics(
    const lv_obj_t* obj, lv_part_t part = LV_PART_MAIN) {
  return ui_theme_metrics(obj, part).frame;
}

inline const UiTopPaneMetrics& ui_top_pane_metrics(
    const lv_obj_t* obj, lv_part_t part = LV_PART_MAIN) {
  return ui_theme_metrics(obj, part).top_pane;
}

inline const UiContextMenuMetrics& ui_context_menu_metrics(
    const lv_obj_t* obj, lv_part_t part = LV_PART_MAIN) {
  return ui_theme_metrics(obj, part).context_menu;
}

inline const UiQuickPingMetrics& ui_quick_ping_metrics(
    const lv_obj_t* obj, lv_part_t part = LV_PART_MAIN) {
  return ui_theme_metrics(obj, part).quick_ping;
}

inline const UiButtonRollerMetrics& ui_button_roller_metrics(
    const lv_obj_t* obj, lv_part_t part = LV_PART_MAIN) {
  return ui_theme_metrics(obj, part).button_roller;
}

inline const UiNavigationMetrics& ui_navigation_metrics(
    const lv_obj_t* obj, lv_part_t part = LV_PART_MAIN) {
  return ui_theme_metrics(obj, part).navigation;
}

}  // namespace heltec::meshcore::ui
