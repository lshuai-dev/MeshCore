#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

extern const lv_obj_class_t ht_gridNav_class;

/**
 * Static navigation item description used during UI construction.
 *
 * The array is owned by the caller and is only consumed while the
 * navigation surface is configured; NavigationPane does not retain it.
 */
struct UiNavigationItem
{
    uint8_t screen_index;
    const char *label;
    const lv_img_dsc_t *icon;
    bool footer;
};

lv_obj_t *ui_gridNav_create_grid(lv_obj_t *parent);
lv_obj_t *ui_gridNav_content(lv_obj_t *obj);

} // namespace Heltec::MeshCore::Ui
