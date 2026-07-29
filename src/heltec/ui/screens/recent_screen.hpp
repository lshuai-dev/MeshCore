#pragma once

#include "../core/abstract_screen.hpp"
#include "../core/app_state_event.hpp"
#include "../core/biz_facade.hpp"

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RecentRowLabel = ht_meta_id(MetaIdScope::Screen, 0x60);
constexpr MetaId RecentSendButton = ht_meta_id(MetaIdScope::Screen, 0x61);
constexpr MetaId RecentSendButtonLabel = ht_meta_id(MetaIdScope::Screen, 0x62);
constexpr MetaId RecentDetailContact = ht_meta_id(MetaIdScope::Screen, 0x63);
constexpr MetaId RecentDetailMessage = ht_meta_id(MetaIdScope::Screen, 0x64);
}

class RecentScreen : public AbstractScreen {
 public:
  RecentScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon)
      : AbstractScreen(biz, title, icon) {}
  eScreenId screenId() const override { return eScreenId::Recent; }
  void onEnter() override;
  void onExit() override;
  bool handleHorizontalSwipe(int8_t dir);

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;
  bool onKey(uint32_t key) override;

 private:
  enum class View : uint8_t {
    Conversations,
    ConversationDetail,
  };

  static constexpr int kConversationRows = 10;
  static constexpr int kConversationWindowStep = 5;
  static constexpr int kMaxRows = kConversationRows;
  static constexpr int kConversationRowTextSize = 56;
  static constexpr int kDetailHeaderTextSize = 48;
  static constexpr int kDetailMessageTextSize =
      heltec::meshcore::history::kMessageTextMax + 40;

  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void onAppStateChanged(const AppStateEvent& event) override;
  void onRefreshRequested() override;

  void refreshView();
  void refreshConversations();
  void refreshConversationDetail();
  void moveConversationSelection(int delta);
  void moveDetailMessage(bool older, bool wrap_at_boundary);
  void openSelectedConversation();
  void returnToConversations();
  void openReply();
  void focusRow(int row);
  void showRow(int row, const char* text, bool wrap, bool focusable = true);
  void hideRow(int row);
  void showDetailLabels(bool visible);
  void showSendButton(bool visible);
  static void onRowClicked(lv_event_t* event);
  static void onSendButtonClicked(lv_event_t* event);

  View _view = View::Conversations;
  bool _entered = false;

  int _conversation_window_start = 0;
  int _conversation_selected = 0;
  int _conversation_total = 0;
  int _conversation_count = 0;
  biz::IBizFacade::RecentConversationItem _conversation_items[kConversationRows]{};

  biz::IBizFacade::MessageConversationKey _active_key{};
  char _active_label[32]{};
  int _message_offset = 0;
  int _message_total = 0;
  uint32_t _detail_sequence = 0;
  bool _detail_outgoing = false;

  _lv_obj_t* _scroll = nullptr;
  _lv_obj_t* _rows[kMaxRows] = {};
  _lv_obj_t* _detail_contact = nullptr;
  _lv_obj_t* _detail_message = nullptr;
  _lv_obj_t* _send_button = nullptr;
  char _row_text[kMaxRows][kConversationRowTextSize] = {};
  char _detail_header_text[kDetailHeaderTextSize] = {};
  char _detail_message_text[kDetailMessageTextSize] = {};
};

}  // namespace heltec::meshcore::ui
