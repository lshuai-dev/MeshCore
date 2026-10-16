/**
 * @file ht_dropdown.hpp
 * @brief Dropdown class with viewport-aware popup placement.
 */
#ifndef HELTEC_UI_CORE_HT_DROPDOWN_HPP
#define HELTEC_UI_CORE_HT_DROPDOWN_HPP

#include <lvgl.h>

namespace heltec::meshcore::ui {

typedef struct {
    lv_dropdown_t dropdown;
    lv_obj_t *viewport;
    lv_obj_t *scroll_parent;
} ht_dropdown_t;

extern const lv_obj_class_t ht_dropdown_class;

lv_obj_t *ht_dropdown_create(lv_obj_t *parent);
void ht_dropdown_set_viewport(lv_obj_t *dropdown, lv_obj_t *viewport);
void ht_dropdown_set_scroll_parent(lv_obj_t *dropdown, lv_obj_t *scroll_parent);
bool ht_dropdown_cycle(lv_obj_t *dropdown, uint32_t key);
void ht_dropdown_realign(lv_obj_t *dropdown);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_CORE_HT_DROPDOWN_HPP
