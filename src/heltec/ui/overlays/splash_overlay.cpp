#include "splash_overlay.hpp"

#include "heltec/app/HeltecMesh.h"
#include "heltec/ui/images.h"
#include "ui/core/ht_meta_data.hpp"

#if defined(HELTEC_V4_R8_TFT)
#include "heltec/drivers/display/display_port.hpp"
#endif

#include <string.h>

#include <lvgl.h>

namespace heltec::meshcore::ui {

SplashOverlay::~SplashOverlay() {
  if (_root) {
    lv_obj_del(_root);
    _root = nullptr;
    _logo = nullptr;
    _ver = nullptr;
    _date = nullptr;
    _attribution = nullptr;
    _visible = false;
  }
}

bool SplashOverlay::create(_lv_obj_t* parent) {
  if (!parent) return false;

  _root = ht_obj_create(parent, meta_id::SplashOverlayRoot);
  if (!_root) return false;
  lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

  _logo = ht_img_create(_root, meta_id::SplashLogo);
  if (!_logo) return false;
#if defined(HELTEC_V4_R8_TFT)
  lv_img_set_src(_logo, &icon_meshcore_logo_alpha_img);
  lv_img_set_antialias(_logo, false);
  lv_img_set_zoom(_logo, LV_IMG_ZOOM_NONE);
#else
  lv_img_set_src(_logo, &icon_meshcore_log_img);
#endif

  _ver = ht_label_create(_root, meta_id::SplashVersion);
  if (!_ver) return false;
  lv_obj_set_width(_ver, lv_pct(100));
  lv_label_set_long_mode(_ver, LV_LABEL_LONG_CLIP);

  _date = ht_label_create(_root, meta_id::SplashDate);
  if (!_date) return false;
  lv_obj_set_width(_date, lv_pct(100));
  lv_label_set_long_mode(_date, LV_LABEL_LONG_CLIP);

  _attribution = ht_label_create(_root, meta_id::SplashAttribution);
  if (!_attribution) return false;
  lv_obj_set_width(_attribution, lv_pct(100));
  lv_label_set_long_mode(_attribution, LV_LABEL_LONG_CLIP);
  lv_label_set_text_static(_attribution, "Heltec MeshCore FW");

  printInfo();

  _visible = true;

  lv_disp_t* disp = lv_disp_get_default();
  if (disp) {
#if defined(HELTEC_V4_R8_TFT)
    heltec::meshcore::dal::display_port::setBacklightOn(true);
#endif
    lv_timer_handler();
    lv_refr_now(disp);
  }

  return true;
}

void SplashOverlay::printInfo() {
  const char* ver = FIRMWARE_VERSION;
  const char* dash = strchr(ver, '-');
  int len = dash ? (int)(dash - ver) : (int)strlen(ver);
  if (len >= (int)sizeof(_version_text)) len = (int)sizeof(_version_text) - 1;
  memcpy(_version_text, ver, (size_t)len);
  _version_text[len] = '\0';
  if (_ver) lv_label_set_text_static(_ver, _version_text);
  if (_date) lv_label_set_text_static(_date, FIRMWARE_BUILD_DATE);
}

void SplashOverlay::hide() {
  if (!_root) return;
  _visible = false;
  lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace heltec::meshcore::ui
