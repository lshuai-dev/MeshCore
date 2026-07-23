// UI icon API — bitmap data in variants/<board>/images.cpp
#pragma once
#include "lvgl.h"

extern const lv_img_dsc_t icon_home_img;
extern const lv_img_dsc_t icon_advert_img;
extern const lv_img_dsc_t icon_compass_img;
extern const lv_img_dsc_t icon_gps_img;
extern const lv_img_dsc_t icon_radio_img;
extern const lv_img_dsc_t icon_findfriend_img;
extern const lv_img_dsc_t icon_recent_img;
extern const lv_img_dsc_t icon_system_img;
#if defined(HELTEC_V4_R8_TFT)
extern const lv_img_dsc_t icon_map_img;
#endif
extern const lv_img_dsc_t icon_meshcore_log_img;

#if defined(HELTEC_V4_R8_TFT)
extern const lv_img_dsc_t icon_meshcore_logo_alpha_img;
extern const lv_img_dsc_t ui_background_img;
#endif

extern const lv_img_dsc_t icon_home_nav_img;
extern const lv_img_dsc_t icon_compass_nav_img;
extern const lv_img_dsc_t icon_gps_nav_img;
extern const lv_img_dsc_t icon_radio_nav_img;
extern const lv_img_dsc_t icon_findfriend_nav_img;
extern const lv_img_dsc_t icon_recent_nav_img;
extern const lv_img_dsc_t icon_system_nav_img;
#if defined(HELTEC_V4_R8_TFT)
extern const lv_img_dsc_t icon_map_nav_img;
#endif

const lv_img_dsc_t* nav_ring_icon_for(const lv_img_dsc_t* screen_icon);

/** 5×7 1-bit alpha compass rose letters (recolor = fg on dial). */
extern const lv_img_dsc_t compass_cardinal_n_img;
extern const lv_img_dsc_t compass_cardinal_e_img;
extern const lv_img_dsc_t compass_cardinal_s_img;
extern const lv_img_dsc_t compass_cardinal_w_img;
