#include "ht_meta_data.hpp"

#include "ht_meta_data_internal.hpp"

namespace heltec::meshcore::ui
{
namespace
{

struct MetaData
{
    void *user_data;
    uint16_t id;
};

void meta_delete_callback(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_DELETE)
        return;

    lv_obj_t *const obj = lv_event_get_target(event);
    MetaData *const meta = static_cast<MetaData *>(lv_event_get_user_data(event));
    if (!meta)
        return;

    if (obj && obj->user_data == meta)
    {
        obj->user_data = nullptr;
    }
    lv_mem_free(meta);
}

MetaData *meta_data(lv_obj_t *obj)
{
    if (!obj || !obj->user_data)
        return nullptr;

    if (lv_obj_has_flag(obj, HT_OBJ_FLAG_HAS_META))
    {
        return static_cast<MetaData *>(obj->user_data);
    }

    // The flag is cleared by the LVGL base constructor. The marker is required
    // while the first theme callback runs during class initialization.
    void *const marker = lv_obj_get_event_user_data(obj, meta_delete_callback);
    if (marker != obj->user_data)
        return nullptr;
    return static_cast<MetaData *>(obj->user_data);
}

const MetaData *meta_data(const lv_obj_t *obj)
{
    return meta_data(const_cast<lv_obj_t *>(obj));
}

MetaData *meta_allocate(uint16_t id, void *user_data)
{
    MetaData *const meta = static_cast<MetaData *>(lv_mem_alloc(sizeof(MetaData)));
    if (!meta)
        return nullptr;

    meta->user_data = user_data;
    meta->id = id;
    return meta;
}

lv_obj_t *create_object(const lv_obj_class_t *class_p, lv_obj_t *parent, uint16_t id,
                        void *user_data)
{
    const bool needs_meta = id != HT_META_ID_UNSET;
    MetaData *meta = nullptr;

    if (needs_meta)
    {
        meta = meta_allocate(id, user_data);
        if (!meta)
            return nullptr;
    }

    lv_obj_t *const obj = lv_obj_class_create_obj(class_p, parent);
    if (!obj)
    {
        if (meta)
            lv_mem_free(meta);
        return nullptr;
    }

    if (meta)
    {
        obj->user_data = meta;
        if (!lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta))
        {
            obj->user_data = nullptr;
            lv_obj_class_init_obj(obj);
            lv_obj_del(obj);
            lv_mem_free(meta);
            return nullptr;
        }
    }

    lv_obj_class_init_obj(obj);

    if (meta)
    {
        lv_obj_add_flag(obj, HT_OBJ_FLAG_HAS_META);
    }
    else if (user_data)
    {
        obj->user_data = user_data;
    }

    return obj;
}

} // namespace

void detail::set_meta_id(lv_obj_t *obj, uint16_t id)
{
    if (!obj)
        return;

    MetaData *const meta = meta_data(obj);
    if (id == HT_META_ID_UNSET)
    {
        if (!meta)
            return;

        void *const user_data = meta->user_data;
        obj->user_data = user_data;
        lv_obj_clear_flag(obj, HT_OBJ_FLAG_HAS_META);
        lv_obj_remove_event_cb_with_user_data(obj, meta_delete_callback, meta);
        lv_mem_free(meta);
        return;
    }

    if (meta)
    {
        meta->id = id;
        if (!lv_obj_has_flag(obj, HT_OBJ_FLAG_HAS_META))
        {
            lv_obj_add_flag(obj, HT_OBJ_FLAG_HAS_META);
        }
        return;
    }

    MetaData *const new_meta = meta_allocate(id, obj->user_data);
    if (!new_meta)
        return;

    obj->user_data = new_meta;
    if (!lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, new_meta))
    {
        obj->user_data = new_meta->user_data;
        lv_mem_free(new_meta);
        return;
    }
    lv_obj_add_flag(obj, HT_OBJ_FLAG_HAS_META);
}

uint16_t ht_id(const lv_obj_t *obj)
{
    const MetaData *const meta = meta_data(obj);
    return meta ? meta->id : HT_META_ID_UNSET;
}

void *ht_user_data(const lv_obj_t *obj)
{
    const MetaData *const meta = meta_data(obj);
    return meta ? meta->user_data : (obj ? obj->user_data : nullptr);
}

void ht_set_user_data(lv_obj_t *obj, void *user_data)
{
    if (!obj)
        return;

    MetaData *const meta = meta_data(obj);
    if (meta)
    {
        meta->user_data = user_data;
    }
    else
    {
        obj->user_data = user_data;
    }
}

lv_obj_t *ht_obj_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_obj_class, parent, id, user_data);
}

lv_obj_t *ht_label_create(lv_obj_t *parent, uint16_t id, const char *text, void *user_data)
{
    lv_obj_t *const obj = create_object(&lv_label_class, parent, id, user_data);
    if (obj && text)
        lv_label_set_text(obj, text);
    return obj;
}

lv_obj_t *ht_btn_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_btn_class, parent, id, user_data);
}

#if LV_USE_SWITCH
lv_obj_t *ht_switch_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_switch_class, parent, id, user_data);
}
#endif

#if LV_USE_SLIDER
lv_obj_t *ht_slider_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_slider_class, parent, id, user_data);
}
#endif

#if LV_USE_DROPDOWN
lv_obj_t *ht_dropdown_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_dropdown_class, parent, id, user_data);
}
#endif

#if LV_USE_ROLLER
lv_obj_t *ht_roller_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_roller_class, parent, id, user_data);
}
#endif

#if LV_USE_TEXTAREA
lv_obj_t *ht_textarea_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_textarea_class, parent, id, user_data);
}
#endif

#if LV_USE_KEYBOARD
lv_obj_t *ht_keyboard_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_keyboard_class, parent, id, user_data);
}
#endif

#if LV_USE_IMG
lv_obj_t *ht_img_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_img_class, parent, id, user_data);
}
#endif

#if LV_USE_IMGBTN
lv_obj_t *ht_imgbtn_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_imgbtn_class, parent, id, user_data);
}
#endif

#if LV_USE_MENU
lv_obj_t *ht_menu_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_menu_class, parent, id, user_data);
}

lv_obj_t *ht_menu_page_create(lv_obj_t *parent, const char *title, uint16_t id, void *user_data)
{
    lv_obj_t *const obj = lv_menu_page_create(parent, title ? const_cast<char *>(title) : nullptr);
    if (!obj)
        return nullptr;

    detail::set_meta_id(obj, id);
    ht_set_user_data(obj, user_data);
    return obj;
}
#endif

#if LV_USE_TILEVIEW
lv_obj_t *ht_tileview_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return create_object(&lv_tileview_class, parent, id, user_data);
}

lv_obj_t *ht_tileview_add_tile(lv_obj_t *tileview, uint8_t col_id, uint8_t row_id, lv_dir_t dir,
                               uint16_t id, void *user_data)
{
    lv_obj_t *const tile = lv_tileview_add_tile(tileview, col_id, row_id, dir);
    if (!tile)
        return nullptr;

    detail::set_meta_id(tile, id);
    ht_set_user_data(tile, user_data);
    return tile;
}
#endif

} // namespace heltec::meshcore::ui
