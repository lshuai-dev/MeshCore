#pragma once
#include "../core/AbstractScreen.hpp"
#include "../core/AppStateEvent.hpp"
namespace Heltec::MeshCore::Biz {
class IBizFacade;
}

namespace Heltec::MeshCore::Ui {
class HomeScreen : public AbstractScreen {
  public:
    HomeScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
        : AbstractScreen(biz, title, icon) {
    }

  protected:
    bool buildSurface() override;

  private:
    void onAppStateChanged(const AppStateEvent &event) override;
    void onActivated() override;
    void refreshSnapshot();
    void refreshLabels();

    _lv_obj_t *_lblId = nullptr;
    _lv_obj_t *_lblMsg = nullptr;
    _lv_obj_t *_lblStatus = nullptr;
    int _message_count = 0;
    bool _companion_connected = false;
    uint32_t _pairing_pin = 0;
    char _id_line[16] = {};
    char _message_line[16] = {};
    char _status_line[32] = {};
};

} // namespace Heltec::MeshCore::Ui
