#pragma once
namespace heltec::meshcore::ui {
class IUiHost {
  public:
    virtual ~IUiHost() = default;

    virtual bool isReady() const = 0;
    virtual void openPreviewOverlay(uint8_t unread, uint32_t received_ms, const char *origin, const char *text) = 0;
    virtual void closePreviewOverlay() = 0;
    virtual void openAlertOverlay(const char *text) = 0;
    virtual void closeAlertOverlay() = 0;
};

} // namespace heltec::meshcore::ui
