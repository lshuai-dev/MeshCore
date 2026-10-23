#include "PreviewOverlay.hpp"
#include "ui/core/HtEvent.h"
#include "ui/core/OperationHints.hpp"
#include "ui/widgets/WidgetClasses.hpp"

#include <cstring>

namespace Heltec::MeshCore::Ui {

namespace {

constexpr uint16_t kMessageScrollSpeed = 18;
constexpr uint16_t kMessageScrollDelayMs = 2000;

void setMessageScrollY(void *object, int32_t value)
{
    lv_obj_set_y(static_cast<lv_obj_t *>(object), static_cast<lv_coord_t>(value));
}

} // namespace

PreviewOverlay::PreviewOverlay(Biz::IBizFacade &biz)
    : FramedOverlay(biz, ht_preview_overlay_class) {
}

bool PreviewOverlay::buildHeader() {
    // Preview metadata replaces the standard framed header.
    return true;
}

bool PreviewOverlay::buildSurface() {
    if (!FramedOverlay::buildSurface()) return false;

    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_root, onRootClicked, LV_EVENT_CLICKED, this);

    lv_obj_t *row = ht_class_create(&ht_preview_metadata_row_class, _content);
    if (!row) return false;
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    _unread_label = ht_label_create(&ht_preview_metadata_label_class, row);
    _age_label = ht_label_create(&ht_preview_metadata_label_class, row);
    if (!_unread_label || !_age_label) return false;
    lv_label_set_text_static(_unread_label, _unread_text);
    lv_label_set_text_static(_age_label, _age_text);

    _origin_label = ht_label_create(&ht_preview_origin_class, _content);
    _message_viewport = lv_obj_create(_content);
    if (!_origin_label || !_message_viewport) return false;

    lv_obj_remove_style_all(_message_viewport);
    lv_obj_set_width(_message_viewport, lv_pct(100));
    lv_obj_set_flex_grow(_message_viewport, 1);
    lv_obj_clear_flag(_message_viewport, LV_OBJ_FLAG_SCROLLABLE);

    _message_label = ht_label_create(&ht_preview_text_class, _message_viewport);
    if (!_message_label) return false;
    lv_label_set_text_static(_origin_label, _origin_text);
    lv_label_set_text_static(_message_label, _message_text);
    lv_obj_set_width(_origin_label, lv_pct(100));
    lv_label_set_long_mode(_origin_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(_message_label, lv_pct(100));
    lv_obj_set_height(_message_label, LV_SIZE_CONTENT);
    lv_obj_align(_message_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_long_mode(_message_label, LV_LABEL_LONG_WRAP);

    setStaticToolTip(OperationHint::kPreviewClose);

    _age_timer = lv_timer_create(ageTimerCallback, 1000U, this);
    if (!_age_timer) return false;
    lv_timer_set_repeat_count(_age_timer, -1);
    lv_timer_pause(_age_timer);

    return true;
}

bool PreviewOverlay::onKey(uint32_t key) {
    if (key == LV_KEY_NEXT || key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_RIGHT ||
        key == LV_KEY_UP || key == LV_KEY_DOWN) {
        emitEvent(EventType::PreviewNext);
        return true;
    }
    if (key == LV_KEY_ENTER) {
        emitEvent(EventType::PreviewClose);
        return true;
    }
    if (key == LV_KEY_ESC) return true;
    return false;
}

void PreviewOverlay::onRootClicked(lv_event_t *event) {
    if (event == nullptr || lv_event_get_code(event) != LV_EVENT_CLICKED) return;

    auto *const self = static_cast<PreviewOverlay *>(lv_event_get_user_data(event));
    if (self == nullptr) return;

    lv_event_stop_bubbling(event);
    lv_event_stop_processing(event);
    self->emitEvent(EventType::PreviewNext);
}

void PreviewOverlay::applyContent(uint8_t unread, uint32_t received_ms, const char *origin,
                                  const char *text) {
    if (_unread_label == nullptr || _age_label == nullptr || _origin_label == nullptr ||
        _message_label == nullptr)
        return;

    lv_snprintf(_unread_text, sizeof(_unread_text), "Unread:%u", (unsigned)unread);
    _received_ms = received_ms;
    updateAge();
    lv_snprintf(_origin_text, sizeof(_origin_text), "%s", origin ? origin : "");
    lv_snprintf(_message_text, sizeof(_message_text), "%s", text ? text : "");
    lv_label_set_text_static(_unread_label, _unread_text);
    lv_label_set_text_static(_origin_label, _origin_text);
    lv_label_set_text_static(_message_label, _message_text);
}

void PreviewOverlay::updateMessageScroll()
{
    if (!_root || !_message_viewport || !_message_label) return;

    lv_anim_del(_message_label, setMessageScrollY);
    lv_obj_set_y(_message_label, 0);
    lv_obj_update_layout(_root);

    const lv_coord_t overflow = lv_obj_get_height(_message_label) -
                                lv_obj_get_content_height(_message_viewport);
    if (overflow <= 0) return;

    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, _message_label);
    lv_anim_set_exec_cb(&animation, setMessageScrollY);
    lv_anim_set_values(&animation, 0, -overflow);
    lv_anim_set_time(&animation, lv_anim_speed_to_time(kMessageScrollSpeed, 0, overflow));
    lv_anim_set_playback_time(&animation, animation.time);
    lv_anim_set_playback_delay(&animation, kMessageScrollDelayMs);
    lv_anim_set_repeat_delay(&animation, kMessageScrollDelayMs);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&animation);
}

void PreviewOverlay::updateAge() {
    const uint32_t age_sec = (lv_tick_get() - _received_ms) / 1000U;
    char age_text[sizeof(_age_text)] = {};
    if (age_sec < 60) {
        lv_snprintf(age_text, sizeof(age_text), "%us", (unsigned)age_sec);
    } else if (age_sec < 3600) {
        lv_snprintf(age_text, sizeof(age_text), "%um", (unsigned)(age_sec / 60));
    } else {
        lv_snprintf(age_text, sizeof(age_text), "%uh", (unsigned)(age_sec / 3600));
    }

    if (std::strcmp(age_text, _age_text) == 0) return;

    lv_snprintf(_age_text, sizeof(_age_text), "%s", age_text);
    lv_label_set_text_static(_age_label, _age_text);
}

void PreviewOverlay::onEnter() {
    updateAge();
    updateMessageScroll();
    if (_age_timer) {
        lv_timer_reset(_age_timer);
        lv_timer_resume(_age_timer);
    }
    AbstractOverlay::onEnter();
}

void PreviewOverlay::onExit() {
    if (_message_label) {
        lv_anim_del(_message_label, setMessageScrollY);
        lv_obj_set_y(_message_label, 0);
    }
    if (_age_timer) lv_timer_pause(_age_timer);
}

void PreviewOverlay::ageTimerCallback(lv_timer_t *timer) {
    auto *self = timer ? static_cast<PreviewOverlay *>(timer->user_data) : nullptr;
    if (self) self->updateAge();
}

void PreviewOverlay::dismissByUser() {
    emitEvent(EventType::PreviewClose);
}

} // namespace Heltec::MeshCore::Ui
