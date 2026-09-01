#pragma once
#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::ui
{

class SplashOverlay
{
  public:
    SplashOverlay() = default;
    ~SplashOverlay();

    bool create(_lv_obj_t *parent);
    _lv_obj_t *root() const
    {
        return _root;
    }

    void printInfo();

    void hide();
    bool isVisible() const
    {
        return _visible;
    }

  private:
    _lv_obj_t *_root = nullptr;
    _lv_obj_t *_logo = nullptr;
    _lv_obj_t *_ver = nullptr;
    _lv_obj_t *_date = nullptr;
    _lv_obj_t *_attribution = nullptr;
    char _version_text[32] = {};
    bool _visible = false;
};

} // namespace heltec::meshcore::ui
