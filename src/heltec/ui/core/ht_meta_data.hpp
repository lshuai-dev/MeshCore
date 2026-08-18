#pragma once

#include <lvgl.h>
#include <stdint.h>

#if !LV_USE_USER_DATA
#error "MetaData requires LV_USE_USER_DATA=1"
#endif

namespace heltec::meshcore::ui
{

inline constexpr uint16_t HT_META_ID_UNSET = UINT16_MAX;
inline constexpr lv_obj_flag_t HT_OBJ_FLAG_HAS_META = LV_OBJ_FLAG_USER_4;

enum class MetaIdScope : uint16_t
{
    App = 0x0100,
    TopPane = 0x0200,
    Screen = 0x0300,
    Navigation = 0x0400,
    ContextMenu = 0x0500,
    Overlay = 0x0600,
    Map = 0x0700,
    Compass = 0x0800,
    ButtonRoller = 0x0900,
    LicenseGate = 0x0A00,
};

constexpr uint16_t ht_meta_id(MetaIdScope scope, uint8_t offset = 0)
{
    return static_cast<uint16_t>(static_cast<uint16_t>(scope) + offset);
}

uint16_t ht_id(const lv_obj_t *obj);
void *ht_user_data(const lv_obj_t *obj);
void ht_set_user_data(lv_obj_t *obj, void *user_data);

#if LV_USE_OBJECT
lv_obj_t *ht_obj_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_LABEL
lv_obj_t *ht_label_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, const char *text = nullptr, void *user_data = nullptr);
#endif

#if LV_USE_BUTTON
lv_obj_t *ht_btn_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_SWITCH
lv_obj_t *ht_switch_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_SLIDER
lv_obj_t *ht_slider_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_DROPDOWN
lv_obj_t *ht_dropdown_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_ROLLER
lv_obj_t *ht_roller_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_TEXTAREA
lv_obj_t *ht_textarea_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_KEYBOARD
lv_obj_t *ht_keyboard_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_IMG
lv_obj_t *ht_img_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_IMGBTN
lv_obj_t *ht_imgbtn_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_MENU
lv_obj_t *ht_menu_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
lv_obj_t *ht_menu_page_create(lv_obj_t *parent, const char *title, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

#if LV_USE_TILEVIEW
lv_obj_t *ht_tileview_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
lv_obj_t *ht_tileview_add_tile(lv_obj_t *tileview, uint8_t col_id, uint8_t row_id, lv_dir_t dir, uint16_t id = HT_META_ID_UNSET, void *user_data = nullptr);
#endif

} // namespace heltec::meshcore::ui
