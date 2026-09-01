#pragma once
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_semantic_classes.hpp"

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui
{

class ButtonRoller
{
  public:
    bool create(lv_obj_t *parent, lv_group_t *group,
                const lv_obj_class_t *root_class = &ht_button_roller_class);
    lv_obj_t *addItem(const char *label);
    lv_obj_t *root() const
    {
        return _root;
    }
    uint8_t itemCount() const
    {
        return _count;
    }
    void resetFocus(bool focus_first_item = true);
    bool focusItem(uint8_t index, bool checked = true);

  protected:
    void layoutItems(uint8_t focus_index, lv_anim_enable_t anim, bool is_checked = false);

  private:
    static constexpr uint8_t kMaxItems = 12;
    lv_obj_t *_root = nullptr;
    lv_group_t *_group = nullptr;
    lv_obj_t *_items[kMaxItems]{};
    uint8_t _count = 0;
    lv_coord_t _last_vp_h = 0;
    uint8_t _last_layout_focus = 0xFF;
};

} // namespace heltec::meshcore::ui
