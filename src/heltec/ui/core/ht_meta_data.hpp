#pragma once

#include <lvgl.h>
#include <stdint.h>

#if !LV_USE_USER_DATA
#error "MetaData requires LV_USE_USER_DATA=1"
#endif

namespace heltec::meshcore::ui {

using MetaId = uint16_t;

enum class MetaIdScope : MetaId {
  None = 0,
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

constexpr MetaId ht_meta_id(MetaIdScope scope, uint8_t offset = 0) {
  return static_cast<MetaId>(static_cast<MetaId>(scope) + offset);
}

namespace meta_id {
constexpr MetaId None = 0;
}

void ht_set_meta_id(lv_obj_t* obj, MetaId id);
MetaId ht_id(const lv_obj_t* obj);
void* ht_user_data(const lv_obj_t* obj);
void ht_set_user_data(lv_obj_t* obj, void* user_data);

lv_obj_t* ht_obj_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
lv_obj_t* ht_label_create(lv_obj_t* parent, MetaId id, const char* text = nullptr,
                          void* user_data = nullptr);
lv_obj_t* ht_btn_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);

#if LV_USE_SWITCH
lv_obj_t* ht_switch_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_SLIDER
lv_obj_t* ht_slider_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_DROPDOWN
lv_obj_t* ht_dropdown_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_ROLLER
lv_obj_t* ht_roller_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_TEXTAREA
lv_obj_t* ht_textarea_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_KEYBOARD
lv_obj_t* ht_keyboard_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_IMG
lv_obj_t* ht_img_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_IMGBTN
lv_obj_t* ht_imgbtn_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

#if LV_USE_MENU
lv_obj_t* ht_menu_create(lv_obj_t* parent, MetaId id, void* user_data = nullptr);
#endif

}  // namespace heltec::meshcore::ui
