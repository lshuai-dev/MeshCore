#pragma once

#include "ui/core/ht_meta_data.hpp"

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui
{

typedef struct
{
    lv_obj_t obj;
} ht_navigator_t;

extern const lv_obj_class_t ht_navigator_class;

lv_obj_t *ht_navigator_create(lv_obj_t *parent, void *user_data = nullptr);

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

lv_obj_t *ui_navigator_create_grid(lv_obj_t *parent);
bool ui_navigator_is(lv_obj_t *obj);
bool ui_navigator_is_grid(lv_obj_t *obj);
lv_obj_t *ui_navigator_content(lv_obj_t *obj);

} // namespace heltec::meshcore::ui
