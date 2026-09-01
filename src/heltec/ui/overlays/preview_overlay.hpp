#pragma once
#include "../core/framed_overlay.hpp"

#include <stdint.h>

namespace heltec::meshcore::biz
{
class IBizFacade;
}
struct _lv_obj_t;
struct _lv_timer_t;
namespace heltec::meshcore::ui
{

class PreviewOverlay : public FramedOverlay
{
  public:
    explicit PreviewOverlay(biz::IBizFacade &biz)
        : FramedOverlay(biz, Options{ false, true })
    {
    }

    void applyContent(uint8_t unread, uint32_t received_ms, const char *origin, const char *text);
    void dismissByUser();
    void onEnter() override;
    void onExit() override;

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    _lv_obj_t *createRoot(_lv_obj_t *parent) override;
    _lv_obj_t *focusTarget() const override;
    bool onKey(uint32_t key) override;
    void updateAge();
    static void ageTimerCallback(_lv_timer_t *timer);

    _lv_obj_t *_title = nullptr;
    _lv_obj_t *_age = nullptr;
    _lv_obj_t *_origin = nullptr;
    _lv_obj_t *_text = nullptr;
    _lv_timer_t *_age_timer = nullptr;
    uint32_t _received_ms = 0;
    char _title_text[16] = {};
    char _age_text[12] = {};
    char _origin_text[62] = {};
    char _message_text[78] = {};
};

} // namespace heltec::meshcore::ui
