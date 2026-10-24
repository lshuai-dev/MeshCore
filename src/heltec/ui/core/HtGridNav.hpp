#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

class HtGridNav
{
  public:
    using FocusChangedFn = void (*)(lv_obj_t *focused, void *user_data);
    enum class Mode : uint8_t
    {
        Browse,
        Controls,
    };

    enum class CaptureMode : uint8_t
    {
        Transient,
        Persistent,
    };

    HtGridNav()
        : _mode(Mode::Browse), _capture_mode(CaptureMode::Transient), _wrap(true),
          _frozen(false), _horizontal_navigation(false)
    {
    }

    void bindRoot(lv_obj_t *root);
    lv_obj_t *root() const
    {
        return _root;
    }
    void add(lv_obj_t *object);
    void remove(lv_obj_t *object);
    void clear();

    bool enterControls();
    void enterBrowse();
    void restoreFocus();
    bool handleKey(uint32_t key);

    bool focus(lv_obj_t *object);
    void focusNext();
    void focusPrevious();
    void setWrap(bool enabled);
    void setCaptureMode(CaptureMode mode);
    void setHorizontalNavigation(bool enabled);
    void setEditing(bool enabled);
    void freeze(bool enabled);
    void setFocusChangedCallback(FocusChangedFn callback, void *user_data);

    bool contains(const lv_obj_t *object) const;
    bool isEditing() const;
    bool isFrozen() const;
    uint16_t count() const;
    Mode mode() const;
    lv_obj_t *focused() const;

  private:
    static constexpr lv_obj_flag_t kNavigableFlag = LV_OBJ_FLAG_USER_1;

    static lv_group_t *nativeControlGroup();
    static bool available(const lv_obj_t *object);
    static lv_obj_tree_walk_res_t clearVisitor(lv_obj_t *object, void *user_data);
    static lv_obj_tree_walk_res_t countVisitor(lv_obj_t *object, void *user_data);
    static lv_obj_tree_walk_res_t firstVisitor(lv_obj_t *object, void *user_data);
    static lv_obj_tree_walk_res_t nextVisitor(lv_obj_t *object, void *user_data);
    static lv_obj_tree_walk_res_t previousVisitor(lv_obj_t *object, void *user_data);

    bool select(lv_obj_t *object);
    bool sendKey(uint32_t key);
    bool activateSelected();
    void releaseCapture();

    lv_obj_t *_root = nullptr;
    lv_obj_t *_selected = nullptr;
    lv_obj_t *_captured = nullptr;
    FocusChangedFn _focus_changed = nullptr;
    void *_focus_user_data = nullptr;
    Mode _mode : 1;
    CaptureMode _capture_mode : 1;
    bool _wrap : 1;
    bool _frozen : 1;
    bool _horizontal_navigation : 1;
};

} // namespace Heltec::MeshCore::Ui
