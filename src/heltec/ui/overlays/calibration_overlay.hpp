#pragma once
#include <stdint.h>
#include "../core/abstract_overlay.hpp"
#include "../core/app_state_event.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}
struct _lv_obj_t;
namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId CalibrationOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x03);
constexpr MetaId CalibrationPanel = ht_meta_id(MetaIdScope::Overlay, 0x60);
constexpr MetaId CalibrationBody = ht_meta_id(MetaIdScope::Overlay, 0x61);
constexpr MetaId CalibrationFooter = ht_meta_id(MetaIdScope::Overlay, 0x62);
}

class CalibrationOverlay : public AbstractOverlay {
 public:
  explicit CalibrationOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void onEnter() override;
  void onExit() override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;
  enum class Phase : uint8_t { Idle, Calibrating, Success, Fail };

  void confirm();
  void openSession();
  void closeSession();
  void render();
  bool persistCalibrationIfNeeded();
  void evaluateQuality(int quality);
  void checkTimeout();
  void startTimeoutTimer();
  void stopTimeoutTimer();
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;
  static void timeoutTimerCb(lv_timer_t* timer);

  _lv_obj_t* _panel = nullptr;
  _lv_obj_t* _body = nullptr;
  _lv_obj_t* _footer = nullptr;

  Phase _phase = Phase::Idle;
  uint32_t _deadline_ms = 0;
  bool _no_sensor = false;
  uint8_t _quality_streak = 0;
  int8_t _last_q = -1;
  bool _persisted = false;
  bool _save_failed = false;
  lv_timer_t* _timeout_timer = nullptr;
};

}  // namespace heltec::meshcore::ui
