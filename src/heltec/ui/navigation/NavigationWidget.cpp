#include "NavigationWidget.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include <lvgl.h>

namespace
{

static bool is_grid_navigation(lv_obj_t *obj)
{
    return obj && lv_obj_check_type(obj, &Heltec::MeshCore::Ui::ht_gridNav_class);
}

static _lv_obj_t *ui_gridNav_create_grid_impl(_lv_obj_t *parent)
{
    if (!parent)
        return nullptr;

    lv_obj_t *obj = lv_obj_class_create_obj(&Heltec::MeshCore::Ui::ht_gridNav_class, parent);
    if (!obj)
    {
        return nullptr;
    }
    lv_obj_class_init_obj(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE | LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *const content = Heltec::MeshCore::Ui::ht_class_create(
        &Heltec::MeshCore::Ui::ht_navigation_content_class, obj);
    if (!content)
    {
        lv_obj_del(obj);
        return nullptr;
    }
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_size(content, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    return obj;
}

} // namespace

namespace Heltec::MeshCore::Ui
{

const lv_obj_class_t ht_gridNav_class = {
    &lv_obj_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_obj_t),
};

_lv_obj_t *ui_gridNav_create_grid(_lv_obj_t *parent)
{
#if HELTEC_UI_HAS_TOUCH
    return ui_gridNav_create_grid_impl(parent);
#else
    (void)parent;
    return nullptr;
#endif
}

_lv_obj_t *ui_gridNav_content(_lv_obj_t *obj)
{
    if (!is_grid_navigation(obj))
        return nullptr;
    lv_obj_t *const content = lv_obj_get_child(obj, 0);
    return content ? content : obj;
}

} // namespace Heltec::MeshCore::Ui
