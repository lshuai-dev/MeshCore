#ifndef HELTEC_UI_CORE_HT_CONTAINER_HPP
#define HELTEC_UI_CORE_HT_CONTAINER_HPP
#include <lvgl.h>

namespace Heltec::MeshCore::Ui {

typedef struct {
    lv_obj_t obj;
} ht_container_t;

extern const lv_obj_class_t ht_container_class;

/** Updates a row container and all descendants with the logical selection state. */
void ht_container_set_selected(lv_obj_t *container, bool selected);

lv_obj_t *ht_container_create(lv_obj_t *parent, void *user_data = nullptr);

} // namespace Heltec::MeshCore::Ui

#endif // HELTEC_UI_CORE_HT_CONTAINER_HPP
