#pragma once

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

bool applyWidgetTheme(_lv_obj_t *obj);
void applyActiveScreenTheme(_lv_obj_t *obj);

void applyContextMenuTitleTheme(_lv_obj_t *title);

} // namespace Heltec::MeshCore::Ui
