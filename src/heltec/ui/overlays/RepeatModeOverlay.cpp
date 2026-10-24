#include "RepeatModeOverlay.hpp"
#include "ui/core/BizFacade.hpp"
#include "ui/core/HtEvent.h"
#include "ui/core/OperationHints.hpp"
#include "ui/theme/WidgetTheme.hpp"
namespace Heltec::MeshCore::Ui {
namespace {

const char *apply_error_text(Biz::IBizFacade::ForwardingApplyResult result) {
    switch (result) {
    case Biz::IBizFacade::ForwardingApplyResult::InvalidSelection:
        return "Invalid frequency";
    case Biz::IBizFacade::ForwardingApplyResult::UnsupportedFrequency:
        return "Frequency unsupported";
    case Biz::IBizFacade::ForwardingApplyResult::InvalidRadioParams:
        return "Invalid radio params";
    case Biz::IBizFacade::ForwardingApplyResult::Unavailable:
        return "Repeat mode unavailable";
    case Biz::IBizFacade::ForwardingApplyResult::Ok:
        return nullptr;
    }
    return "Repeat mode failed";
}

void format_frequency(char *buffer, size_t capacity, uint32_t khz) {
    const uint32_t whole = khz / 1000u;
    const uint32_t fraction = khz % 1000u;
    if (fraction == 0) {
        lv_snprintf(buffer, capacity, "%u.0", (unsigned)whole);
    } else {
        lv_snprintf(buffer, capacity, "%u.%03u", (unsigned)whole, (unsigned)fraction);
    }
}

bool format_frequency_item(const Biz::IBizFacade &biz, int index, char *buffer, size_t capacity) {
    uint32_t lower_khz = 0;
    uint32_t upper_khz = 0;
    if (!buffer || capacity == 0 || !biz.forwardingFrequencyRange(index, &lower_khz, &upper_khz)) {
        return false;
    }

    char lower[16]{};
    format_frequency(lower, sizeof(lower), lower_khz);
    if (lower_khz == upper_khz) {
        lv_snprintf(buffer, capacity, "%s MHz", lower);
        return true;
    }

    char upper[16]{};
    format_frequency(upper, sizeof(upper), upper_khz);
    lv_snprintf(buffer, capacity, "%s-%s MHz", lower, upper);
    return true;
}

} // namespace

RepeatModeOverlay::RepeatModeOverlay(Biz::IBizFacade &biz)
    : FramedOverlay(biz) {
}

bool RepeatModeOverlay::build() {
    if (!FramedOverlay::build()) return false;

    setStaticTitle("Repeat frequency");
    setStaticToolTip(OperationHint::kRepeatMode);

    _lv_obj_t *const list = ht_class_create(&ht_repeat_mode_list_class, _content);
    if (!list) return false;
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);

#if HELTEC_UI_HAS_TOUCH
#if LV_USE_ROLLER == 0
#error "Touch repeat mode overlay requires LV_USE_ROLLER=1"
#endif
    _touch_roller = lv_roller_create(list);
    if (!_touch_roller) return false;
    lv_obj_set_width(_touch_roller, lv_pct(100));
    lv_obj_add_event_cb(_touch_roller, onRollerEvent, LV_EVENT_CLICKED, this);
#else
    if (!_roller.create(list, &_gridNav, &ht_repeat_mode_roller_class,
                        &ht_repeat_mode_item_class, &ht_repeat_mode_label_class)) return false;
#endif

    char label[kFrequencyLabelCapacity]{};
#if HELTEC_UI_HAS_TOUCH
    size_t options_used = 0;
#endif
    const int available = _biz.forwardingFrequencyCount();
    for (int index = 0; index < available && _frequency_count < kMaxFrequencyItems; ++index) {
        if (!format_frequency_item(_biz, index, label, sizeof(label))) {
            continue;
        }

#if HELTEC_UI_HAS_TOUCH
        const int written = lv_snprintf(_roller_options + options_used,
                                        sizeof(_roller_options) - options_used, "%s%s",
                                        options_used > 0 ? "\n" : "", label);
        if (written < 0 || static_cast<size_t>(written) >=
                               sizeof(_roller_options) - options_used) {
            break;
        }
        options_used += static_cast<size_t>(written);
#else
        _lv_obj_t *const button = _roller.addItem(label);
        if (!button) return false;
        if (_lv_obj_t *const label_obj = lv_obj_get_child(button, 0)) {
            lv_obj_center(label_obj);
        }
        lv_obj_add_event_cb(button, onItemClicked, LV_EVENT_CLICKED, this);
#endif
        _frequency_indices[_frequency_count] = static_cast<int8_t>(index);
        ++_frequency_count;
    }

#if HELTEC_UI_HAS_TOUCH
    if (_frequency_count == 0) {
        lv_snprintf(_roller_options, sizeof(_roller_options), "No frequency");
    }
    // Keep the three choices visible around the current one and preserve the
    // previous ButtonRoller's wrap-around navigation.
    lv_roller_set_options(_touch_roller, _roller_options, LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(_touch_roller, _frequency_count > 0 ? _frequency_count : 1);
#endif

    return true;
}

void RepeatModeOverlay::onEnter() {
#if HELTEC_UI_HAS_TOUCH
    _gridNav.clear();
    _gridNav.add(_touch_roller);
    (void)_gridNav.enterControls();
#endif
    setApplying(false);

    int selected_item = 0;
    const int current_frequency = _biz.currentForwardingFrequencyIndex();
    for (uint8_t i = 0; i < _frequency_count; ++i) {
        if (_frequency_indices[i] == current_frequency) {
            selected_item = i;
            break;
        }
    }
    if (_frequency_count > 0) {
#if HELTEC_UI_HAS_TOUCH
        if (_touch_roller) {
            lv_roller_set_selected(_touch_roller, static_cast<uint16_t>(selected_item),
                                   LV_ANIM_OFF);
        }
#else
        (void)_roller.focusItem(static_cast<uint8_t>(selected_item), true);
#endif
    } else {
#if !HELTEC_UI_HAS_TOUCH
        (void)_roller.focusItem(0, true);
#endif
    }
}

bool RepeatModeOverlay::onKey(uint32_t key) {
    if (key != LV_KEY_ESC) return false;
    cancelSelection();
    return true;
}

#if HELTEC_UI_HAS_TOUCH

bool RepeatModeOverlay::hitRoller(int16_t x, int16_t y) const {
    if (!_touch_roller || !lv_obj_is_valid(_touch_roller) ||
        lv_obj_has_flag(_touch_roller, LV_OBJ_FLAG_HIDDEN)) {
        return false;
    }
    lv_area_t area{};
    lv_obj_get_coords(_touch_roller, &area);
    return x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2;
}

void RepeatModeOverlay::onRollerEvent(lv_event_t *event) {
    if (!event) return;
    auto *const self = static_cast<RepeatModeOverlay *>(lv_event_get_user_data(event));
    _lv_obj_t *const roller = lv_event_get_target(event);
    if (!self || !roller || self->_applying || self->_frequency_count == 0) return;

    const lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_CLICKED) return;

    const auto *const state = reinterpret_cast<const lv_roller_t *>(roller);
    if (state && !state->moved) {
        self->applySelection(static_cast<uint8_t>(lv_roller_get_selected(roller)));
    }
}

#else

void RepeatModeOverlay::onItemClicked(lv_event_t *event) {
    if (!event || lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    auto *const self = static_cast<RepeatModeOverlay *>(lv_event_get_user_data(event));
    _lv_obj_t *const target = lv_event_get_target(event);
    if (!self || !target || self->_applying) return;

    if (lv_obj_get_parent(target) != self->_roller.root()) return;
    const uint32_t item_index = lv_obj_get_index(target);
    if (item_index < self->_frequency_count) {
        self->applySelection(static_cast<uint8_t>(item_index));
    }
}

#endif

void RepeatModeOverlay::applySelection(uint8_t item_index) {
    if (_applying || item_index >= _frequency_count) return;
    setApplying(true);

    const Biz::IBizFacade::ForwardingApplyResult result =
        _biz.setForwardingEnabled(true, _frequency_indices[item_index]);
    const char *const error = apply_error_text(result);
    if (!error && _biz.forwardingEnabled()) {
        showAlert("Repeat mode enabled", 1200);
        (void)notifyApp(EventType::RepeatModeClose);
        return;
    }

    setApplying(false);
    showAlert(error ? error : "Repeat mode failed", 2000);
}

void RepeatModeOverlay::cancelSelection() {
    if (_applying) return;
    (void)notifyApp(EventType::RepeatModeClose);
}

void RepeatModeOverlay::setApplying(bool applying) {
    _applying = applying;
#if HELTEC_UI_HAS_TOUCH
    if (_touch_roller) {
        if (applying || _frequency_count == 0) {
            lv_obj_add_state(_touch_roller, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(_touch_roller, LV_STATE_DISABLED);
        }
    }
#else
    _lv_obj_t *const root = _roller.root();
    const uint32_t count = root ? lv_obj_get_child_cnt(root) : 0;
    for (uint32_t i = 0; i < count; ++i) {
        _lv_obj_t *const button = lv_obj_get_child(root, i);
        if (!button) continue;
        if (applying) {
            lv_obj_add_state(button, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(button, LV_STATE_DISABLED);
        }
    }
#endif
}

} // namespace Heltec::MeshCore::Ui
