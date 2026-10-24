#pragma once

#include "../core/FramedOverlay.hpp"
#include "SendMessageModel.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
}

struct _lv_obj_t;

namespace Heltec::MeshCore::Ui
{

struct UiSendMessageTarget;

/** @brief Message composer overlay selected at compile time for touch or keypad input. */
class SendMessageOverlay final : public FramedOverlay
{
  public:
    explicit SendMessageOverlay(Biz::IBizFacade &biz);
    void submitCustomMessage(const char *text);
    void prepareTarget(const UiSendMessageTarget *target);


  protected:
    void onEnter() override;
    void onExit() override;
    bool build() override;

  private:
    using Page = SendMessageModel::Page;
    using TargetKind = SendMessageModel::TargetKind;
    using Target = SendMessageModel::Target;

    bool onKey(uint32_t key) override;
    void syncContacts();
    void rebuildTargetCategories();
    void rebuildTargetList();
    void rebuildMainRows();
    void renderRows();
    void applySelection();
    void handleBack();
    void sendMessageText(const char *text, bool alert_on_direct_failure = false);
    void scheduleSendResultAlert(bool ok);
    static void showSendResultAlertAsync(void *user_data);
    void openCustomKeyboard();
    void setTarget(const Target &target);
    void hideVisual();
    int rowCount() const;
    const char *rowLabel(int index) const;

#if HELTEC_UI_HAS_TOUCH
    static constexpr int kMaxListItems = SendMessageModel::kMaxListItems;

    void syncOverlayGroup();
    void renderTouchList();
    void clearTouchList();
    _lv_obj_t *createTouchRow(uint8_t index, const char *text);
    void syncSelectionVisual();
    int selectedIndex() const;

    _lv_obj_t *_list = nullptr;
    _lv_obj_t *_empty_focus = nullptr;
    _lv_obj_t *_row_objs[kMaxListItems] = {};
    _lv_obj_t *_row_labels[kMaxListItems] = {};
    char _row_text[kMaxListItems][32] = {};
    bool _syncing_focus = false;
#else
    static constexpr int kMaxVisibleRows = 5;

    bool ensureRowPool();
    int visibleRowCount() const;
    int focusRow() const;
    int wrapIndex(int index) const;

    _lv_obj_t *_row_objs[kMaxVisibleRows] = {};
    char _row_text[kMaxVisibleRows][SendMessageModel::kRowTextCapacity] = {};
#if defined(HELTEC_LORA_V4_TFT)
    _lv_obj_t *_row_labels[kMaxVisibleRows] = {};
#endif
    int8_t _visible_row_count = 0;
    bool _confirm_pending = false;
#endif

    _lv_obj_t *_list_mid = nullptr;
    SendMessageModel _model;
    Target _pending_target = {};
    bool _pending_target_valid = false;
    bool _send_alert_ok = false;
    bool _send_alert_scheduled = false;
};

} // namespace Heltec::MeshCore::Ui
