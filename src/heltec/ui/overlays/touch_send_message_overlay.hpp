#pragma once

#if HELTEC_HAS_TOUCH

#include "../core/framed_overlay.hpp"
#include "send_message_model.hpp"

#include <stdint.h>

namespace heltec::meshcore::biz
{
class IBizFacade;
}

struct _lv_obj_t;

namespace heltec::meshcore::ui
{

struct UiSendMessageTarget;

/** V4 R8 touch-first send message overlay. */
class SendMessageOverlay : public FramedOverlay
{
  public:
    explicit SendMessageOverlay(biz::IBizFacade &biz);
    void submitCustomMessage(const char *text);
    void prepareTarget(const UiSendMessageTarget *target);

    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;

  private:
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
    _lv_obj_t *createTouchRow(uint8_t index, const char *text);
    void syncSelectionVisual();
    void applySelection();
    void handleBack();
    void sendMessageText(const char *text, bool alert_on_direct_failure = false);
    void scheduleSendResultAlert(bool ok);
    static void showSendResultAlertAsync(void *user_data);
    void openCustomKeyboard();
    void setTarget(const Target &t);
    void hideVisual();

    int rowCount() const;
    int selectedIndex() const;
    const char *rowLabel(int index) const;

    _lv_obj_t *_list_mid = nullptr;
    _lv_obj_t *_list = nullptr;
    _lv_obj_t *_empty_focus = nullptr;
    _lv_obj_t *_row_objs[kMaxListItems] = {};
    _lv_obj_t *_row_labels[kMaxListItems] = {};
    char _row_text[kMaxListItems][32] = {};

    SendMessageModel _model;
    Target _pending_target{};
    bool _pending_target_valid = false;
    bool _syncing_focus = false;
    bool _send_alert_ok = false;
    bool _send_alert_scheduled = false;
};

} // namespace heltec::meshcore::ui

#endif // HELTEC_HAS_TOUCH
