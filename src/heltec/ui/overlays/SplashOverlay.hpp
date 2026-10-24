#pragma once
#include <stdint.h>

struct _lv_obj_t;

namespace Heltec::MeshCore::Ui
{

class SplashOverlay
{
  public:
    SplashOverlay() = default;
    ~SplashOverlay();

    bool create(_lv_obj_t *parent);
    void hide();

  private:
    void printInfo(_lv_obj_t *version, _lv_obj_t *date);

    _lv_obj_t *_root = nullptr;
    char _version_text[32] = {};
};

} // namespace Heltec::MeshCore::Ui
