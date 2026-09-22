#ifndef HELTEC_UI_CORE_HT_CONTAINER_HPP
#define HELTEC_UI_CORE_HT_CONTAINER_HPP
#include <lvgl.h>

namespace heltec::meshcore::ui {

typedef struct {
    lv_obj_t obj;
} ht_container_t;

extern const lv_obj_class_t ht_container_class;

/** Updates a row container and all descendants with the logical focus state. */
void ht_container_set_focused(lv_obj_t *container, bool focused);

lv_obj_t *ht_container_create(lv_obj_t *parent, void *user_data = nullptr);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_CORE_HT_CONTAINER_HPP
