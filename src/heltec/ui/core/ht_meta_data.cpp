#include "ht_meta_data.hpp"

namespace heltec::meshcore::ui {
namespace {

struct MetaData {
  void* user_data;
  MetaId id;
};

void meta_delete_callback(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_DELETE) return;
  lv_obj_t* obj = lv_event_get_target(e);
  auto* meta = static_cast<MetaData*>(lv_event_get_user_data(e));
  if (!meta) return;
  if (obj && obj->user_data == meta) obj->user_data = nullptr;
  lv_mem_free(meta);
}

MetaData* meta_data(lv_obj_t* obj) {
  if (!obj || !obj->user_data) return nullptr;
  void* const marker = lv_obj_get_event_user_data(obj, meta_delete_callback);
  return marker == obj->user_data ? static_cast<MetaData*>(obj->user_data) : nullptr;
}

const MetaData* meta_data(const lv_obj_t* obj) {
  return meta_data(const_cast<lv_obj_t*>(obj));
}

MetaData* meta_allocater(MetaId id, void* user_data) {
  auto* meta = static_cast<MetaData*>(lv_mem_alloc(sizeof(MetaData)));
  if (!meta) return nullptr;
  meta->user_data = user_data;
  meta->id = id;
  return meta;
}

}  // namespace

void ht_set_meta_id(lv_obj_t* obj, MetaId id) {
  if (!obj) return;
  if (MetaData* meta = meta_data(obj)) {
    meta->id = id;
    return;
  }

  void* const previous_user_data = obj->user_data;
  MetaData* meta = meta_allocater(id, previous_user_data);
  if (!meta) return;
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
}

MetaId ht_id(const lv_obj_t* obj) {
  const MetaData* meta = meta_data(obj);
  return meta ? meta->id : meta_id::None;
}

void* ht_user_data(const lv_obj_t* obj) {
  const MetaData* meta = meta_data(obj);
  return meta ? meta->user_data : nullptr;
}

void ht_set_user_data(lv_obj_t* obj, void* user_data) {
  if (!obj) return;
  if (MetaData* meta = meta_data(obj)) {
    meta->user_data = user_data;
    return;
  }

  MetaData* meta = meta_allocater(meta_id::None, user_data);
  if (!meta) return;
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
}

lv_obj_t* ht_obj_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_obj_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}

lv_obj_t* ht_label_create(lv_obj_t* parent, MetaId id, const char* text, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_label_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  if (text) lv_label_set_text(obj, text);
  return obj;
}

lv_obj_t* ht_btn_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_btn_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}

#if LV_USE_SWITCH
lv_obj_t* ht_switch_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_switch_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_SLIDER
lv_obj_t* ht_slider_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_slider_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_DROPDOWN
lv_obj_t* ht_dropdown_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_dropdown_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_ROLLER
lv_obj_t* ht_roller_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_roller_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_TEXTAREA
lv_obj_t* ht_textarea_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_textarea_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_KEYBOARD
lv_obj_t* ht_keyboard_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_keyboard_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_IMG
lv_obj_t* ht_img_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_img_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_IMGBTN
lv_obj_t* ht_imgbtn_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_imgbtn_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

#if LV_USE_MENU
lv_obj_t* ht_menu_create(lv_obj_t* parent, MetaId id, void* user_data) {
  MetaData* meta = meta_allocater(id, user_data);
  if (!meta) return nullptr;
  lv_obj_t* obj = lv_obj_class_create_obj(&lv_menu_class, parent);
  if (!obj) {
    lv_mem_free(meta);
    return nullptr;
  }
  obj->user_data = meta;
  lv_obj_add_event_cb(obj, meta_delete_callback, LV_EVENT_DELETE, meta);
  lv_obj_class_init_obj(obj);
  return obj;
}
#endif

}  // namespace heltec::meshcore::ui
