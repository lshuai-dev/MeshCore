#pragma once
#include <stdint.h>
#include "../core/abstract_overlay.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}
struct _lv_event_t;
struct _lv_obj_t;
namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId KeyboardOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x04);
constexpr MetaId KeyboardTitle = ht_meta_id(MetaIdScope::Overlay, 0x80);
constexpr MetaId KeyboardTextarea = ht_meta_id(MetaIdScope::Overlay, 0x81);
constexpr MetaId KeyboardKeyboard = ht_meta_id(MetaIdScope::Overlay, 0x82);
constexpr MetaId KeyboardSpacer = ht_meta_id(MetaIdScope::Overlay, 0x83);
}

class KeyboardOverlay : public AbstractOverlay {
 public:
  explicit KeyboardOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  bool prepareMessageInput(const char* title);
  bool prepareWaypointInput();
  bool isWaypointCompose() const { return _compose_mode == ComposeMode::Waypoint; }

  void onEnter() override;
  void onExit() override;

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override { return _keyboard; }
  bool onKey(uint32_t key) override;

  enum class ComposeMode : uint8_t { Message, Waypoint };

  static void on_keyboard_events(_lv_event_t* e);
  static void on_keyboard_value_pre(_lv_event_t* e);
  static void on_keyboard_key_pre(_lv_event_t* e);
  static void on_keyboard_key_post(_lv_event_t* e);
  bool ensureContent();
  void submitMessageInput();
  void submitWaypointInput();

  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _textarea = nullptr;
  _lv_obj_t* _vertical_spacer = nullptr;
  _lv_obj_t* _keyboard = nullptr;

  ComposeMode _compose_mode = ComposeMode::Message;
  bool _skip_next_ok_value = false;
  uint32_t _skip_next_ok_value_ms = 0;

  static constexpr int kMaxText = 80;
  char _title_text[40]{};
  char _text_buffer[kMaxText + 1]{};
  char _submitted_text[kMaxText + 1]{};
};

}  // namespace heltec::meshcore::ui
