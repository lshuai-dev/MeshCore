#pragma once

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

#include <stdint.h>

#include "../core/abstract_overlay.hpp"
#include "send_message_overlay_ids.hpp"
#include "send_message_model.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

struct _lv_obj_t;

namespace heltec::meshcore::ui {

/** V4 R8 touch-first send message overlay. */
class SendMessageOverlay : public AbstractOverlay {
 public:
  explicit SendMessageOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}
  using UiSurface::setTarget;

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void submitCustomMessage(const char* text);

  lv_obj_t* focusedObject() const override { return _list ? _list : _root; }
  void onEnter() override;
  void onExit() override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  bool onKey(uint32_t key) override;
  void syncOverlayGroup();

  using Page = SendMessageModel::Page;
  using TargetKind = SendMessageModel::TargetKind;
  using Target = SendMessageModel::Target;
  static constexpr int kMaxListItems = SendMessageModel::kMaxListItems;

  void syncContacts();
  void rebuildTargetCategories();
  void rebuildTargetList();
  void rebuildMainRows();
  void renderRows();
  void renderTouchList();
  void clearTouchList();
  _lv_obj_t* createTouchRow(uint8_t index, const char* text);
  void syncSelectionVisual();
  void applySelection();
  void handleBack();
  void sendMessageText(const char* text, bool alert_on_direct_failure = false);
  void scheduleSendResultAlert(bool ok);
  static void showSendResultAlertAsync(void* user_data);
  void openCustomKeyboard();
  void setTarget(const Target& t);
  void hideVisual();

  int rowCount() const;
  int selectedIndex() const;
  const char* rowLabel(int index) const;

  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _list_mid = nullptr;
  _lv_obj_t* _list = nullptr;
  _lv_obj_t* _footer = nullptr;
  _lv_obj_t* _row_objs[kMaxListItems] = {};
  _lv_obj_t* _row_labels[kMaxListItems] = {};
  char _row_text[kMaxListItems][32] = {};

  SendMessageModel _model;
  bool _syncing_focus = false;
  bool _send_alert_ok = false;
  bool _send_alert_scheduled = false;
};

}  // namespace heltec::meshcore::ui

#endif  // HELTEC_V4_R8_TFT && HELTEC_HAS_TOUCH
