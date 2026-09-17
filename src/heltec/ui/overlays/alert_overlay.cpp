#include "alert_overlay.hpp"
#include "ui/core/ht_event.h"
#include "ui/core/widget_classes.hpp"

namespace heltec::meshcore::ui {
namespace {

constexpr uint32_t kClickGuardMs = 500;

void fit_to_parent(lv_obj_t *obj) {
    if (!obj) {
        return;
    }

    lv_obj_t *parent = obj ? lv_obj_get_parent(obj) : nullptr;
    if (parent) {
        lv_obj_update_layout(parent);
        const lv_coord_t w = lv_obj_get_width(parent);
        const lv_coord_t h = lv_obj_get_height(parent);
        if (w > 0 && h > 0) {
            lv_obj_set_size(obj, w, h);
            lv_obj_set_pos(obj, 0, 0);
            return;
        }
    }

    lv_disp_t *const display = lv_disp_get_default();
    if (display) {
        lv_obj_set_size(obj, lv_disp_get_hor_res(display), lv_disp_get_ver_res(display));
        lv_obj_set_pos(obj, 0, 0);
    }
}

} // namespace

bool AlertOverlay::buildSurface() {
    if (!DialogOverlay::buildSurface()) return false;

    lv_obj_set_layout(_root, 0);

    lv_obj_add_flag(panel(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        panel(),
        [](lv_event_t *e) {
            auto *self = static_cast<AlertOverlay *>(lv_event_get_user_data(e));
            if (!self) return;
            const uint32_t age_ms = lv_tick_elaps(self->_entered_ms);
            lv_event_stop_bubbling(e);
            lv_event_stop_processing(e);
            if (age_ms < kClickGuardMs) {
                return;
            }
            (void)self->emitEvent(EventType::AlertClose);
        },
        LV_EVENT_CLICKED, this);

    _label = ht_label_create(&ht_overlay_feedback_label_class, contentHost());
    if (!_label) return false;
    lv_obj_set_width(_label, lv_pct(100));
    lv_label_set_long_mode(_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(_label, _text_buffer);

    return true;
}

void AlertOverlay::onEnter() {
    DialogOverlay::onEnter();
    if (!_root) return;
    _entered_ms = lv_tick_get();

    fit_to_parent(_root);
    lv_obj_set_layout(_root, 0);

    if (panel()) {
        lv_obj_update_layout(panel());
        lv_obj_align(panel(), LV_ALIGN_CENTER, 0, 0);
    }
    lv_obj_update_layout(_root);
    lv_obj_invalidate(_root);
}

void AlertOverlay::setText(const char *text) {
    lv_snprintf(_text_buffer, sizeof(_text_buffer), "%s", text ? text : "");
    if (_label) {
        lv_label_set_text_static(_label, _text_buffer);
    }
    if (panel()) {
        lv_obj_update_layout(panel());
        lv_obj_align(panel(), LV_ALIGN_CENTER, 0, 0);
    }
}

bool AlertOverlay::onKey(uint32_t key) {
    if (key == LV_KEY_ENTER) {
        return true;
    }
    if (key == LV_KEY_ESC) {
        return emitEvent(EventType::AlertClose);
    }
    return false;
}

} // namespace heltec::meshcore::ui
