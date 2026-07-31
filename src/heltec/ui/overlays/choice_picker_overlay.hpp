#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../core/abstract_overlay.hpp"

struct _lv_obj_t;

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId ChoicePickerOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x09);
constexpr MetaId ChoicePickerTitle = ht_meta_id(MetaIdScope::Overlay, 0xD0);
constexpr MetaId ChoicePickerList = ht_meta_id(MetaIdScope::Overlay, 0xD1);
constexpr MetaId ChoicePickerRow = ht_meta_id(MetaIdScope::Overlay, 0xD2);
constexpr MetaId ChoicePickerFooter = ht_meta_id(MetaIdScope::Overlay, 0xD3);
constexpr MetaId ChoicePickerHeader = ht_meta_id(MetaIdScope::Overlay, 0xD4);
constexpr MetaId ChoicePickerBackButton = ht_meta_id(MetaIdScope::Overlay, 0xD5);
constexpr MetaId ChoicePickerBackIcon = ht_meta_id(MetaIdScope::Overlay, 0xD6);
}  // namespace meta_id

/** Lightweight data source used by the reusable, virtualized choice picker. */
class IChoicePickerSource {
 public:
  virtual ~IChoicePickerSource() = default;
  virtual const char* choicePickerTitle() const = 0;
  virtual int choicePickerOptionCount() = 0;
  virtual int choicePickerSelectedIndex() const = 0;
  virtual bool choicePickerOptionLabel(int index, char* buf, size_t cap) = 0;
  virtual void choicePickerCommit(int index) = 0;
  virtual void choicePickerClosed(bool committed) = 0;
};

class ChoicePickerOverlay final : public AbstractOverlay {
 public:
  explicit ChoicePickerOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  bool prepare(IChoicePickerSource* source);
  void onEnter() override;
  void onExit() override;
  void stepSelection(int8_t direction);

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  static constexpr uint8_t kMaxVisibleRows = 7;
  static constexpr size_t kRowTextSize = 48;

  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;

  static void onRowClicked(lv_event_t* event);
  static void onBackClicked(lv_event_t* event);
  void configureVisibleRows();
  void renderRows();
  void finish(bool commit, int selected_override = -1);
  int wrapIndex(int index) const;

  IChoicePickerSource* _source = nullptr;
  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _list = nullptr;
  _lv_obj_t* _rows[kMaxVisibleRows] = {};
  _lv_obj_t* _footer = nullptr;
  int _row_option[kMaxVisibleRows] = {};
  char _row_text[kMaxVisibleRows][kRowTextSize] = {};
  char _title_text[kRowTextSize] = {};
  int _selected = 0;
  int _original = 0;
  int _count = 0;
  uint8_t _visible_rows = 1;
  bool _finishing = false;
};

}  // namespace heltec::meshcore::ui
