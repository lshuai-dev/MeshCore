#include "navigation_widget.hpp"
#include "ui/widgets/widget_classes.hpp"
#include <lvgl.h>

namespace
{

static heltec::meshcore::ui::ht_navigator_t *navigator_data(lv_obj_t *obj)
{
    if (!obj || !lv_obj_check_type(obj, &heltec::meshcore::ui::ht_navigator_class))
    {
        return nullptr;
    }
    return reinterpret_cast<heltec::meshcore::ui::ht_navigator_t *>(obj);
}

static _lv_obj_t *ui_navigator_create_grid_impl(_lv_obj_t *parent)
{
    if (!parent)
        return nullptr;

    lv_obj_t *obj = heltec::meshcore::ui::ht_navigator_create(parent, nullptr);
    if (!obj)
    {
        return nullptr;
    }
    heltec::meshcore::ui::ht_navigator_t *w = navigator_data(obj);
    if (!w)
    {
        lv_obj_del(obj);
        return nullptr;
    }
    w->content = nullptr;
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE | LV_OBJ_FLAG_HIDDEN);
    w->content = heltec::meshcore::ui::ht_class_create(
        &heltec::meshcore::ui::ht_navigation_content_class, obj);
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

namespace heltec::meshcore::ui
{

namespace
{

void navigator_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    (void)class_p;
    (void)obj;
}

} // namespace

const lv_obj_class_t ht_navigator_class = {
    &lv_obj_class,
    navigator_constructor,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(ht_navigator_t),
};

lv_obj_t *ht_navigator_create(lv_obj_t *parent, void *user_data)
{
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_navigator_class, parent);
    if (!obj) {
        return nullptr;
    }

    lv_obj_class_init_obj(obj);
    lv_obj_set_user_data(obj, user_data);
    return obj;
}

_lv_obj_t *ui_navigator_create_grid(_lv_obj_t *parent)
{
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
    return ui_navigator_create_grid_impl(parent);
#else
    (void)parent;
    return nullptr;
#endif
}

bool ui_navigator_is(_lv_obj_t *obj)
{
    return obj && navigator_data(obj) != nullptr;
}

_lv_obj_t *ui_navigator_content(_lv_obj_t *obj)
{
    heltec::meshcore::ui::ht_navigator_t *w = navigator_data(obj);
    if (!w)
        return ui_navigator_is(obj) ? obj : nullptr;
    return w->content ? w->content : obj;
}

} // namespace heltec::meshcore::ui
