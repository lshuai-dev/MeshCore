#pragma once
#include "ui/core/HtGridNav.hpp"
#include "ui/widgets/WidgetClasses.hpp"

#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

class ButtonRoller
{
  public:
    bool create(lv_obj_t *parent, HtGridNav *navigator,
                const lv_obj_class_t *root_class = &ht_button_roller_class,
                const lv_obj_class_t *item_class = &ht_button_roller_item_class,
                const lv_obj_class_t *label_class = &ht_button_roller_label_class);
    lv_obj_t *addItem(const char *label);
    bool ensureCapacity(uint8_t count);
    bool showItems(uint8_t count);
    bool setItemLabelStatic(uint8_t index, const char *label);
    lv_obj_t *root() const
    {
        return _root;
    }
    uint8_t itemCount() const
    {
        return _active_count;
    }
    void setInputEnabled(bool enabled);
    void resetFocus(bool focus_first_item = true);
    bool focusItem(uint8_t index, bool checked = true);

  protected:
    void layoutItems(uint8_t focus_index, lv_anim_enable_t anim, bool is_checked = false);

  private:
    static constexpr uint8_t kMaxItems = 12;
    lv_obj_t *_root = nullptr;
    HtGridNav *_gridNav = nullptr;
    const lv_obj_class_t *_item_class = &ht_button_roller_item_class;
    const lv_obj_class_t *_label_class = &ht_button_roller_label_class;
    lv_obj_t *itemAt(uint8_t index) const;

    uint8_t _active_count = 0;
    bool _input_enabled = true;
    lv_coord_t _last_vp_h = 0;
    uint8_t _last_layout_focus = 0xFF;
};

} // namespace Heltec::MeshCore::Ui
