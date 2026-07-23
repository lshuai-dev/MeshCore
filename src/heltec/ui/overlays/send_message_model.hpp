#pragma once

#include <stddef.h>
#include <stdint.h>

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

class SendMessageModel {
 public:
  enum class Page : uint8_t { Main, TargetCategory, TargetList };
  enum class TargetKind : uint8_t { Broadcast, Personal, Group };

  struct Target {
    TargetKind kind = TargetKind::Broadcast;
    uint8_t pub_key_prefix[6]{};
    int channel_idx = -1;
    char label[24] = "broadcast";
  };

  static constexpr int kMaxContacts = 12;
  static constexpr int kMaxListItems = 12;
  static constexpr int kMainRowCount = 7;
  static constexpr int kMaxCategoryRows = 3;

  static void safeCopy(char* dest, size_t dest_size, const char* src);

  void reset(const biz::IBizFacade& biz);
  void syncContacts(const biz::IBizFacade& biz);
  void rebuildTargetCategories(const biz::IBizFacade& biz);
  void rebuildTargetList(const biz::IBizFacade& biz, int list_kind);
  void rebuildMainRows();

  Page page() const { return _page; }
  void setPage(Page page);
  int selectedIndex() const { return static_cast<int>(_select); }
  void setSelectedIndex(int index);
  int wrapIndex(int index) const;

  int rowCount() const;
  const char* rowLabel(int index) const;

  const Target& target() const { return _target; }
  void setTarget(const Target& target) { _target = target; }
  void resetTarget();

  int listKind() const { return _list_kind; }
  void setListKind(int list_kind) { _list_kind = list_kind; }
  int listCount() const { return _list_count; }
  int listChannelIndex(int index) const;
  bool contactAt(int index, uint8_t pub_key_prefix[6], char* label, size_t label_len) const;

 private:
  struct CachedContact {
    bool used = false;
    uint8_t pub_key_prefix[6]{};
    char name[32]{};
  };

  Page _page = Page::Main;
  int8_t _select = 0;
  int8_t _row_count = 0;

  Target _target{};
  CachedContact _contacts[kMaxContacts]{};
  int _contact_count = 0;

  char _main_rows[kMainRowCount][28]{};
  char _cat_rows[kMaxCategoryRows][16]{};
  int _cat_count = 0;
  char _list_rows[kMaxListItems][28]{};
  int _list_channel_idx[kMaxListItems]{};
  int _list_count = 0;
  int _list_kind = 0;
};

}  // namespace heltec::meshcore::ui
