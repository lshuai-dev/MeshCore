#include "ui/theme/ui_theme_metrics.hpp"

namespace heltec::meshcore::ui {

lv_style_prop_t UI_PROP_THEME_METRICS = LV_STYLE_PROP_INV;

namespace {

const UiThemeMetrics kDefaultThemeMetrics = {
    {2, 0, 0, 0, 0, 2},
    {12, 4, 14, 0, -1, 0},
    {12, 1, 1, 1, 1, 3, 0},
    {100, 100, 8, 104},
    {0, 2, 12},
    {0, 560, 380, 4, 2, 2, 2, 3, 6, 0, 16, 8, 36},
};

}  // namespace

void ui_theme_metrics_init() {
  if (UI_PROP_THEME_METRICS == LV_STYLE_PROP_INV) {
    UI_PROP_THEME_METRICS =
        lv_style_register_prop(LV_STYLE_PROP_INHERIT | LV_STYLE_PROP_LAYOUT_REFR);
  }
}

const UiThemeMetrics& ui_theme_metrics(const lv_obj_t* obj, lv_part_t part) {
  if (!obj || UI_PROP_THEME_METRICS == LV_STYLE_PROP_INV) {
    return kDefaultThemeMetrics;
  }
  const lv_style_value_t value =
      lv_obj_get_style_prop(obj, part, UI_PROP_THEME_METRICS);
  return value.ptr ? *static_cast<const UiThemeMetrics*>(value.ptr)
                   : kDefaultThemeMetrics;
}

}  // namespace heltec::meshcore::ui
