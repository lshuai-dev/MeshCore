#include "SplashOverlay.hpp"

#include "heltec/app/HeltecMesh.h"
#include "heltec/ui/Images.h"
#include "ui/widgets/WidgetClasses.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/DisplayPort.hpp"
#endif

#include <lvgl.h>
#include <string.h>

namespace Heltec::MeshCore::Ui
{

SplashOverlay::~SplashOverlay()
{
    if (_root)
    {
        lv_obj_del(_root);
        _root = nullptr;
    }
}

bool SplashOverlay::create(_lv_obj_t *parent)
{
    if (!parent)
        return false;

    _root = ht_class_create(&ht_splash_overlay_class, parent);
    if (!_root)
        return false;
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *const logo = ht_class_create(&ht_splash_logo_class, _root);
    if (!logo)
        return false;
#if !LV_USE_THEME_MONO && \
    !(defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME)
    lv_img_set_src(logo, &icon_meshcore_logo_alpha_img);
    lv_img_set_antialias(logo, false);
    lv_img_set_zoom(logo, LV_IMG_ZOOM_NONE);
#else
    lv_img_set_src(logo, &icon_meshcore_log_img);
#endif
    lv_obj_t *const version = ht_label_create(&ht_splash_label_class, _root);
    if (!version)
        return false;
    lv_obj_set_width(version, lv_pct(100));
    lv_label_set_long_mode(version, LV_LABEL_LONG_CLIP);

    lv_obj_t *const date = ht_label_create(&ht_splash_label_class, _root);
    if (!date)
        return false;
    lv_obj_set_width(date, lv_pct(100));
    lv_label_set_long_mode(date, LV_LABEL_LONG_CLIP);

    lv_obj_t *const attribution = ht_label_create(&ht_splash_label_class, _root);
    if (!attribution)
        return false;
    lv_obj_set_width(attribution, lv_pct(100));
    lv_label_set_long_mode(attribution, LV_LABEL_LONG_CLIP);
    lv_label_set_text_static(attribution, "Heltec MeshCore FW");

    printInfo(version, date);

    lv_disp_t *disp = lv_disp_get_default();
    if (disp)
    {
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
        lv_obj_update_layout(_root);
        lv_obj_invalidate(_root);
        Heltec::MeshCore::Dal::DisplayPort::commitBootFrame(nullptr, true);
#else
        lv_timer_handler();
        lv_refr_now(disp);
#endif
    }

    return true;
}

void SplashOverlay::printInfo(_lv_obj_t *version, _lv_obj_t *date)
{
    const char *ver = FIRMWARE_VERSION;
    const char *dash = strchr(ver, '-');
    int len = dash ? (int)(dash - ver) : (int)strlen(ver);
    if (len >= (int)sizeof(_version_text))
        len = (int)sizeof(_version_text) - 1;
    memcpy(_version_text, ver, (size_t)len);
    _version_text[len] = '\0';
    lv_label_set_text_static(version, _version_text);
    lv_label_set_text_static(date, FIRMWARE_BUILD_DATE);
}

void SplashOverlay::hide()
{
    if (!_root)
        return;
    lv_obj_add_flag(_root, LV_OBJ_FLAG_HIDDEN);
}

} // namespace Heltec::MeshCore::Ui
