#include "QuickPingOverlay.hpp"

#if HELTEC_UI_HAS_TOUCH

#include "ui/widgets/DropdownViewport.hpp"
#include "ui/widgets/HtTextarea.hpp"

#include "ui/widgets/HtContainer.hpp"
#include "ui/widgets/SemanticClasses.hpp"

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

namespace
{

lv_obj_class_t quick_ping_class(const lv_obj_class_t *base_class, uint32_t instance_size)
{
    return {
        base_class,
        nullptr,
        nullptr,
#if LV_USE_USER_DATA
        nullptr,
#endif
        nullptr,
        LV_SIZE_CONTENT,
        LV_SIZE_CONTENT,
        LV_OBJ_CLASS_EDITABLE_INHERIT,
        LV_OBJ_CLASS_GROUP_DEF_INHERIT,
        instance_size,
    };
}

} // namespace

const lv_obj_class_t ht_quick_ping_overlay_class =
    quick_ping_class(&lv_obj_class, sizeof(lv_obj_t));
const lv_obj_class_t ht_quick_ping_backdrop_class =
    quick_ping_class(&lv_obj_class, sizeof(lv_obj_t));
const lv_obj_class_t ht_quick_ping_title_bar_class =
    quick_ping_class(&lv_obj_class, sizeof(lv_obj_t));
const lv_obj_class_t ht_quick_ping_content_class =
    quick_ping_class(&lv_obj_class, sizeof(lv_obj_t));
const lv_obj_class_t ht_quick_ping_message_controls_class =
    quick_ping_class(&ht_container_class, sizeof(ht_container_t));
const lv_obj_class_t ht_quick_ping_keyboard_editor_class =
    quick_ping_class(&ht_textarea_class, sizeof(ht_textarea_t));
const lv_obj_class_t ht_quick_ping_keyboard_counter_class =
    quick_ping_class(&lv_label_class, sizeof(lv_label_t));
const lv_obj_class_t ht_quick_ping_title_class =
    quick_ping_class(&lv_label_class, sizeof(lv_label_t));
const lv_obj_class_t ht_quick_ping_label_class =
    quick_ping_class(&lv_label_class, sizeof(lv_label_t));
const lv_obj_class_t ht_quick_ping_dropdown_class =
    quick_ping_class(&lv_dropdown_class, sizeof(lv_dropdown_t));
const lv_obj_class_t ht_quick_ping_message_dropdown_class =
    quick_ping_class(&ht_quick_ping_dropdown_class, sizeof(lv_dropdown_t));
const lv_obj_class_t ht_quick_ping_message_editor_class =
    quick_ping_class(&ht_textarea_class, sizeof(ht_textarea_t));
const lv_obj_class_t ht_quick_ping_keyboard_class =
    quick_ping_class(&lv_keyboard_class, sizeof(lv_keyboard_t));

} // namespace Heltec::MeshCore::Ui

#include "heltec/ui/core/BizFacade.hpp"
#include "heltec/ui/Images.h"
#include "ui/theme/UiTheme.hpp"
#include "ui/core/UiMessage.hpp"
#include "ui/core/Scheduler.hpp"
#include "ui/theme/WidgetTheme.hpp"

#include <cstring>

namespace Heltec::MeshCore::Ui
{
namespace
{

constexpr const char *kTargetOptions = "group\nbroadcast\npersonal\nadvert";
constexpr lv_coord_t kPaneInset = 3;
constexpr lv_coord_t kPaneWidth = 234;
constexpr lv_coord_t kPaneHeight = 314;
constexpr lv_coord_t kTitleBarHeight = 39;
constexpr lv_coord_t kDropdownRowHeight = 40;
constexpr lv_coord_t kDropdownControlX = 81;
constexpr lv_coord_t kDropdownControlY = 3;
constexpr lv_coord_t kDropdownControlWidth = 130;
constexpr lv_coord_t kDropdownControlHeight = 32;
constexpr lv_coord_t kRowLabelX = 12;
constexpr lv_coord_t kRowLabelWidth = 70;
constexpr lv_coord_t kIconBadgeSize = 28;
constexpr lv_coord_t kIconBadgeX = 2;
constexpr lv_coord_t kIconLabelX = 33;
constexpr lv_coord_t kIconLabelWidth = 56;
constexpr lv_coord_t kIconDropdownControlX = 91;
constexpr lv_coord_t kIconDropdownControlWidth = 120;
constexpr lv_coord_t kMessageDropdownWidth = 26;

bool focus_object_available(const lv_obj_t *obj)
{
    if (!obj || !lv_obj_is_valid(obj) || lv_obj_has_state(obj, LV_STATE_DISABLED))
        return false;
    for (const lv_obj_t *current = obj; current; current = lv_obj_get_parent(current))
    {
        if (lv_obj_has_flag(current, LV_OBJ_FLAG_HIDDEN))
            return false;
    }
    return true;
}

lv_obj_t *create_object(const lv_obj_class_t *class_p, lv_obj_t *parent)
{
    lv_obj_t *const object = lv_obj_class_create_obj(class_p, parent);
    if (object)
        lv_obj_class_init_obj(object);
    return object;
}
constexpr lv_coord_t kMessageInputWidth = kIconDropdownControlWidth - kMessageDropdownWidth;
constexpr lv_coord_t kKeyboardEditorHeight = 40;
constexpr lv_coord_t kKeyboardEditorInset = 4;

#ifndef QUICK_PING_SLIDE_ANIM_MS
#define QUICK_PING_SLIDE_ANIM_MS 420
#endif

static const char *kMessageCompactKbMap[] = {
    "a",          "b",   "c",   "d",  "e",
    "f",          "g",   "\n",  "h",  "i",
    "j",          "k",   "l",   "m",  "n",
    "\n",         "o",   "p",   "q",  "r",
    "s",          "t",   "u",   "\n", "v",
    "w",          "x",   "y",   "z",  LV_SYMBOL_BACKSPACE,
    "\n",         "123", "ABC", "SP", LV_SYMBOL_CLOSE,
    LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactKbCtrl[] = {
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    4,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
};

static const char *kMessageCompactUpperKbMap[] = {
    "A",          "B",   "C",   "D",  "E",
    "F",          "G",   "\n",  "H",  "I",
    "J",          "K",   "L",   "M",  "N",
    "\n",         "O",   "P",   "Q",  "R",
    "S",          "T",   "U",   "\n", "V",
    "W",          "X",   "Y",   "Z",  LV_SYMBOL_BACKSPACE,
    "\n",         "123", "abc", "SP", LV_SYMBOL_CLOSE,
    LV_SYMBOL_OK, "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactUpperKbCtrl[] = {
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    4,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
};

static const char *kMessageCompactSymbolKbMap[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "\n",
    "8",
    "9",
    "0",
    "@",
    "#",
    "$",
    "%",
    "\n",
    "&",
    "*",
    "+",
    "-",
    "=",
    "/",
    "_",
    "\n",
    ".",
    ",",
    "?",
    "!",
    ":",
    ";",
    "\"",
    "\n",
    "abc",
    "SP",
    LV_SYMBOL_BACKSPACE,
    LV_SYMBOL_CLOSE,
    LV_SYMBOL_OK,
    "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactSymbolKbCtrl[] = {
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    4,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
};

bool sameKey(const uint8_t lhs[6], const uint8_t rhs[6])
{
    return memcmp(lhs, rhs, 6) == 0;
}

size_t normalizeSingleLineText(const char *source, char *dest, size_t dest_size)
{
    if (!dest || dest_size == 0)
        return 0;
    if (!source)
    {
        dest[0] = '\0';
        return 0;
    }

    size_t written = 0;
    bool pending_space = false;
    for (size_t i = 0; source[i] != '\0'; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(source[i]);
        if (c <= 32 || c == 127)
        {
            if (written > 0)
                pending_space = true;
            continue;
        }
        if (pending_space)
        {
            if (written + 1 >= dest_size)
                break;
            dest[written++] = ' ';
            pending_space = false;
        }
        if (written + 1 >= dest_size)
            break;
        dest[written++] = static_cast<char>(c);
    }
    dest[written] = '\0';
    return written;
}

void sanitizeOptionLabel(char *label, size_t label_size)
{
    if (!label || label_size == 0)
        return;
    if (normalizeSingleLineText(label, label, label_size) > 0)
        return;
    strncpy(label, "?", label_size - 1);
    label[label_size - 1] = '\0';
}

void syncControlFocusToRow(lv_event_t *event)
{
    if (!event)
    {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
    {
        return;
    }

    _lv_obj_t *const control = lv_event_get_target(event);
    _lv_obj_t *const row = control ? lv_obj_get_parent(control) : nullptr;
    if (!row || !lv_obj_has_class(row, &ht_container_class))
    {
        return;
    }

    ht_container_set_selected(row, code == LV_EVENT_FOCUSED);
}

_lv_obj_t *createFieldIcon(_lv_obj_t *row, const lv_img_dsc_t *image)
{
    if (!row || !image)
        return nullptr;

    _lv_obj_t *badge = ht_class_create(&ht_quick_ping_badge_class, row);
    if (!badge)
        return nullptr;
    lv_obj_set_size(badge, kIconBadgeSize, kIconBadgeSize);
    lv_obj_align(badge, LV_ALIGN_LEFT_MID, kIconBadgeX, 0);
    lv_obj_clear_flag(badge,
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    _lv_obj_t *icon = lv_img_create(badge);
    if (!icon)
        return badge;
    lv_img_set_src(icon, image);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_center(icon);
    return badge;
}

} // namespace

QuickPingOverlay::QuickPingOverlay(Biz::IBizFacade &biz)
    : AbstractOverlay(biz)
{
}

void QuickPingOverlay::prepareTarget(const UiSendMessageTarget *target)
{
    _preset_edit_active = false;
    _pending_target_valid = target != nullptr;
    if (!target)
        return;

    _pending_target_direct = target->kind == UiMessageTargetKind::Direct;
    _pending_target_channel = target->channel_idx;
    memcpy(_pending_target_key, target->pub_key_prefix, sizeof(_pending_target_key));
}

bool QuickPingOverlay::preparePresetEdit(uint8_t index, const char *text)
{
    if (index >= _biz.messagePresetCount() || !text || !_ta_message)
        return false;
    _pending_target_valid = false;
    _preset_edit_active = true;
    _preset_edit_index = index;
    lv_textarea_set_text(_ta_message, text);
    return true;
}

bool QuickPingOverlay::build()
{
    if (!AbstractOverlay::build())
        return false;

    lv_obj_t *const parent = lv_obj_get_parent(_root);
    if (!parent)
        return false;

    // Create the backdrop before the pane root so it stays above the existing
    // overlay siblings while the pane itself is kept on top by SurfaceManager.
    _backdrop = create_object(&ht_quick_ping_backdrop_class, parent);
    if (!_backdrop)
        return false;
    lv_obj_set_size(_backdrop, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_backdrop, 0, 0);
    lv_obj_clear_flag(_backdrop, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(_backdrop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(_root);

    auto fail = [this]() {
        lv_obj_del(_backdrop);
        _backdrop = nullptr;
        return false;
    };

    lv_obj_set_size(_root, kPaneWidth, kPaneHeight);
    lv_obj_align(_root, LV_ALIGN_TOP_LEFT, kPaneInset, kPaneInset);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_root, onOutsideEvent, LV_EVENT_CLICKED, this);

    _title_bar = create_object(&ht_quick_ping_title_bar_class, _root);
    if (!_title_bar)
        return fail();
    lv_obj_set_size(_title_bar, lv_pct(100), kTitleBarHeight);
    lv_obj_set_flex_flow(_title_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_title_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_title_bar, LV_OBJ_FLAG_SCROLLABLE);

    _title = ht_label_create(&ht_quick_ping_title_class, _title_bar, "Quick Ping");
    if (!_title)
        return fail();
    lv_obj_set_size(_title, lv_pct(100), LV_SIZE_CONTENT);
    lv_label_set_long_mode(_title, LV_LABEL_LONG_DOT);

    _content = create_object(&ht_quick_ping_content_class, _root);
    if (!_content)
        return fail();
    lv_obj_set_width(_content, lv_pct(100));
    lv_obj_set_flex_grow(_content, 1);
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(_content, LV_OBJ_FLAG_SCROLLABLE);

    _row_target = createDropdownRow(_content, "Target :", &quick_ping_target_img, &_dd_target);
    _row_recipient = createDropdownRow(_content, "to:", nullptr, &_dd_recipient);
    _row_message = createMessageRow(_content);
    _keyboard = createKeyboard(_content);
    if (!_row_target || !_row_recipient || !_row_message || !_keyboard)
    {
        return fail();
    }
    _rebuilding_navigation = true;
    _lv_obj_t *const focus_objects[] = {
        _dd_target,
        _dd_recipient,
        _ta_message,
        _dd_message,
        _keyboard,
    };
    for (_lv_obj_t *object : focus_objects)
    {
        lv_obj_add_flag(object, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        _gridNav.add(object);
    }
    _rebuilding_navigation = false;
    updateMessageTextPresentation(false);
    return true;
}

void QuickPingOverlay::onEnter()
{
    if (_backdrop && lv_obj_is_valid(_backdrop))
    {
        lv_obj_clear_flag(_backdrop, LV_OBJ_FLAG_HIDDEN);
    }
    _message_submit_handled = false;
    _close_animating = false;
    _edit_state = EditState::Browse;
    _editing_dropdown = nullptr;
    closeDropdowns();
    syncLists();
    rebuildMessagePresetOptions();
    applyPendingTarget();
    syncDropdownOptions();
    applyState(false);
    if (_preset_edit_active)
    {
        _message_submit_handled = false;
        setKeyboardVisible(true);
        restoreModeFocus(_keyboard);
    }
    lv_obj_update_layout(_root);
    updateMessageTextPresentation(false);
    startOpenAnimation();
}

void QuickPingOverlay::onExit()
{
    if (_backdrop && lv_obj_is_valid(_backdrop))
    {
        lv_obj_add_flag(_backdrop, LV_OBJ_FLAG_HIDDEN);
    }
    cancelMotion(this, slideExec);
    slideExec(this, kPaneInset);
    (void)lv_async_call_cancel(openPendingKeyboardAsync, this);
    clearRepeatSelection();
    _close_animating = false;
    _keyboard_open_pending = false;
    _message_submit_handled = false;
    _preset_edit_active = false;
    closeDropdowns();
    setKeyboardVisible(false);
}

void QuickPingOverlay::slideExec(void *var, int32_t value)
{
    auto *self = static_cast<QuickPingOverlay *>(var);
    if (!self || !self->_root || !lv_obj_is_valid(self->_root))
        return;

    const lv_coord_t y = static_cast<lv_coord_t>(value);
    lv_obj_set_y(self->_root, y);
    if (!self->_backdrop || !lv_obj_is_valid(self->_backdrop))
        return;

    lv_obj_set_y(self->_backdrop, y - kPaneInset);
    lv_obj_set_style_opa(self->_backdrop, LV_OPA_COVER, LV_PART_MAIN);
}

lv_coord_t QuickPingOverlay::paneHeight() const
{
    if (!_root || !lv_obj_is_valid(_root))
    {
        return 0;
    }

    lv_coord_t height = lv_obj_get_height(_root);
    if (height <= 0)
    {
        lv_obj_t *const parent = lv_obj_get_parent(_root);
        if (parent && lv_obj_is_valid(parent))
        {
            height = lv_obj_get_height(parent);
        }
    }
    return height;
}

void QuickPingOverlay::startOpenAnimation()
{
    if (!_root || !lv_obj_is_valid(_root))
        return;
    cancelMotion(this, slideExec);
    lv_obj_update_layout(_root);
    const lv_coord_t height = paneHeight();
    if (height <= 0 || QUICK_PING_SLIDE_ANIM_MS == 0)
    {
        slideExec(this, kPaneInset);
        return;
    }

    UiMotionSpec motion;
    motion.target = this;
    motion.exec = slideExec;
    motion.start_value = -height - kPaneInset;
    motion.end_value = kPaneInset;
    motion.duration_ms = QUICK_PING_SLIDE_ANIM_MS;
    motion.path = UiMotionPath::EaseInOut;
    if (!startMotion(motion))
    {
        slideExec(this, kPaneInset);
    }
}

bool QuickPingOverlay::requestCloseAnimation()
{
    if (!_root || !lv_obj_is_valid(_root))
        return false;
    if (_close_animating)
        return true;

    closeDropdowns();
    clearRepeatSelection();
    cancelMotion(this, slideExec);
    lv_obj_update_layout(_root);
    const lv_coord_t height = paneHeight();
    if (height <= 0 || QUICK_PING_SLIDE_ANIM_MS == 0)
        return false;
    const lv_coord_t current_y = lv_obj_get_y(_root);
    const lv_coord_t hidden_y = -height - kPaneInset;
    if (current_y <= hidden_y)
        return false;

    UiMotionSpec motion;
    motion.target = this;
    motion.exec = slideExec;
    motion.ready = closeAnimationReady;
    motion.ready_data = this;
    motion.start_value = current_y;
    motion.end_value = hidden_y;
    motion.duration_ms = QUICK_PING_SLIDE_ANIM_MS;
    motion.path = UiMotionPath::EaseInOut;
    if (!startMotion(motion))
        return false;
    _close_animating = true;
    return true;
}

void QuickPingOverlay::closeAnimationReady(void *user_data)
{
    auto *self = static_cast<QuickPingOverlay *>(user_data);
    if (!self)
        return;
    self->_close_animating = false;
    (void)self->notifyApp(UiMessageType::QuickPingClose);
}

bool QuickPingOverlay::onKey(uint32_t key)
{
    /* The keyboard owns navigation while visible. Do not let page-level
       browse handling consume its directional and confirm keys. */
    if (keyboardVisible() && key != LV_KEY_ESC)
    {
        if (key == LV_KEY_ENTER && _keyboard && lv_obj_is_valid(_keyboard))
        {
            const uint16_t selected = lv_keyboard_get_selected_btn(_keyboard);
            const char *label = lv_btnmatrix_get_btn_text(_keyboard, selected);
            if (label && strcmp(label, LV_SYMBOL_CLOSE) == 0)
            {
                (void)lv_event_send(_keyboard, LV_EVENT_CANCEL, nullptr);
                return true;
            }
        }
        return _gridNav.handleKey(key);
    }
    if (keyboardVisible() && key == LV_KEY_ESC)
    {
        /* Match KeyboardOverlay: only the keyboard cancel button closes it. */
        return true;
    }
    if (key == LV_KEY_UP || key == LV_KEY_PREV)
    {
        if (_edit_state == EditState::Browse)
        {
            _gridNav.focusPrevious();
            return true;
        }
        return false;
    }
    if (key == LV_KEY_DOWN || key == LV_KEY_NEXT)
    {
        if (_edit_state == EditState::Browse)
        {
            _gridNav.focusNext();
            return true;
        }
        return false;
    }
    if (key == LV_KEY_ESC)
    {
        if (_edit_state == EditState::Edit)
        {
            cancelDropdownEdit(_editing_dropdown);
            return true;
        }
    }
    if (key == LV_KEY_ENTER && _edit_state == EditState::Edit)
    {
        commitDropdownEdit(_editing_dropdown);
        return true;
    }
    if (key == LV_KEY_ENTER && _edit_state == EditState::Browse)
    {
        _lv_obj_t *focused = _gridNav.focused();
        if (isEditableDropdown(focused))
        {
            beginDropdownEdit(focused);
            lv_dropdown_open(focused);
            return true;
        }
    }
    if (key != LV_KEY_ESC)
        return false;
    clearRepeatSelection();
    if (closeDropdown(_dd_target))
        return true;
    if (closeDropdown(_dd_recipient))
        return true;
    if (closeDropdown(_dd_message))
        return true;
    return notifyApp(UiMessageType::QuickPingClose);
}

_lv_obj_t *QuickPingOverlay::createDropdownRow(_lv_obj_t *parent, const char *label_text,
                                               const lv_img_dsc_t *icon, _lv_obj_t **out_dropdown)
{
    if (out_dropdown)
        *out_dropdown = nullptr;
    _lv_obj_t *row = ht_class_create(&ht_quick_ping_row_class, parent);
    if (!row)
        return nullptr;
    lv_obj_set_size(row, lv_pct(100), kDropdownRowHeight);
    lv_obj_set_layout(row, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_CLICKED, this);

    const bool has_icon = icon != nullptr;
    _lv_obj_t *caption = row;
    if (has_icon)
    {
        caption = lv_obj_create(row);
        if (caption)
        {
            lv_obj_set_width(caption, LV_SIZE_CONTENT);
            lv_obj_set_height(caption, kDropdownRowHeight);
            lv_obj_set_style_max_width(caption, kIconDropdownControlX, LV_PART_MAIN);
            lv_obj_set_layout(caption, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(caption, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(caption, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);
            lv_obj_clear_flag(caption, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(caption, 0, 0);
            createFieldIcon(caption, icon);
        }
    }

    _lv_obj_t *label =
        ht_label_create(&ht_quick_ping_label_class, caption, label_text ? label_text : "");
    if (label)
    {
        lv_obj_set_width(label, has_icon ? LV_SIZE_CONTENT : kRowLabelWidth);
        if (has_icon)
            lv_obj_set_style_max_width(label, kIconDropdownControlX - kIconBadgeSize, LV_PART_MAIN);
        if (!has_icon)
            lv_obj_align(label, LV_ALIGN_LEFT_MID, kRowLabelX, 0);
        lv_label_set_long_mode(label, has_icon ? LV_LABEL_LONG_CLIP : LV_LABEL_LONG_DOT);
    }

    _lv_obj_t *dropdown = ht_class_create(&ht_quick_ping_dropdown_class, row);
    if (!dropdown)
        return row;
    const lv_coord_t control_x = has_icon ? kIconDropdownControlX : kDropdownControlX;
    const lv_coord_t control_width = has_icon ? kIconDropdownControlWidth : kDropdownControlWidth;
    lv_obj_set_size(dropdown, control_width, kDropdownControlHeight);
    lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, control_x, kDropdownControlY);
    lv_dropdown_set_dir(dropdown, LV_DIR_BOTTOM);
    lv_obj_add_event_cb(dropdown, onDropdownConfirmPreprocess,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
    lv_obj_add_event_cb(dropdown, onDropdownConfirmPreprocess,
                        static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS),
                        this);
    lv_obj_add_event_cb(dropdown, onDropdownEvent, LV_EVENT_ALL, this);
    lv_obj_add_event_cb(dropdown, syncControlFocusToRow, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(dropdown, syncControlFocusToRow, LV_EVENT_DEFOCUSED, nullptr);
    if (out_dropdown)
        *out_dropdown = dropdown;
    return row;
}

_lv_obj_t *QuickPingOverlay::createMessageRow(_lv_obj_t *parent)
{
    _lv_obj_t *row = ht_class_create(&ht_quick_ping_message_row_class, parent);
    if (!row)
        return nullptr;
    lv_obj_set_size(row, lv_pct(100), kDropdownRowHeight);
    lv_obj_set_layout(row, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_CLICKED, this);

    _lv_obj_t *caption = lv_obj_create(row);
    if (caption)
    {
        lv_obj_set_width(caption, LV_SIZE_CONTENT);
        lv_obj_set_height(caption, kDropdownRowHeight);
        lv_obj_set_style_max_width(caption, kIconDropdownControlX, LV_PART_MAIN);
        lv_obj_set_layout(caption, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(caption, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(caption, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(caption, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        createFieldIcon(caption, &quick_ping_message_img);
        _lv_obj_t *caption_label =
            ht_label_create(&ht_quick_ping_label_class, caption, "message:");
        if (caption_label)
        {
            lv_obj_set_width(caption_label, LV_SIZE_CONTENT);
            lv_obj_set_style_max_width(caption_label, kIconDropdownControlX - kIconBadgeSize,
                                       LV_PART_MAIN);
            lv_label_set_long_mode(caption_label, LV_LABEL_LONG_CLIP);
        }
    }

    _lv_obj_t *controls =
        create_object(&ht_quick_ping_message_controls_class, row);
    if (!controls)
        return row;
    lv_obj_set_size(controls, lv_pct(100), kDropdownRowHeight);
    lv_obj_set_pos(controls, 0, 0);
    lv_obj_set_layout(controls, 0);
    lv_obj_clear_flag(controls,
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_move_foreground(caption);

    _ta_message = ht_class_create(&ht_quick_ping_message_editor_class, controls);
    if (_ta_message)
    {
        lv_obj_add_flag(_ta_message, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_textarea_set_one_line(_ta_message, true);
        // set_one_line() changes the height to LV_SIZE_CONTENT, so restore the
        // intended fixed control height afterwards.
        lv_obj_set_size(_ta_message, kMessageInputWidth, kDropdownControlHeight);
        lv_obj_align(_ta_message, LV_ALIGN_TOP_LEFT, kIconDropdownControlX, kDropdownControlY);
        lv_obj_set_scroll_dir(_ta_message, LV_DIR_HOR);
        lv_obj_clear_flag(_ta_message, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
        lv_textarea_set_max_length(_ta_message, kMaxMessageLength);
        lv_textarea_set_text_buffer(_ta_message, _message_text, sizeof(_message_text));
        lv_textarea_set_cursor_click_pos(_ta_message, true);
        lv_textarea_set_placeholder_text(_ta_message, "");
        if (_lv_obj_t *text_label = lv_textarea_get_label(_ta_message))
        {
            lv_obj_clear_flag(text_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
        }
        lv_obj_add_event_cb(_ta_message, onMessageInputEvent,
                            static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
        lv_obj_add_event_cb(_ta_message, onMessageInputEvent, LV_EVENT_CLICKED, this);
        lv_obj_add_event_cb(_ta_message, syncControlFocusToRow, LV_EVENT_FOCUSED, nullptr);
        lv_obj_add_event_cb(_ta_message, syncControlFocusToRow, LV_EVENT_DEFOCUSED, nullptr);
    }

    _dd_message = ht_class_create(&ht_quick_ping_message_dropdown_class, controls);
    if (_dd_message)
    {
        lv_obj_add_flag(_dd_message, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_set_size(_dd_message, kMessageDropdownWidth, kDropdownControlHeight);
        lv_obj_align(_dd_message, LV_ALIGN_TOP_LEFT, kIconDropdownControlX + kMessageInputWidth,
                     kDropdownControlY);
        lv_dropdown_set_options_static(_dd_message, "-");
        lv_dropdown_set_text(_dd_message, LV_SYMBOL_DOWN);
        lv_dropdown_set_symbol(_dd_message, nullptr);
        lv_dropdown_set_dir(_dd_message, LV_DIR_BOTTOM);
        lv_obj_add_event_cb(_dd_message, onDropdownConfirmPreprocess,
                            static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
        lv_obj_add_event_cb(_dd_message, onDropdownConfirmPreprocess,
                            static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS),
                            this);
        lv_obj_add_event_cb(_dd_message, onDropdownEvent, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(_dd_message, syncControlFocusToRow, LV_EVENT_FOCUSED, nullptr);
        lv_obj_add_event_cb(_dd_message, syncControlFocusToRow, LV_EVENT_DEFOCUSED, nullptr);
    }
    return row;
}

_lv_obj_t *QuickPingOverlay::createKeyboard(_lv_obj_t *parent)
{
    _lv_obj_t *keyboard = ht_class_create(&ht_quick_ping_keyboard_class, parent);
    if (!keyboard)
        return nullptr;
    lv_obj_set_size(keyboard, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(keyboard, 0, 0);
    lv_obj_set_flex_grow(keyboard, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_FLOATING);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_2, kMessageCompactKbMap,
                        kMessageCompactKbCtrl);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_3, kMessageCompactUpperKbMap,
                        kMessageCompactUpperKbCtrl);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_4, kMessageCompactSymbolKbMap,
                        kMessageCompactSymbolKbCtrl);

    _keyboard_editor = create_object(&ht_quick_ping_keyboard_editor_class, keyboard);
    if (!_keyboard_editor)
    {
        lv_obj_del(keyboard);
        return nullptr;
    }
    lv_textarea_set_one_line(_keyboard_editor, true);
    lv_obj_set_size(_keyboard_editor, lv_pct(100), kKeyboardEditorHeight);
    const lv_coord_t keyboard_top_pad = lv_obj_get_style_pad_top(keyboard, LV_PART_MAIN);
    lv_obj_set_pos(_keyboard_editor, 0, kKeyboardEditorInset - keyboard_top_pad);
    lv_obj_add_flag(_keyboard_editor, LV_OBJ_FLAG_FLOATING);
    lv_textarea_set_max_length(_keyboard_editor, kMaxMessageLength);
    lv_textarea_set_text_buffer(_keyboard_editor, _message_text, sizeof(_message_text));
    lv_textarea_set_cursor_click_pos(_keyboard_editor, true);
    lv_textarea_set_placeholder_text(_keyboard_editor, "Message");
    lv_textarea_set_cursor_pos(_keyboard_editor, LV_TEXTAREA_CURSOR_LAST);
    lv_obj_add_event_cb(_keyboard_editor, onKeyboardEditorEvent, LV_EVENT_VALUE_CHANGED, this);

    _keyboard_counter = create_object(&ht_quick_ping_keyboard_counter_class,
                                              _keyboard_editor);
    if (!_keyboard_counter)
    {
        lv_obj_del(keyboard);
        _keyboard_editor = nullptr;
        return nullptr;
    }
    lv_obj_set_size(_keyboard_counter, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(_keyboard_counter, LV_ALIGN_RIGHT_MID, 42, 0);
    lv_obj_add_flag(_keyboard_counter, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(_keyboard_counter, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    updateKeyboardCounter();

    lv_keyboard_set_textarea(keyboard, _keyboard_editor);
    lv_keyboard_set_popovers(keyboard, false);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_2);
    lv_obj_add_event_cb(keyboard, onKeyboardEvent, LV_EVENT_READY, this);
    lv_obj_add_event_cb(keyboard, onKeyboardEvent, LV_EVENT_CANCEL, this);
    lv_obj_add_event_cb(keyboard, onKeyboardValuePre,
                        static_cast<lv_event_code_t>(LV_EVENT_VALUE_CHANGED | LV_EVENT_PREPROCESS),
                        this);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    return keyboard;
}

void QuickPingOverlay::applyState(bool keep_focus)
{
    syncRecipientDropdown();

    const TargetKind kind = targetKind();
    setRowVisible(_row_recipient, kind == TargetKind::Group || kind == TargetKind::Personal);
    const bool message_ready = currentTargetReady();
    if (!message_ready)
        setKeyboardVisible(false);
    setDropdownEnabled(_ta_message, message_ready);
    setDropdownEnabled(_dd_message, message_ready);

    _lv_obj_t *preferred = keep_focus ? _gridNav.focused() : _dd_target;
    restoreModeFocus(preferred);
}

void QuickPingOverlay::restoreModeFocus(_lv_obj_t *preferred)
{
    _lv_obj_t *fallback = nullptr;
    _lv_obj_t *const candidates[] = { _dd_target, _dd_recipient, _ta_message, _dd_message, _keyboard };
    for (_lv_obj_t *candidate : candidates)
    {
        if (focus_object_available(candidate))
        {
            fallback = candidate;
            break;
        }
    }
    _rebuilding_navigation = true;
    lv_obj_t *const target = focus_object_available(preferred) ? preferred : fallback;
    if (target && _gridNav.contains(target))
        (void)_gridNav.focus(target);
    _rebuilding_navigation = false;
}

void QuickPingOverlay::syncLists()
{
    const int old_channel = currentGroupChannel();
    uint8_t old_key[6] = {};
    const uint8_t *key = currentContactKey();
    const bool had_key = key != nullptr;
    if (had_key)
        memcpy(old_key, key, sizeof(old_key));

    _group_count = 0;
    const int group_total = _biz.sendMessageGroupCount();
    for (int i = 0; i < group_total && _group_count < kMaxGroups; ++i)
    {
        CachedGroup &g = _groups[_group_count];
        int channel_idx = -1;
        if (!_biz.sendMessageGroupAt(i, &channel_idx, nullptr, 0))
            continue;
        g.channel_idx = channel_idx;
        g.source_index = static_cast<uint8_t>(i);
        ++_group_count;
    }

    _contact_total = _biz.sendMessagePersonalCount();

    int selected_global = _contact_window_start + _contact_index;
    if (selected_global < 0)
        selected_global = 0;
    if (selected_global >= _contact_total)
        selected_global = _contact_total - 1;
    if (had_key)
    {
        for (int i = 0; i < _contact_total; ++i)
        {
            uint8_t candidate[6]{};
            if (!_biz.sendMessagePersonalAt(i, candidate, nullptr, 0))
                continue;
            if (sameKey(candidate, old_key))
            {
                selected_global = i;
                break;
            }
        }
    }

    const int max_start = _contact_total > kMaxContacts ? _contact_total - kMaxContacts : 0;
    int start = _contact_window_start;
    if (start > max_start)
        start = max_start;
    if (selected_global < start || selected_global >= start + kMaxContacts)
    {
        start = (selected_global / kContactWindowStep) * kContactWindowStep;
        if (start > max_start)
            start = max_start;
    }
    loadContactWindow(start);

    int group_match = -1;
    if (old_channel >= 0)
    {
        for (int i = 0; i < _group_count; ++i)
        {
            if (_groups[i].channel_idx == old_channel)
            {
                group_match = i;
                break;
            }
        }
    }
    if (group_match >= 0)
    {
        _group_index = static_cast<uint8_t>(group_match);
    }
    else if (_group_count <= 0)
    {
        _group_index = 0;
    }
    else if (_group_index >= _group_count)
    {
        _group_index = static_cast<uint8_t>(_group_count - 1);
    }

    int contact_match = -1;
    if (had_key)
    {
        for (int i = 0; i < _contact_count; ++i)
        {
            if (sameKey(_contacts[i].pub_key_prefix, old_key))
            {
                contact_match = i;
                break;
            }
        }
    }
    if (contact_match >= 0)
    {
        _contact_index = contact_match;
    }
    else if (_contact_count <= 0)
    {
        _contact_index = 0;
    }
    else
    {
        _contact_index = selected_global - _contact_window_start;
        if (_contact_index < 0)
            _contact_index = 0;
        if (_contact_index >= _contact_count)
            _contact_index = _contact_count - 1;
    }
}

void QuickPingOverlay::applyPendingTarget()
{
    if (!_pending_target_valid)
        return;
    _pending_target_valid = false;

    if (!_pending_target_direct)
    {
        _target_index = static_cast<uint8_t>(TargetKind::Group);
        for (int index = 0; index < _group_count; ++index)
        {
            if (_groups[index].channel_idx == _pending_target_channel)
            {
                _group_index = static_cast<uint8_t>(index);
                return;
            }
        }
        return;
    }

    _target_index = static_cast<uint8_t>(TargetKind::Personal);
    for (int global_index = 0; global_index < _contact_total; ++global_index)
    {
        uint8_t candidate[sizeof(_pending_target_key)] = {};
        if (!_biz.sendMessagePersonalAt(global_index, candidate, nullptr, 0) ||
            !sameKey(candidate, _pending_target_key))
        {
            continue;
        }
        const int start = (global_index / kContactWindowStep) * kContactWindowStep;
        loadContactWindow(start);
        _contact_index = global_index - _contact_window_start;
        return;
    }
}

void QuickPingOverlay::loadContactWindow(int start)
{
    const int max_start = _contact_total > kMaxContacts ? _contact_total - kMaxContacts : 0;
    if (start < 0)
        start = 0;
    if (start > max_start)
        start = max_start;
    _contact_window_start = start;
    _contact_count = 0;
    for (int i = 0; i < kMaxContacts; ++i)
        _contacts[i] = CachedContact{};
    for (int i = start; i < _contact_total && _contact_count < kMaxContacts; ++i)
    {
        CachedContact &c = _contacts[_contact_count];
        if (!_biz.sendMessagePersonalAt(i, c.pub_key_prefix, nullptr, 0))
            continue;
        ++_contact_count;
    }
}

void QuickPingOverlay::recenterContactWindow(int global_index, int direction)
{
    if (_contact_total <= 0)
        return;
    const int max_start = _contact_total > kMaxContacts ? _contact_total - kMaxContacts : 0;
    int start = _contact_window_start;
    int local = global_index - start;
    if (direction > 0 && local >= kMaxContacts - 2 && start < max_start)
    {
        start += kContactWindowStep;
        if (start > max_start)
            start = max_start;
    }
    else if (direction < 0 && local <= 1 && start > 0)
    {
        start -= kContactWindowStep;
        if (start < 0)
            start = 0;
    }
    if (start != _contact_window_start)
        loadContactWindow(start);
    _contact_index = global_index - _contact_window_start;
    if (_contact_index < 0)
        _contact_index = 0;
    if (_contact_index >= _contact_count)
        _contact_index = _contact_count - 1;
}

void QuickPingOverlay::moveContactSelection(int delta)
{
    if (_contact_total <= 0 || delta == 0)
        return;
    const int old_start = _contact_window_start;
    int global = _contact_window_start + _contact_index;
    global = (global + (delta > 0 ? 1 : -1) + _contact_total) % _contact_total;
    if (global == 0 && delta > 0)
    {
        loadContactWindow(0);
    }
    else if (global == _contact_total - 1 && delta < 0)
    {
        loadContactWindow(_contact_total - kMaxContacts);
    }
    recenterContactWindow(global, delta);
    if (_contact_window_start != old_start)
    {
        syncRecipientDropdown();
        dropdown_viewport_realign(_dd_recipient, _root);
    }
    else if (_dd_recipient)
    {
        const bool was_syncing = _syncing_dropdowns;
        _syncing_dropdowns = true;
        lv_dropdown_set_selected(_dd_recipient, static_cast<uint16_t>(_contact_index));
        _syncing_dropdowns = was_syncing;
    }
}

void QuickPingOverlay::syncDropdownOptions()
{
    const bool was_syncing = _syncing_dropdowns;
    _syncing_dropdowns = true;
    if (_dd_target)
    {
        lv_dropdown_set_options_static(_dd_target, kTargetOptions);
        lv_dropdown_set_selected(_dd_target, dropdownIndexForTargetKind(targetKind()));
    }
    if (_dd_message)
    {
        lv_dropdown_set_options_static(_dd_message, _message_preset_options);
        lv_dropdown_set_selected(_dd_message, _message_index);
    }
    _syncing_dropdowns = was_syncing;
}

void QuickPingOverlay::rebuildMessagePresetOptions()
{
    _message_preset_options[0] = '\0';
    const int count = _biz.messagePresetCount();
    for (int index = 0; index < count && index < kMessagePresetCount; ++index)
    {
        (void)appendOption(_message_preset_options, sizeof(_message_preset_options),
                           _biz.messagePresetAt(index));
    }
}

void QuickPingOverlay::syncRecipientDropdown()
{
    if (!_dd_recipient)
        return;

    _recipient_options[0] = '\0';
    const TargetKind kind = targetKind();
    bool has_options = false;

    if (kind == TargetKind::Group)
    {
        if (_group_count <= 0)
        {
            appendOption(_recipient_options, sizeof(_recipient_options), "no groups");
        }
        else
        {
            char label[32] = {};
            for (int i = 0; i < _group_count; ++i)
            {
                int channel_idx = -1;
                if (!_biz.sendMessageGroupAt(_groups[i].source_index, &channel_idx,
                                             label, sizeof(label)))
                    continue;
                sanitizeOptionLabel(label, sizeof(label));
                appendOption(_recipient_options, sizeof(_recipient_options), label);
            }
            has_options = true;
        }
    }
    else if (kind == TargetKind::Personal)
    {
        if (_contact_count <= 0)
        {
            appendOption(_recipient_options, sizeof(_recipient_options), "no contacts");
        }
        else
        {
            char label[32] = {};
            for (int i = 0; i < _contact_count; ++i)
            {
                uint8_t key[6] = {};
                if (!_biz.sendMessagePersonalAt(_contact_window_start + i, key,
                                                label, sizeof(label)))
                    continue;
                sanitizeOptionLabel(label, sizeof(label));
                appendOption(_recipient_options, sizeof(_recipient_options), label);
            }
            has_options = true;
        }
    }
    else
    {
        appendOption(_recipient_options, sizeof(_recipient_options), "-");
    }

    const bool was_syncing = _syncing_dropdowns;
    _syncing_dropdowns = true;
    lv_dropdown_set_options_static(_dd_recipient, _recipient_options);
    if (kind == TargetKind::Group)
    {
        lv_dropdown_set_selected(_dd_recipient, _group_count > 0 ? _group_index : 0);
    }
    else if (kind == TargetKind::Personal)
    {
        lv_dropdown_set_selected(_dd_recipient, _contact_count > 0 ? _contact_index : 0);
    }
    else
    {
        lv_dropdown_set_selected(_dd_recipient, 0);
    }
    setDropdownEnabled(_dd_recipient, has_options);
    _syncing_dropdowns = was_syncing;
}

void QuickPingOverlay::setRowVisible(_lv_obj_t *row, bool visible)
{
    if (!row)
        return;
    if (visible)
    {
        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (row == _row_recipient)
            closeDropdowns(); // Popup lists are outside the hidden row's object tree.
        lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    }
}

void QuickPingOverlay::setDropdownEnabled(_lv_obj_t *dropdown, bool enabled)
{
    if (!dropdown)
        return;
    if (enabled)
    {
        lv_obj_clear_state(dropdown, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(dropdown, LV_STATE_DISABLED);
    }
}

void QuickPingOverlay::closeDropdowns()
{
    closeDropdown(_dd_target);
    closeDropdown(_dd_recipient);
    closeDropdown(_dd_message);
}

bool QuickPingOverlay::closeDropdown(_lv_obj_t *dropdown)
{
    if (!dropdown || !lv_obj_is_valid(dropdown))
        return false;
    if (!lv_dropdown_is_open(dropdown))
        return false;
    if (_repeat_dropdown == dropdown)
        clearRepeatSelection();
    lv_dropdown_close(dropdown);
    return true;
}

void QuickPingOverlay::updateMessageTextPresentation(bool editing)
{
    if (!_ta_message || !lv_obj_is_valid(_ta_message))
        return;
    _lv_obj_t *const text_label = lv_textarea_get_label(_ta_message);
    if (!text_label || !lv_obj_is_valid(text_label))
        return;

    // Let lv_textarea manage the label scroll position while editing.  Re-aligning
    // the internal label here would reset the horizontal cursor scroll.
    if (editing)
    {
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(text_label, LV_SIZE_CONTENT);
        lv_obj_add_state(text_label, LV_STATE_USER_1);
    }
    else
    {
        lv_obj_clear_state(text_label, LV_STATE_USER_1);
        lv_obj_set_width(text_label, lv_pct(100));
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_CLIP);
    }

    lv_obj_update_layout(_ta_message);
    if (!editing)
    {
        lv_obj_scroll_to(_ta_message, 0, 0, LV_ANIM_OFF);
    }
}

void QuickPingOverlay::updateKeyboardCounter()
{
    if (!_keyboard_counter || !lv_obj_is_valid(_keyboard_counter))
        return;
    const char *text = _keyboard_editor && lv_obj_is_valid(_keyboard_editor)
                           ? lv_textarea_get_text(_keyboard_editor)
                           : "";
    const size_t length = text ? strlen(text) : 0;
    const unsigned limit = _preset_edit_active ? kMaxPresetLength : kMaxMessageLength;
    lv_snprintf(_keyboard_count_text, sizeof(_keyboard_count_text), "%u/%u",
                static_cast<unsigned>(length), limit);
    lv_label_set_text_static(_keyboard_counter, _keyboard_count_text);
}

void QuickPingOverlay::syncKeyboardEditorToMessage()
{
    if (!_keyboard_editor || !lv_obj_is_valid(_keyboard_editor) || !_ta_message ||
        !lv_obj_is_valid(_ta_message))
    {
        return;
    }
    lv_textarea_set_text(_ta_message, lv_textarea_get_text(_keyboard_editor));
    updateMessageTextPresentation(false);
}

void QuickPingOverlay::setKeyboardVisible(bool visible)
{
    if (!_keyboard || !lv_obj_is_valid(_keyboard))
        return;
    const bool was_visible = keyboardVisible();
    if (visible)
    {
        if (!was_visible)
        {
            _keyboard_return_focus = _gridNav.focused();
        }
        if (_title_bar && lv_obj_is_valid(_title_bar))
        {
            lv_obj_add_flag(_title_bar, LV_OBJ_FLAG_HIDDEN);
        }
        if (_root)
            lv_obj_update_layout(_root);
        if (_keyboard_editor && lv_obj_is_valid(_keyboard_editor))
        {
            lv_textarea_set_max_length(_keyboard_editor,
                                       _preset_edit_active ? kMaxPresetLength : kMaxMessageLength);
            lv_textarea_set_text(_keyboard_editor,
                                 _ta_message ? lv_textarea_get_text(_ta_message) : "");
            lv_textarea_set_cursor_pos(_keyboard_editor, LV_TEXTAREA_CURSOR_LAST);
            updateKeyboardCounter();
            lv_obj_add_state(_keyboard_editor, LV_STATE_FOCUSED);
            lv_keyboard_set_textarea(_keyboard, _keyboard_editor);
        }
        lv_keyboard_set_mode(_keyboard, LV_KEYBOARD_MODE_USER_2);
        lv_obj_set_size(_keyboard, lv_pct(100), lv_pct(100));
        lv_obj_set_pos(_keyboard, 0, 0);
        lv_obj_clear_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(_keyboard);
        _gridNav.setCaptureMode(HtGridNav::CaptureMode::Persistent);
        _gridNav.setHorizontalNavigation(true);
        if (_gridNav.focus(_keyboard))
        {
            _gridNav.setEditing(true);
        }
    }
    else
    {
        _gridNav.setEditing(false);
        _gridNav.setHorizontalNavigation(false);
        _gridNav.setCaptureMode(HtGridNav::CaptureMode::Transient);
        if (was_visible)
            syncKeyboardEditorToMessage();
        lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
        if (_keyboard_editor && lv_obj_is_valid(_keyboard_editor))
        {
            lv_obj_clear_state(_keyboard_editor, LV_STATE_FOCUSED);
            lv_textarea_set_max_length(_keyboard_editor, kMaxMessageLength);
        }
        if (_title_bar && lv_obj_is_valid(_title_bar))
        {
            lv_obj_clear_flag(_title_bar, LV_OBJ_FLAG_HIDDEN);
        }
        _lv_obj_t *const return_focus = _keyboard_return_focus;
        _keyboard_return_focus = nullptr;
        if (focus_object_available(return_focus) && _gridNav.contains(return_focus))
        {
            (void)_gridNav.focus(return_focus);
        }
    }
    if (was_visible != visible)
        updateMessageTextPresentation(false);
    if (was_visible != visible && _root)
        lv_obj_update_layout(_root);
}

bool QuickPingOverlay::keyboardVisible() const
{
    return _keyboard && lv_obj_is_valid(_keyboard) &&
           !lv_obj_has_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
}

bool QuickPingOverlay::acceptsKeyboardCycle() const
{
    return keyboardVisible();
}

bool QuickPingOverlay::hitVisibleKeyboard(int16_t x, int16_t y) const
{
    if (!keyboardVisible())
        return false;
    lv_point_t point;
    point.x = static_cast<lv_coord_t>(x);
    point.y = static_cast<lv_coord_t>(y);
    return lv_obj_hit_test(_keyboard, &point);
}

bool QuickPingOverlay::hitVerticalSwipeControl(int16_t x, int16_t y) const
{
    lv_point_t point;
    point.x = static_cast<lv_coord_t>(x);
    point.y = static_cast<lv_coord_t>(y);
    if (hitVisibleKeyboard(x, y))
        return true;

    _lv_obj_t *const protected_controls[] = {
        _dd_target,
        _dd_recipient,
        _dd_message,
        _ta_message,
    };
    for (_lv_obj_t *control : protected_controls)
    {
        if (control && lv_obj_is_valid(control) && !lv_obj_has_flag(control, LV_OBJ_FLAG_HIDDEN) &&
            lv_obj_hit_test(control, &point))
        {
            return true;
        }
    }

    _lv_obj_t *const dropdowns[] = { _dd_target, _dd_recipient, _dd_message };
    for (_lv_obj_t *dropdown : dropdowns)
    {
        _lv_obj_t *list = dropdown ? lv_dropdown_get_list(dropdown) : nullptr;
        if (list && lv_obj_is_valid(list) && !lv_obj_has_flag(list, LV_OBJ_FLAG_HIDDEN) &&
            lv_obj_hit_test(list, &point))
        {
            return true;
        }
    }
    return false;
}

bool QuickPingOverlay::hitSwipeDismissRegion(int16_t x, int16_t y) const
{
    if (!_root || !lv_obj_is_valid(_root) || lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN))
    {
        return false;
    }

    lv_point_t point;
    point.x = static_cast<lv_coord_t>(x);
    point.y = static_cast<lv_coord_t>(y);
    if (!lv_obj_hit_test(_root, &point) || hitVerticalSwipeControl(x, y))
        return false;
    return true;
}

bool QuickPingOverlay::dismissTransientControls()
{
    if (closeDropdown(_dd_target) || closeDropdown(_dd_recipient) || closeDropdown(_dd_message))
    {
        return true;
    }
    if (!keyboardVisible())
        return false;
    setKeyboardVisible(false);
    restoreModeFocus(_ta_message);
    return true;
}

bool QuickPingOverlay::targetInside(_lv_obj_t *target, _lv_obj_t *ancestor) const
{
    if (!target || !ancestor || !lv_obj_is_valid(target) || !lv_obj_is_valid(ancestor))
        return false;
    for (_lv_obj_t *obj = target; obj; obj = lv_obj_get_parent(obj))
    {
        if (obj == ancestor)
            return true;
    }
    return false;
}

void QuickPingOverlay::closeKeyboardForOutsideTarget(_lv_obj_t *target)
{
    if (!keyboardVisible())
        return;
    if (targetInside(target, _keyboard) || targetInside(target, _ta_message))
        return;
    setKeyboardVisible(false);
    restoreModeFocus(focus_object_available(target) ? target : nullptr);
}

void QuickPingOverlay::handleDropdownRow(_lv_obj_t *target)
{
    _lv_obj_t *dropdown = nullptr;
    if (targetInside(target, _row_target))
    {
        dropdown = _dd_target;
    }
    else if (targetInside(target, _row_recipient))
    {
        dropdown = _dd_recipient;
    }
    else if (targetInside(target, _row_message))
    {
        lv_indev_t *const indev = lv_indev_get_act();
        lv_point_t point{};
        if (indev)
        {
            lv_indev_get_point(indev, &point);
        }
        if (indev && _dd_message && lv_obj_hit_test(_dd_message, &point))
        {
            dropdown = _dd_message;
        }
        else
        {
            handleMessageInput();
            return;
        }
    }
    if (!focus_object_available(dropdown))
        return;

    // A click originating from the dropdown button is already toggled by the
    // LVGL dropdown class.  Do not reopen it when that click bubbles to the row.
    if (targetInside(target, dropdown))
        return;

    closeKeyboardForOutsideTarget(dropdown);
    restoreModeFocus(dropdown);
    if (lv_dropdown_is_open(dropdown))
    {
        closeDropdown(dropdown);
    }
    else
    {
        lv_dropdown_open(dropdown);
    }
}

void QuickPingOverlay::handleTargetChanged()
{
    if (!_dd_target)
        return;
    _target_index = static_cast<uint8_t>(lv_dropdown_get_selected(_dd_target));
    const TargetKind kind = targetKind();

    if (kind == TargetKind::Advert)
    {
        (void)_biz.sendAdvert();
    }
    else if (kind == TargetKind::Group && _group_count <= 0)
    {
        showAlert("No group channels", 900);
    }
    else if (kind == TargetKind::Personal && _contact_count <= 0)
    {
        showAlert("No contacts", 900);
    }

    applyState(true);
}

void QuickPingOverlay::handleRecipientChanged()
{
    if (!_dd_recipient)
        return;
    const uint16_t selected = lv_dropdown_get_selected(_dd_recipient);
    if (targetKind() == TargetKind::Group)
    {
        _group_index =
            static_cast<uint8_t>(_group_count > 0 && selected < _group_count ? selected : 0);
    }
    else if (targetKind() == TargetKind::Personal)
    {
        _contact_index = _contact_count > 0 && selected < _contact_count ? selected : 0;
    }
    closeDropdown(_dd_recipient);
    restoreModeFocus(_dd_recipient);
}

void QuickPingOverlay::handleMessageSelectionChanged()
{
    if (!_dd_message)
        return;
    closeDropdown(_dd_message);
    _message_index = static_cast<uint8_t>(lv_dropdown_get_selected(_dd_message));

    if (_message_index < kMessagePresetCount)
    {
        if (!currentTargetReady())
        {
            showMissingTargetAlert();
            applyState(true);
            return;
        }
        if (_ta_message)
            lv_textarea_set_text(_ta_message, _biz.messagePresetAt(_message_index));
        updateMessageTextPresentation(false);
        sendMessageText(_biz.messagePresetAt(_message_index));
    }
    applyState(true);
}

void QuickPingOverlay::handleMessageInput()
{
    if (!currentTargetReady())
    {
        showMissingTargetAlert();
        return;
    }
    if (keyboardVisible() || _keyboard_open_pending)
        return;
    _keyboard_open_pending = true;
    if (lv_async_call(openPendingKeyboardAsync, this) != LV_RES_OK)
        _keyboard_open_pending = false;
}

void QuickPingOverlay::openPendingKeyboard()
{
    if (!_keyboard_open_pending)
        return;
    _keyboard_open_pending = false;
    if (!focus_object_available(_ta_message) || !currentTargetReady())
        return;
    // A newly opened keyboard starts a new submission cycle.
    _message_submit_handled = false;
    setKeyboardVisible(true);
    restoreModeFocus(_keyboard);
}

void QuickPingOverlay::submitMessageFromTextarea()
{
    if (_message_submit_handled)
        return;
    if (_preset_edit_active)
    {
        const char *const text = _keyboard_editor ? lv_textarea_get_text(_keyboard_editor) : nullptr;
        const size_t length = text ? strlen(text) : 0;
        if (length == 0)
        {
            showAlert("Preset cannot be empty", 1200);
            return;
        }
        if (length > kMaxPresetLength)
        {
            showAlert("Preset too long", 1200);
            return;
        }
        using Result = Biz::IBizFacade::PresetUpdateResult;
        const Result result = _biz.updateMessagePreset(_preset_edit_index, text);
        if (result != Result::Updated && result != Result::Unchanged)
        {
            showAlert(result == Result::InvalidText ? "Invalid preset text" :
                                                             "Invalid preset",
                           1200);
            return;
        }
        if (!_biz.commitMessagePresets())
        {
            showAlert("Preset save failed", 1600);
            return;
        }
        _message_submit_handled = true;
        _preset_edit_active = false;
        showAlert(result == Result::Updated ? "Preset saved" : "Preset unchanged", 1000);
        (void)notifyApp(UiMessageType::QuickPingClose);
        return;
    }
    syncKeyboardEditorToMessage();
    if (!_ta_message)
    {
        setKeyboardVisible(false);
        restoreModeFocus(nullptr);
        return;
    }
    if (!currentTargetReady())
    {
        _message_submit_handled = true;
        showMissingTargetAlert();
        setKeyboardVisible(false);
        restoreModeFocus(_ta_message);
        return;
    }
    const char *text = lv_textarea_get_text(_ta_message);
    // Latch before calling into the business layer. This protects against a
    // nested READY event generated while the send result is being displayed.
    _message_submit_handled = true;
    if (!sendMessageText(text))
    {
        // Keep the keyboard usable after a validation or transport failure.
        _message_submit_handled = false;
        return;
    }
    setKeyboardVisible(false);
    restoreModeFocus(_ta_message);
}

bool QuickPingOverlay::sendMessageText(const char *text)
{
    if (text && strlen(text) > kMaxMessageLength)
    {
        showAlert("Message too long", 900);
        return false;
    }
    char normalized[kMaxMessageLength + 1] = {};
    if (normalizeSingleLineText(text, normalized, sizeof(normalized)) == 0)
    {
        showAlert("Enter message", 900);
        return false;
    }

    const TargetKind kind = targetKind();
    if (kind == TargetKind::Personal)
    {
        const uint8_t *key = currentContactKey();
        if (!key)
        {
            showAlert("No contacts", 900);
            return false;
        }
        return _biz.sendDirectMessage(key, normalized);
    }
    else if (kind == TargetKind::Group)
    {
        const int channel = currentGroupChannel();
        if (channel < 0)
        {
            showAlert("No group channels", 900);
            return false;
        }
        return _biz.sendGroupMessage(channel, normalized);
    }
    else if (kind == TargetKind::Broadcast)
    {
        const int len = static_cast<int>(strlen(normalized));
        const bool ok = _biz.sendBroadcast(normalized, len);
        showAlert(ok ? "Queued" : "Failed", 900);
        return ok;
    }
    showMissingTargetAlert();
    return false;
}

void QuickPingOverlay::activateDropdown(_lv_obj_t *dropdown)
{
    if (!dropdown || _syncing_dropdowns)
        return;
    if (dropdown == _dd_target)
    {
        handleTargetChanged();
    }
    else if (dropdown == _dd_recipient)
    {
        handleRecipientChanged();
    }
    else if (dropdown == _dd_message)
    {
        handleMessageSelectionChanged();
    }
}

bool QuickPingOverlay::isEditableDropdown(_lv_obj_t *object) const
{
    return object == _dd_target || object == _dd_recipient || object == _dd_message;
}

void QuickPingOverlay::beginDropdownEdit(_lv_obj_t *dropdown)
{
    if (!isEditableDropdown(dropdown))
        return;
    if (_edit_state == EditState::Edit && _editing_dropdown == dropdown)
        return;
    if (_edit_state == EditState::Edit)
        cancelDropdownEdit(_editing_dropdown);
    _editing_dropdown = dropdown;
    _editing_original_index = lv_dropdown_get_selected(dropdown);
    _edit_state = EditState::Edit;
    lv_obj_add_state(dropdown, LV_STATE_EDITED);
    _gridNav.setEditing(true);
}

void QuickPingOverlay::commitDropdownEdit(_lv_obj_t *dropdown)
{
    if (!isEditableDropdown(dropdown))
        return;
    lv_dropdown_close(dropdown);
    lv_obj_clear_state(dropdown, LV_STATE_EDITED);
    _editing_dropdown = nullptr;
    _edit_state = EditState::Browse;
    _gridNav.setEditing(false);
    activateDropdown(dropdown);
}

void QuickPingOverlay::cancelDropdownEdit(_lv_obj_t *dropdown)
{
    if (!isEditableDropdown(dropdown))
        return;
    _syncing_dropdowns = true;
    lv_dropdown_set_selected(dropdown, _editing_original_index);
    _syncing_dropdowns = false;
    lv_dropdown_close(dropdown);
    lv_obj_clear_state(dropdown, LV_STATE_EDITED);
    _editing_dropdown = nullptr;
    _edit_state = EditState::Browse;
    _gridNav.setEditing(false);
}

void QuickPingOverlay::clearRepeatSelection()
{
    (void)lv_async_call_cancel(finishRepeatSelectionAsync, this);
    _repeat_pending = false;
    _repeat_index = 0;
    _repeat_dropdown = nullptr;
}

void QuickPingOverlay::armRepeatSelection(_lv_obj_t *dropdown)
{
    if (!dropdown || !lv_obj_is_valid(dropdown) || !lv_dropdown_is_open(dropdown))
        return;
    clearRepeatSelection();
    _repeat_pending = true;
    _repeat_index = lv_dropdown_get_selected(dropdown);
    _repeat_dropdown = dropdown;
    if (lv_async_call(finishRepeatSelectionAsync, this) != LV_RES_OK)
        clearRepeatSelection();
}

void QuickPingOverlay::finishRepeatSelection()
{
    if (!_repeat_pending)
        return;
    _lv_obj_t *dropdown = _repeat_dropdown;
    const uint16_t index = _repeat_index;
    _repeat_pending = false;
    _repeat_index = 0;
    _repeat_dropdown = nullptr;

    if (!dropdown || !lv_obj_is_valid(dropdown) || lv_dropdown_is_open(dropdown))
        return;
    if (lv_dropdown_get_selected(dropdown) != index)
        return;
    activateDropdown(dropdown);
}

bool QuickPingOverlay::currentTargetReady() const
{
    switch (targetKind())
    {
    case TargetKind::Broadcast:
        return true;
    case TargetKind::Group:
        return _group_count > 0 && _group_index < _group_count;
    case TargetKind::Personal:
        return _contact_count > 0 && _contact_index < _contact_count;
    case TargetKind::Advert:
        return false;
    }
    return false;
}

QuickPingOverlay::TargetKind QuickPingOverlay::targetKind() const
{
    return targetKindFromDropdownIndex(_target_index);
}

QuickPingOverlay::TargetKind QuickPingOverlay::targetKindFromDropdownIndex(uint16_t index)
{
    switch (index)
    {
    case 0:
        return TargetKind::Group;
    case 1:
        return TargetKind::Broadcast;
    case 2:
        return TargetKind::Personal;
    case 3:
        return TargetKind::Advert;
    default:
        return TargetKind::Broadcast;
    }
}

uint16_t QuickPingOverlay::dropdownIndexForTargetKind(TargetKind kind)
{
    return static_cast<uint16_t>(kind);
}

int QuickPingOverlay::currentGroupChannel() const
{
    if (_group_count <= 0 || _group_index >= _group_count)
        return -1;
    return _groups[_group_index].channel_idx;
}

const uint8_t *QuickPingOverlay::currentContactKey() const
{
    if (_contact_count <= 0 || _contact_index >= _contact_count)
        return nullptr;
    return _contacts[_contact_index].pub_key_prefix;
}

void QuickPingOverlay::showMissingTargetAlert() const
{
    switch (targetKind())
    {
    case TargetKind::Group:
        showAlert("No group channels", 900);
        break;
    case TargetKind::Personal:
        showAlert("No contacts", 900);
        break;
    case TargetKind::Advert:
        showAlert("Select message target", 900);
        break;
    case TargetKind::Broadcast:
        showAlert("Failed", 900);
        break;
    }
}

bool QuickPingOverlay::appendOption(char *buf, size_t buf_len, const char *option) const
{
    if (!buf || buf_len == 0 || !option)
        return false;
    const size_t used = strlen(buf);
    if (used >= buf_len - 1)
        return false;
    if (option[0] == '\0')
        option = "?";
    const char *sep = used > 0 ? "\n" : "";
    const int written = lv_snprintf(buf + used, buf_len - used, "%s%s", sep, option);
    return written > 0 && static_cast<size_t>(written) < buf_len - used;
}

void QuickPingOverlay::onDropdownEvent(lv_event_t *e)
{
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;

    const lv_event_code_t code = lv_event_get_code(e);
    _lv_obj_t *dropdown = lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED ||
        code == LV_EVENT_VALUE_CHANGED)
    {
        if (!self->_rebuilding_navigation)
            self->closeKeyboardForOutsideTarget(dropdown);
    }

    if (code == LV_EVENT_READY)
    {
        dropdown_viewport_realign(dropdown, self->_root);
        return;
    }
    if (code != LV_EVENT_VALUE_CHANGED)
        return;
    if (self->_syncing_dropdowns)
        return;
    self->clearRepeatSelection();
    self->activateDropdown(dropdown);

    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
}

void QuickPingOverlay::onDropdownConfirmPreprocess(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_KEY)
    {
        const uint32_t key = lv_event_get_key(e);
        auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
        if (!self)
            return;
        _lv_obj_t *dropdown = lv_event_get_target(e);

        if (key == LV_KEY_ENTER && self->isEditableDropdown(dropdown))
        {
            if (!lv_dropdown_is_open(dropdown))
            {
                self->beginDropdownEdit(dropdown);
                lv_dropdown_open(dropdown);
            }
            else if (self->_edit_state == EditState::Edit)
            {
                self->commitDropdownEdit(dropdown);
            }
            lv_event_stop_bubbling(e);
            lv_event_stop_processing(e);
            return;
        }

        if (dropdown == self->_dd_recipient && self->targetKind() == TargetKind::Personal &&
            lv_dropdown_is_open(dropdown))
        {
            int direction = 0;
            if (key == LV_KEY_NEXT || key == LV_KEY_RIGHT || key == LV_KEY_DOWN)
                direction = 1;
            if (key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP)
                direction = -1;
            if (direction != 0)
            {
                self->moveContactSelection(direction);
                lv_event_stop_bubbling(e);
                lv_event_stop_processing(e);
                return;
            }
        }

        // Handle ESC before the LVGL dropdown class closes the list. Otherwise
        // the bubbled ESC reaches the overlay after the list is already closed
        // and incorrectly closes the whole QuickPing pane as well.
        if (key == LV_KEY_ESC)
        {
            if (!dropdown || !lv_dropdown_is_open(dropdown))
                return;
            self->clearRepeatSelection();
            lv_dropdown_close(dropdown);
            lv_event_stop_bubbling(e);
            lv_event_stop_processing(e);
            return;
        }
        if (key != LV_KEY_ENTER)
            return;
        if (dropdown && lv_dropdown_is_open(dropdown))
            self->armRepeatSelection(dropdown);
        return;
    }
    else if (code == LV_EVENT_RELEASED)
    {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev || lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER)
            return;
    }
    else
    {
        return;
    }
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *dropdown = lv_event_get_target(e);
    if (dropdown && lv_dropdown_is_open(dropdown))
        self->armRepeatSelection(dropdown);
}

void QuickPingOverlay::finishRepeatSelectionAsync(void *user_data)
{
    auto *self = static_cast<QuickPingOverlay *>(user_data);
    if (self)
        self->finishRepeatSelection();
}

void QuickPingOverlay::openPendingKeyboardAsync(void *user_data)
{
    auto *self = static_cast<QuickPingOverlay *>(user_data);
    if (self)
        self->openPendingKeyboard();
}

void QuickPingOverlay::onMessageInputEvent(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;

    if (code == LV_EVENT_KEY)
    {
        const uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ENTER)
        {
            self->handleMessageInput();
        }
        else if (key == LV_KEY_ESC)
        {
            (void)self->onKey(key);
        }
        else
        {
            return;
        }
    }
    else if (code == LV_EVENT_CLICKED)
    {
        self->handleMessageInput();
    }
    else
    {
        return;
    }
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
}

void QuickPingOverlay::onKeyboardEditorEvent(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
        return;
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (self)
        self->updateKeyboardCounter();
}

void QuickPingOverlay::onKeyboardEvent(lv_event_t *e)
{
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY)
    {
        self->submitMessageFromTextarea();
    }
    else if (code == LV_EVENT_CANCEL)
    {
        if (self->_preset_edit_active)
        {
            self->_preset_edit_active = false;
            (void)self->notifyApp(UiMessageType::QuickPingClose);
        }
        else
        {
            self->setKeyboardVisible(false);
            self->restoreModeFocus(self->_ta_message);
        }
    }
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        lv_event_stop_bubbling(e);
        lv_event_stop_processing(e);
    }
}

void QuickPingOverlay::onKeyboardValuePre(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
        return;
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *keyboard = lv_event_get_target(e);
    if (!keyboard)
        return;
    const uint16_t btn_id = lv_btnmatrix_get_selected_btn(keyboard);
    if (btn_id == LV_BTNMATRIX_BTN_NONE)
        return;
    const char *text = lv_btnmatrix_get_btn_text(keyboard, btn_id);
    if (!text)
        return;

    if (strcmp(text, "ABC") == 0)
    {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_3);
        lv_event_stop_processing(e);
    }
    else if (strcmp(text, "abc") == 0)
    {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_2);
        lv_event_stop_processing(e);
    }
    else if (strcmp(text, "123") == 0)
    {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_4);
        lv_event_stop_processing(e);
    }
    else if (strcmp(text, "SP") == 0)
    {
        if (self->_keyboard_editor)
        {
            lv_textarea_add_char(self->_keyboard_editor, ' ');
        }
        lv_event_stop_processing(e);
    }
    else if (strcmp(text, LV_SYMBOL_CLOSE) == 0)
    {
        if (self->_preset_edit_active)
        {
            self->_preset_edit_active = false;
            (void)self->notifyApp(UiMessageType::QuickPingClose);
        }
        else
        {
            self->setKeyboardVisible(false);
            self->restoreModeFocus(self->_ta_message);
        }
        lv_event_stop_processing(e);
    }
    if (strcmp(text, "ABC") == 0 || strcmp(text, "abc") == 0 || strcmp(text, "123") == 0 ||
        strcmp(text, "SP") == 0 || strcmp(text, LV_SYMBOL_CLOSE) == 0)
    {
        lv_event_stop_bubbling(e);
    }
}

void QuickPingOverlay::onOutsideEvent(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED)
        return;
    auto *self = static_cast<QuickPingOverlay *>(lv_event_get_user_data(e));
    if (!self)
        return;
    _lv_obj_t *target = lv_event_get_target(e);

    // A background click only dismisses a transient dropdown/keyboard. The pane
    // itself is closed by its upward swipe gesture or by ESC/back.
    if (target == self->_root || target == self->_content)
    {
        (void)self->dismissTransientControls();
    }
    else
    {
        self->closeKeyboardForOutsideTarget(target);
        self->handleDropdownRow(target);
    }
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
}

} // namespace Heltec::MeshCore::Ui

#endif // HELTEC_UI_HAS_TOUCH
