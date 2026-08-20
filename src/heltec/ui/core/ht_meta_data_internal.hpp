#ifndef HELTEC_UI_CORE_HT_META_DATA_INTERNAL_HPP
#define HELTEC_UI_CORE_HT_META_DATA_INTERNAL_HPP

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui::detail
{

lv_obj_t *create_object(const lv_obj_class_t *class_p, lv_obj_t *parent, uint16_t id,
                        void *user_data);
void set_meta_id(lv_obj_t *obj, uint16_t id);

} // namespace heltec::meshcore::ui::detail

#endif // HELTEC_UI_CORE_HT_META_DATA_INTERNAL_HPP
