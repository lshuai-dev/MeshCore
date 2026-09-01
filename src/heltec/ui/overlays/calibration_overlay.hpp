#pragma once
#include "../core/dialog_overlay.hpp"
#include "../core/app_state_event.hpp"

#include <stdint.h>

namespace heltec::meshcore::biz
{
class IBizFacade;
}
struct _lv_obj_t;
namespace heltec::meshcore::ui
{

class CalibrationOverlay : public DialogOverlay
{
  public:
    explicit CalibrationOverlay(biz::IBizFacade &biz)
        : DialogOverlay(biz, Options{Variant::Fullscreen, false, true, false})
    {
    }

    void onEnter() override;
    void onExit() override;

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void onSurfaceRefresh() override;
    enum class Phase : uint8_t
    {
        Idle,
        Calibrating,
        Success,
        Fail
    };

    void confirm();
    void openSession();
    void closeSession();
    void render();
    bool persistCalibrationIfNeeded();
    void evaluateQuality(int quality);
    void checkTimeout();
    void startTimeoutTimer();
    void stopTimeoutTimer();
    _lv_obj_t *focusTarget() const override;
    bool onKey(uint32_t key) override;
    static void timeoutTimerCb(lv_timer_t *timer);

    _lv_obj_t *_body = nullptr;
    _lv_obj_t *_footer = nullptr;
    char _body_text[48] = {};
    char _footer_text[64] = {};

    Phase _phase = Phase::Idle;
    uint32_t _deadline_ms = 0;
    bool _no_sensor = false;
    uint8_t _quality_streak = 0;
    int8_t _last_q = -1;
    bool _persisted = false;
    bool _save_failed = false;
    lv_timer_t *_timeout_timer = nullptr;
};

} // namespace heltec::meshcore::ui
