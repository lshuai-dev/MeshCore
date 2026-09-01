#include "ui_navigator.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ht_meta_data_internal.hpp"
#include "ui/core/ui_semantic_classes.hpp"

#include <lvgl.h>

namespace
{

enum
{
    kNavigatorLayoutGrid = 1,
};

typedef struct
{
    lv_obj_t *content;
    uint8_t layout;
} NavigatorData;

static NavigatorData *navigator_data(lv_obj_t *obj)
{
    if (!obj || !lv_obj_check_type(obj, &heltec::meshcore::ui::ht_navigator_class))
    {
        return nullptr;
    }
    return static_cast<NavigatorData *>(heltec::meshcore::ui::ht_user_data(obj));
}

static void release_navigator_data(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DELETE)
        return;
    lv_obj_t *obj = lv_event_get_target(e);
    NavigatorData *w = static_cast<NavigatorData *>(heltec::meshcore::ui::ht_user_data(obj));
    if (!w)
        return;
    heltec::meshcore::ui::ht_set_user_data(obj, nullptr);
    lv_mem_free(w);
}

static _lv_obj_t *ui_navigator_create_with_layout(_lv_obj_t *parent, uint8_t layout)
{
    if (!parent)
        return nullptr;

    NavigatorData *w = static_cast<NavigatorData *>(lv_mem_alloc(sizeof(NavigatorData)));
    if (!w)
        return nullptr;
    w->content = nullptr;
    w->layout = layout;

    lv_obj_t *obj = heltec::meshcore::ui::ht_navigator_create(parent, w);
    if (!obj)
    {
        lv_mem_free(w);
        return nullptr;
    }
    lv_obj_add_event_cb(obj, release_navigator_data, LV_EVENT_DELETE, nullptr);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE | LV_OBJ_FLAG_HIDDEN);
    if (layout == kNavigatorLayoutGrid)
    {
        w->content = heltec::meshcore::ui::ht_semantic_create(
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
    }
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
    return detail::create_object(&ht_navigator_class, parent, user_data);
}

_lv_obj_t *ui_navigator_create_grid(_lv_obj_t *parent)
{
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
    return ui_navigator_create_with_layout(parent, kNavigatorLayoutGrid);
#else
    (void)parent;
    return nullptr;
#endif
}

bool ui_navigator_is(_lv_obj_t *obj)
{
    return obj && navigator_data(obj) != nullptr;
}

bool ui_navigator_is_grid(_lv_obj_t *obj)
{
    NavigatorData *w = navigator_data(obj);
    return w && w->layout == kNavigatorLayoutGrid;
}

_lv_obj_t *ui_navigator_content(_lv_obj_t *obj)
{
    NavigatorData *w = navigator_data(obj);
    if (!w)
        return ui_navigator_is(obj) ? obj : nullptr;
    return w->content ? w->content : obj;
}

} // namespace heltec::meshcore::ui
