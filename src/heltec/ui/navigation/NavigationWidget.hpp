#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

typedef struct
{
    lv_obj_t obj;
    lv_obj_t *content;
} ht_gridNav_t;

extern const lv_obj_class_t ht_gridNav_class;

lv_obj_t *ht_gridNav_create(lv_obj_t *parent, void *user_data = nullptr);

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
bool ui_gridNav_is(lv_obj_t *obj);
lv_obj_t *ui_gridNav_content(lv_obj_t *obj);

} // namespace Heltec::MeshCore::Ui
