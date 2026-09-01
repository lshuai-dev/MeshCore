#include "ht_meta_data.hpp"

#include "ht_meta_data_internal.hpp"

namespace heltec::meshcore::ui
{
namespace
{

lv_obj_t *create_object_impl(const lv_obj_class_t *class_p, lv_obj_t *parent, void *user_data)
{
    lv_obj_t *const obj = lv_obj_class_create_obj(class_p, parent);
    if (!obj)
    {
        return nullptr;
    }

    obj->user_data = user_data;
    lv_obj_class_init_obj(obj);
    return obj;
}

} // namespace

lv_obj_t *detail::create_object(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                void *user_data)
{
    return create_object_impl(class_p, parent, user_data);
}

void *ht_user_data(const lv_obj_t *obj)
{
    return obj ? obj->user_data : nullptr;
}

void ht_set_user_data(lv_obj_t *obj, void *user_data)
{
    if (obj)
    {
        obj->user_data = user_data;
    }
}

lv_obj_t *ht_obj_create(lv_obj_t *parent, void *user_data)
{
    return create_object_impl(&lv_obj_class, parent, user_data);
}

lv_obj_t *ht_label_create(lv_obj_t *parent, const char *text, void *user_data)
{
    lv_obj_t *const obj = create_object_impl(&lv_label_class, parent, user_data);
    if (obj && text)
    {
        lv_label_set_text(obj, text);
    }
    return obj;
}

#define HT_DEFINE_WIDGET_FACTORY(function_name, class_name)                                      \
    lv_obj_t *function_name(lv_obj_t *parent, void *user_data)                                  \
    {                                                                                            \
        return create_object_impl(&class_name, parent, user_data);                               \
    }

HT_DEFINE_WIDGET_FACTORY(ht_btn_create, lv_btn_class)
#if LV_USE_SWITCH
HT_DEFINE_WIDGET_FACTORY(ht_switch_create, lv_switch_class)
#endif
#if LV_USE_SLIDER
HT_DEFINE_WIDGET_FACTORY(ht_slider_create, lv_slider_class)
#endif
#if LV_USE_DROPDOWN
HT_DEFINE_WIDGET_FACTORY(ht_dropdown_create, lv_dropdown_class)
#endif
#if LV_USE_ROLLER
HT_DEFINE_WIDGET_FACTORY(ht_roller_create, lv_roller_class)
#endif
#if LV_USE_TEXTAREA
HT_DEFINE_WIDGET_FACTORY(ht_textarea_create, lv_textarea_class)
#endif
#if LV_USE_KEYBOARD
HT_DEFINE_WIDGET_FACTORY(ht_keyboard_create, lv_keyboard_class)
#endif
#if LV_USE_IMG
HT_DEFINE_WIDGET_FACTORY(ht_img_create, lv_img_class)
#endif
#if LV_USE_IMGBTN
HT_DEFINE_WIDGET_FACTORY(ht_imgbtn_create, lv_imgbtn_class)
#endif
#if LV_USE_MENU
HT_DEFINE_WIDGET_FACTORY(ht_menu_create, lv_menu_class)

lv_obj_t *ht_menu_page_create(lv_obj_t *parent, const char *title, void *user_data)
{
    lv_obj_t *const obj = lv_menu_page_create(parent, title ? const_cast<char *>(title) : nullptr);
    ht_set_user_data(obj, user_data);
    return obj;
}
#endif
#if LV_USE_TILEVIEW
HT_DEFINE_WIDGET_FACTORY(ht_tileview_create, lv_tileview_class)

lv_obj_t *ht_tileview_add_tile(lv_obj_t *tileview, uint8_t col_id, uint8_t row_id, lv_dir_t dir,
                               void *user_data)
{
    lv_obj_t *const tile = lv_tileview_add_tile(tileview, col_id, row_id, dir);
    ht_set_user_data(tile, user_data);
    return tile;
}
#endif

#undef HT_DEFINE_WIDGET_FACTORY

} // namespace heltec::meshcore::ui
