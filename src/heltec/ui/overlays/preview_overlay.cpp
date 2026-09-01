#include "preview_overlay.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ht_selector_view.hpp"
#include "ui/core/ui_semantic_classes.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ht_event.h"

#include <Arduino.h>
#include <lvgl.h>

namespace heltec::meshcore::ui
{

_lv_obj_t *PreviewOverlay::createRoot(_lv_obj_t *parent)
{
    return ht_semantic_create(&ht_preview_overlay_class, parent);
}

_lv_obj_t *PreviewOverlay::create(lv_obj_t *parent)
{
    if (!FramedOverlay::create(parent))
        return nullptr;
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        _root,
        [](lv_event_t *e)
        {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED)
                return;
            auto *self = static_cast<PreviewOverlay *>(lv_event_get_user_data(e));
            if (!self)
                return;
            lv_event_stop_bubbling(e);
            lv_event_stop_processing(e);
            self->emit(EventType::PREVIEW_NEXT);
        },
        LV_EVENT_CLICKED, this);

    lv_obj_t *row = ht_semantic_create(&ht_preview_metadata_row_class, contentHost());
    if (!row)
        return nullptr;
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    _title = ht_label_create(row);
    _age = ht_label_create(row);
    if (!_title || !_age)
        return nullptr;
    lv_label_set_text_static(_title, _title_text);
    lv_label_set_text_static(_age, _age_text);

    _origin = ht_semantic_label_create(&ht_preview_origin_class, contentHost());
    _text = ht_semantic_label_create(&ht_preview_text_class, contentHost());
    if (!_origin || !_text)
        return nullptr;
    lv_label_set_text_static(_origin, _origin_text);
    lv_label_set_text_static(_text, _message_text);
    lv_obj_set_width(_origin, lv_pct(100));
    lv_label_set_long_mode(_origin, LV_LABEL_LONG_DOT);
    lv_obj_set_width(_text, lv_pct(100));
    lv_obj_set_flex_grow(_text, 1);
    lv_label_set_long_mode(_text, LV_LABEL_LONG_WRAP);

    setFooterText(operation_hint::kPreviewClose);

    _age_timer = lv_timer_create(ageTimerCallback, 1000U, this);
    if (!_age_timer)
        return nullptr;
    lv_timer_set_repeat_count(_age_timer, -1);
    lv_timer_pause(_age_timer);

    return _root;
}

_lv_obj_t *PreviewOverlay::focusTarget() const
{
    return _root;
}

bool PreviewOverlay::onKey(uint32_t key)
{
    if (key == LV_KEY_NEXT || key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_RIGHT ||
        key == LV_KEY_UP || key == LV_KEY_DOWN)
    {
        emit(EventType::PREVIEW_NEXT);
        return true;
    }
    if (key == LV_KEY_ENTER || key == LV_KEY_ESC)
    {
        dismissByUser();
        return true;
    }
    return false;
}

void PreviewOverlay::applyContent(uint8_t unread, uint32_t received_ms, const char *origin,
                                  const char *text)
{
    if (!_root)
        return;
    lv_snprintf(_title_text, sizeof(_title_text), "Unread:%u", (unsigned)unread);
    _received_ms = received_ms;
    updateAge();
    lv_snprintf(_origin_text, sizeof(_origin_text), "%s", origin ? origin : "");
    lv_snprintf(_message_text, sizeof(_message_text), "%s", text ? text : "");
    lv_label_set_text_static(_title, _title_text);
    lv_label_set_text_static(_origin, _origin_text);
    lv_label_set_text_static(_text, _message_text);
}

void PreviewOverlay::updateAge()
{
    const uint32_t age_sec = (millis() - _received_ms) / 1000U;
    if (age_sec < 60)
    {
        lv_snprintf(_age_text, sizeof(_age_text), "%us", (unsigned)age_sec);
    }
    else if (age_sec < 3600)
    {
        lv_snprintf(_age_text, sizeof(_age_text), "%um", (unsigned)(age_sec / 60));
    }
    else
    {
        lv_snprintf(_age_text, sizeof(_age_text), "%uh", (unsigned)(age_sec / 3600));
    }
    lv_label_set_text_static(_age, _age_text);
}

void PreviewOverlay::onEnter()
{
    updateAge();
    if (_age_timer)
    {
        lv_timer_reset(_age_timer);
        lv_timer_resume(_age_timer);
    }
    AbstractOverlay::onEnter();
}

void PreviewOverlay::onExit()
{
    if (_age_timer)
        lv_timer_pause(_age_timer);
}

void PreviewOverlay::ageTimerCallback(lv_timer_t *timer)
{
    auto *self = timer ? static_cast<PreviewOverlay *>(timer->user_data) : nullptr;
    if (self)
        self->updateAge();
}

void PreviewOverlay::dismissByUser()
{
    emit(EventType::PREVIEW_CLOSE);
}

} // namespace heltec::meshcore::ui
