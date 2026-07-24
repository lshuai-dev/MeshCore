#pragma once

#include "send_message_overlay_ids.hpp"

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#include "touch_send_message_overlay.hpp"
#else

#include <stdint.h>

#include "../core/abstract_overlay.hpp"
#include "send_message_model.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

struct _lv_obj_t;

namespace heltec::meshcore::ui {

/** @brief 发消息 overlay；由 UiApp::activate 驱动显隐。 */
class SendMessageOverlay : public AbstractOverlay {
 public:
  explicit SendMessageOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}
  using UiSurface::setTarget;

  void submitCustomMessage(const char* text);

  void onEnter() override;
  void onExit() override;

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;

  using Page = SendMessageModel::Page;
  using TargetKind = SendMessageModel::TargetKind;
  using Target = SendMessageModel::Target;

  static constexpr int kMaxVisibleRows = 5;

  void syncContacts();
  void rebuildTargetCategories();
  void rebuildTargetList();
  void rebuildMainRows();
  void renderRows();
  bool ensureRowPool();
  int visibleRowCount() const;
  int focusRow() const;
  int wrapIndex(int index) const;
  void applySelection();
  void handleBack();
  void sendMessageText(const char* text, bool alert_on_direct_failure = false);
  void scheduleSendResultAlert(bool ok);
  static void showSendResultAlertAsync(void* user_data);
  void openCustomKeyboard();
  void setTarget(const Target& t);
  void hideVisual();

  int rowCount() const;
  const char* rowLabel(int index) const;

  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _list_mid = nullptr;
  _lv_obj_t* _footer = nullptr;
  _lv_obj_t* _row_objs[kMaxVisibleRows]{};

  int8_t _visible_row_count = 0;

  SendMessageModel _model;
  bool _confirm_pending = false;
  bool _send_alert_ok = false;
  bool _send_alert_scheduled = false;
};

}  // namespace heltec::meshcore::ui

#endif  // HELTEC_V4_R8_TFT && HELTEC_HAS_TOUCH
