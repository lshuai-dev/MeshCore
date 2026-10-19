#pragma once
#include "../core/FramedOverlay.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}
struct _lv_obj_t;
struct _lv_timer_t;
namespace heltec::meshcore::ui {

class PreviewOverlay : public FramedOverlay {
  public:
    explicit PreviewOverlay(biz::IBizFacade &biz);

    void applyContent(uint8_t unread, uint32_t received_ms, const char *origin, const char *text);
    void dismissByUser();
    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;
    bool buildHeader() override;

  private:
    bool onKey(uint32_t key) override;
    void updateAge();
    void updateMessageScroll();
    static void onRootClicked(lv_event_t *event);
    static void ageTimerCallback(_lv_timer_t *timer);

    _lv_obj_t *_unread_label = nullptr;
    _lv_obj_t *_age_label = nullptr;
    _lv_obj_t *_origin_label = nullptr;
    _lv_obj_t *_message_viewport = nullptr;
    _lv_obj_t *_message_label = nullptr;
    _lv_timer_t *_age_timer = nullptr;
    uint32_t _received_ms = 0;
    char _unread_text[10] = {};
    char _age_text[8] = {};
    char _origin_text[62] = {};
    char _message_text[161] = {};
};

} // namespace heltec::meshcore::ui
