#pragma once
#include "../core/FramedOverlay.hpp"

namespace Heltec::MeshCore::Biz {
class IBizFacade;
}
struct _lv_obj_t;
struct _lv_timer_t;
namespace Heltec::MeshCore::Ui {

class PreviewOverlay final : public FramedOverlay {
  public:
    explicit PreviewOverlay(Biz::IBizFacade &biz);

    /** origin 和 text 来自固定消息环，在下次 applyContent() 前必须保持有效。 */
    void applyContent(uint8_t unread, uint32_t received_ms, const char *origin, const char *text);

  protected:
    void onEnter() override;
    void onExit() override;
    bool build() override;
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
};

} // namespace Heltec::MeshCore::Ui
