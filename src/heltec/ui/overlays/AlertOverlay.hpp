#pragma once
#include "../core/DialogOverlay.hpp"
struct _lv_obj_t;

namespace Heltec::MeshCore::Biz {
class IBizFacade;
}

namespace Heltec::MeshCore::Ui {

/** @brief 短时提示 overlay；由 UiApp::activate 驱动显隐。 */
class AlertOverlay : public DialogOverlay {
  public:
    explicit AlertOverlay(Biz::IBizFacade &biz)
        : DialogOverlay(biz, Options{ Variant::Alert, false, false, false }) {
    }

    /** text 由 UiTask 持有，Alert 显示期间必须保持有效。 */
    void setText(const char *text);

  protected:
    void onEnter() override;
    bool build() override;

  private:
    bool onKey(uint32_t key) override;

    _lv_obj_t *_label = nullptr;
    uint32_t _entered_ms = 0;
};

} // namespace Heltec::MeshCore::Ui
