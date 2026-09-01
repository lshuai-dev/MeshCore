#pragma once

#include <lvgl.h>

#if !LV_USE_USER_DATA
#error "UI object factories require LV_USE_USER_DATA=1"
#endif

namespace heltec::meshcore::ui
{

void *ht_user_data(const lv_obj_t *obj);
void ht_set_user_data(lv_obj_t *obj, void *user_data);

lv_obj_t *ht_obj_create(lv_obj_t *parent, void *user_data = nullptr);
lv_obj_t *ht_label_create(lv_obj_t *parent, const char *text = nullptr,
                          void *user_data = nullptr);
lv_obj_t *ht_btn_create(lv_obj_t *parent, void *user_data = nullptr);

#if LV_USE_SWITCH
lv_obj_t *ht_switch_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_SLIDER
lv_obj_t *ht_slider_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_DROPDOWN
lv_obj_t *ht_dropdown_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_ROLLER
lv_obj_t *ht_roller_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_TEXTAREA
lv_obj_t *ht_textarea_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_KEYBOARD
lv_obj_t *ht_keyboard_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_IMG
lv_obj_t *ht_img_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_IMGBTN
lv_obj_t *ht_imgbtn_create(lv_obj_t *parent, void *user_data = nullptr);
#endif
#if LV_USE_MENU
lv_obj_t *ht_menu_create(lv_obj_t *parent, void *user_data = nullptr);
lv_obj_t *ht_menu_page_create(lv_obj_t *parent, const char *title,
                              void *user_data = nullptr);
#endif
#if LV_USE_TILEVIEW
lv_obj_t *ht_tileview_create(lv_obj_t *parent, void *user_data = nullptr);
lv_obj_t *ht_tileview_add_tile(lv_obj_t *tileview, uint8_t col_id, uint8_t row_id, lv_dir_t dir,
                               void *user_data = nullptr);
#endif

} // namespace heltec::meshcore::ui
