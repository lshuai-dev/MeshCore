#include "NavigationWidget.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include <lvgl.h>

namespace
{

static Heltec::MeshCore::Ui::ht_gridNav_t *navigator_data(lv_obj_t *obj)
{
    if (!obj || !lv_obj_check_type(obj, &Heltec::MeshCore::Ui::ht_gridNav_class))
    {
        return nullptr;
    }
    return reinterpret_cast<Heltec::MeshCore::Ui::ht_gridNav_t *>(obj);
}

static _lv_obj_t *ui_gridNav_create_grid_impl(_lv_obj_t *parent)
{
    if (!parent)
        return nullptr;

    lv_obj_t *obj = Heltec::MeshCore::Ui::ht_gridNav_create(parent, nullptr);
    if (!obj)
    {
        return nullptr;
    }
    Heltec::MeshCore::Ui::ht_gridNav_t *w = navigator_data(obj);
    if (!w)
    {
        lv_obj_del(obj);
        return nullptr;
    }
    w->content = nullptr;
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE | LV_OBJ_FLAG_HIDDEN);
    w->content = Heltec::MeshCore::Ui::ht_class_create(
        &Heltec::MeshCore::Ui::ht_navigation_content_class, obj);
    if (!w->content)
    {
        lv_obj_del(obj);
        return nullptr;
    }
    lv_obj_set_pos(w->content, 0, 0);
    lv_obj_set_size(w->content, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(w->content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(w->content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
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
    sizeof(ht_gridNav_t),
};

lv_obj_t *ht_gridNav_create(lv_obj_t *parent, void *user_data)
{
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_gridNav_class, parent);
    if (!obj) {
        return nullptr;
    }

    lv_obj_class_init_obj(obj);
    lv_obj_set_user_data(obj, user_data);
    return obj;
}

_lv_obj_t *ui_gridNav_create_grid(_lv_obj_t *parent)
{
#if HELTEC_UI_HAS_TOUCH
    return ui_gridNav_create_grid_impl(parent);
#else
    (void)parent;
    return nullptr;
#endif
}

bool ui_gridNav_is(_lv_obj_t *obj)
{
    return obj && navigator_data(obj) != nullptr;
}

_lv_obj_t *ui_gridNav_content(_lv_obj_t *obj)
{
    Heltec::MeshCore::Ui::ht_gridNav_t *w = navigator_data(obj);
    if (!w)
        return ui_gridNav_is(obj) ? obj : nullptr;
    return w->content ? w->content : obj;
}

} // namespace Heltec::MeshCore::Ui
