#include "HeltecMesh.h"
#include "MeshCore.h"
#include "app/MeshAppUi.hpp"
#include "app/PowerMgr.hpp"
#include "config/NodePrefs.h"
#include "heltec/drivers/display/DisplayPort.hpp"
#include "heltec/drivers/input/MomentaryButton.hpp"
#include "lvgl.h"
#include "ui/app/App.hpp"
#include "ui/core/AppStateNotifier.hpp"

#include <Arduino.h> // needed for PlatformIO
#include <Mesh.h>
#include <helpers/sensors/LocationProvider.h>
#if defined(RADIOCORE_BOARD_RC52)
#include <nrf_gpio.h>
#endif
#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
#include "config/HeltecLicense.h"
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include <heltec/sensors/IcmCompassProvider.h>
#endif

#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
static void logRc52SensorPowerState(const char *stage)
{
    const uint32_t pin = static_cast<uint32_t>(PIN_SENSOR_EN);
    MESH_DEBUG_PRINTLN("[boot][rc52] %s VSENSOR_CTRL pin=%lu dir=%lu out=%lu in=%lu",
                       stage, static_cast<unsigned long>(pin),
                       static_cast<unsigned long>(nrf_gpio_pin_dir_get(pin)),
                       static_cast<unsigned long>(nrf_gpio_pin_out_read(pin)),
                       static_cast<unsigned long>(nrf_gpio_pin_read(pin)));
}
#endif

static uint32_t _atoi(const char *sp)
{
    uint32_t n = 0;
    while (*sp && *sp >= '0' && *sp <= '9')
    {
        n *= 10;
        n += (*sp++ - '0');
    }
    return n;
}

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
#include <InternalFileSystem.h>
#if defined(QSPIFLASH)
#include <CustomLFS_QSPIFlash.h>
DataStore store(InternalFS, QSPIFlash, rtc_clock);
#else
#if defined(EXTRAFS)
#include <CustomLFS.h>
CustomLFS ExtraFS(0xD4000, 0x19000, 128);
DataStore store(InternalFS, ExtraFS, rtc_clock);
#else
DataStore store(InternalFS, rtc_clock);
#endif
#endif
#elif defined(RP2040_PLATFORM)
#include <LittleFS.h>
DataStore store(LittleFS, rtc_clock);
#elif defined(ESP32)
#include <SPIFFS.h>
DataStore store(SPIFFS, rtc_clock);
#endif

#ifdef ESP32
#ifdef WIFI_SSID
#include <helpers/esp32/SerialWifiInterface.h>
SerialWifiInterface serial_interface;
#ifndef TCP_PORT
#define TCP_PORT 5000
#endif
#elif defined(BLE_PIN_CODE)
#include <helpers/esp32/SerialBLEInterface.h>
SerialBLEInterface serial_interface;
#elif defined(SERIAL_RX)
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
HardwareSerial companion_serial(1);
#else
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#endif
#elif defined(RP2040_PLATFORM)
// #ifdef WIFI_SSID
//   #include <helpers/rp2040/SerialWifiInterface.h>
//   SerialWifiInterface serial_interface;
//   #ifndef TCP_PORT
//     #define TCP_PORT 5000
//   #endif
//  #elif defined(BLE_PIN_CODE)
//    #include <helpers/rp2040/SerialBLEInterface.h>
//    SerialBLEInterface serial_interface;
#if defined(SERIAL_RX)
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
HardwareSerial companion_serial(1);
#else
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#endif
#elif defined(NRF52_PLATFORM)
#ifdef BLE_PIN_CODE
#include <helpers/nrf52/SerialBLEInterface.h>
SerialBLEInterface serial_interface;
#else
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#endif
#elif defined(STM32_PLATFORM)
#include <helpers/ArduinoSerialInterface.h>
ArduinoSerialInterface serial_interface;
#else
#error "need to define a serial interface"
#endif

#include "ui/core/UiTask.hpp"
static Heltec::MeshCore::Ui::UiTask s_uiTask(&serial_interface);
namespace Heltec::MeshCore::Ui
{
UiTask &uiTask()
{
    return s_uiTask;
}
} // namespace Heltec::MeshCore::Ui

/* GLOBAL OBJECTS */

StdRNG fast_rng;
SimpleMeshTables tables;

HeltecMesh the_mesh(radio_driver, fast_rng, rtc_clock, tables, store,
                    &Heltec::MeshCore::Ui::uiTask());

/* END GLOBAL OBJECTS */

namespace
{

class ApplicationPowerPlatform final : public Heltec::MeshCore::Power::PowerPlatform
{
  public:
    mesh::PowerSource getPowerSource() const override
    {
#if defined(NRF52_PLATFORM)
        return board.isExternalPowered() ? mesh::PowerSource::External : mesh::PowerSource::Battery;
#else
        return board.getPowerSource();
#endif
    }

    bool isDisplayOn() const override
    {
        return Heltec::MeshCore::Dal::DisplayPort::isBacklightOn();
    }

    void setDisplayOn(bool on) override
    {
        if (on)
        {
            Heltec::MeshCore::Dal::DisplayPort::resume();
        }
        else
        {
            Heltec::MeshCore::Dal::DisplayPort::suspend();
        }
    }

    bool isGpsAvailable() const override
    {
        return sensors.getLocationProvider() != nullptr;
    }

    bool isGpsPowered() const override
    {
        LocationProvider *location = sensors.getLocationProvider();
        return location && location->isEnabled();
    }

    bool setGpsPowered(bool on) override
    {
        LocationProvider *location = sensors.getLocationProvider();
        if (!location)
            return !on;
        if (location->isEnabled() == on)
            return true;
        if (!sensors.setSettingValue("gps", on ? "1" : "0"))
            return false;
        return location->isEnabled() == on;
    }

    bool isCompassAvailable() const override
    {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        return true;
#else
        return false;
#endif
    }

    bool isCompassSampling() const override
    {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        extern ICMCompassProvider compassProvider;
        return compassProvider.samplingEnabled();
#else
        return false;
#endif
    }

    bool setCompassSampling(bool on) override
    {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        extern ICMCompassProvider compassProvider;
        compassProvider.setSamplingEnabled(on);
        return compassProvider.samplingEnabled() == on;
#else
        return !on;
#endif
    }

    void startShutdownFeedback() override
    {
        Heltec::MeshCore::Ui::uiTask().startShutdownFeedback();
    }

    bool shutdownFeedbackDone() override
    {
        return Heltec::MeshCore::Ui::uiTask().shutdownFeedbackDone();
    }

    void powerOffRadio() override
    {
        radio_driver.powerOff();
    }
};

class MeshAppRT final : public Heltec::MeshCore::Power::PowerListener
{
  public:
    ApplicationPowerPlatform power_platform;
    Heltec::MeshCore::Power::PowerMgr power{ board, power_platform };
    Heltec::MeshCore::Biz::MeshAppUi biz{ power };
    Heltec::MeshCore::Ui::UiApp ui{ biz, power };

    static MeshAppRT &instance()
    {
        static MeshAppRT core;
        return core;
    }

    MeshAppRT(const MeshAppRT &) = delete;
    MeshAppRT &operator=(const MeshAppRT &) = delete;
    MeshAppRT(MeshAppRT &&) = delete;
    MeshAppRT &operator=(MeshAppRT &&) = delete;

    void pollBackend()
    {
        biz.poll();
    }

    void onPowerChanged(Heltec::MeshCore::Power::PowerChangeMask changes,
                        const Heltec::MeshCore::Power::PowerSnapshot &snapshot) override
    {
        biz.handlePowerChanged(changes, snapshot);
        ui.handlePowerChanged(changes, snapshot);
    }

  private:
    MeshAppRT()
    {
        power.setListener(this);
    }
};

} // namespace

static void requestRadioPresetOverlayIfUnconfigured(bool hasDisplay)
{
    if (!hasDisplay)
        return;
    NodePrefs *p = the_mesh.getNodePrefs();
    if (p && p->lora_band_configured == 0)
    {
        MeshAppRT::instance().ui.openRadioParamSyncOverlay();
    }
}

void halt()
{
    while (1);
}

void setup()
{
    Serial.begin(115200);
#if defined(MESH_DEBUG) && MESH_DEBUG
    {
        unsigned long start_time = millis();
        while (!Serial && (millis() - start_time < 5000))
        {
            delay(10);
        }
    }
#endif
    lv_init();
    board.begin();
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after board.begin");
#endif
    Heltec::MeshCore::Dal::DisplayPort::setBacklightOn(true);
    MeshAppRT &core = MeshAppRT::instance();
    const bool hasDisplay = core.ui.init();
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after UI init");
#endif
    if (hasDisplay)
    {
        for (uint8_t i = 0; i < 8; ++i)
        {
            lv_timer_handler();
            if (lv_disp_t *d = lv_disp_get_default())
            {
                lv_refr_now(d);
            }
        }
    }

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    InternalFS.begin();
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after InternalFS.begin");
#endif
#if defined(QSPIFLASH)
    if (!QSPIFlash.begin())
    {
        MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: failed to initialize");
    }
    else
    {
        MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: initialized successfully");
    }
#else
#if defined(EXTRAFS)
    ExtraFS.begin();
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after ExtraFS.begin");
#endif
#endif
#endif
#endif
#if defined(ESP32)
    // Recover an unmountable SPIFFS volume by formatting it once during boot.
    const bool fs_ok = SPIFFS.begin(true);
    if (!fs_ok)
    {
        MESH_DEBUG_PRINTLN("[fs] SPIFFS mount/format failed");
    }
    else
    {
        MESH_DEBUG_PRINTLN("[fs] SPIFFS mounted");
    }
#endif
#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
#if HELTEC_LICENSE_BOOT_GATE
    if (!heltecLicenseBootGate(hasDisplay))
    {
        halt();
    }
#endif
#endif
    if (!radio_init())
    {
        halt();
    }
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after radio_init");
#endif

    fast_rng.begin(radio_get_rng_seed());
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    store.begin();
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after store.begin");
#endif
    the_mesh.begin(hasDisplay);
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("after the_mesh.begin");
#endif

#ifdef BLE_PIN_CODE
    serial_interface.begin(BLE_NAME_PREFIX, the_mesh.getNodePrefs()->node_name,
                           the_mesh.getBLEPin());
#else
    serial_interface.begin(Serial);
#endif
    the_mesh.startInterface(serial_interface);
    if (NodePrefs *p = the_mesh.getNodePrefs())
    {
        if (0 == p->companion_link_enabled)
        {
            Heltec::MeshCore::Ui::uiTask().disableSerial();
        }
    }
#elif defined(RP2040_PLATFORM)
    LittleFS.begin();
    store.begin();
    the_mesh.begin(
#ifdef DISPLAY_CLASS
        disp != nullptr
#else
        false
#endif
    );

// #ifdef WIFI_SSID
//   WiFi.begin(WIFI_SSID, WIFI_PWD);
//   serial_interface.begin(TCP_PORT);
//  #elif defined(BLE_PIN_CODE)
//    char dev_name[32+16];
//    sprintf(dev_name, "%s%s", BLE_NAME_PREFIX, the_mesh.getNodeName());
//    serial_interface.begin(dev_name, the_mesh.getBLEPin());
#if defined(SERIAL_RX)
    companion_serial.setPins(SERIAL_RX, SERIAL_TX);
    companion_serial.begin(115200);
    serial_interface.begin(companion_serial);
#else
    serial_interface.begin(Serial);
#endif
    the_mesh.startInterface(serial_interface);
#elif defined(ESP32)
    store.begin();
    the_mesh.begin(hasDisplay);

#ifdef WIFI_SSID
    board.setInhibitSleep(true);
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    serial_interface.begin(TCP_PORT);
#elif defined(BLE_PIN_CODE)
    serial_interface.begin(BLE_NAME_PREFIX, the_mesh.getNodePrefs()->node_name,
                           the_mesh.getBLEPin());
#elif defined(SERIAL_RX)
    companion_serial.setPins(SERIAL_RX, SERIAL_TX);
    companion_serial.begin(115200);
    serial_interface.begin(companion_serial);
#else
    serial_interface.begin(Serial);
#endif
    the_mesh.startInterface(serial_interface);
    if (NodePrefs *p = the_mesh.getNodePrefs())
    {
        if (0 == p->companion_link_enabled)
        {
            Heltec::MeshCore::Ui::uiTask().disableSerial();
        }
    }
#else
#error "need to define filesystem"
#endif
#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
    {
        Heltec::MeshCore::Ui::AppStateEvent config_event{};
        config_event.type = Heltec::MeshCore::Ui::AppStateEventType::ConfigChanged;
        Heltec::MeshCore::Ui::app_state_notifier().notify(config_event);
    }
#endif
    store.notifyBootRegionMapStorageDone();
#if defined(RADIOCORE_BOARD_RC52) && defined(PIN_SENSOR_EN)
    logRc52SensorPowerState("before sensors.begin");
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    {
        extern ICMCompassProvider compassProvider;
        float hmm[4];
        if (store.loadCompassMagCal(hmm))
        {
            compassProvider.applyMagCalibration(hmm);
        }
    }
#endif
    sensors.begin();
    {
        Heltec::MeshCore::Power::PowerConfig power_config =
            Heltec::MeshCore::Power::defaultPowerConfig();
        power_config.display_available = hasDisplay;
        core.power.begin(millis(), power_config);
        the_mesh.attachPowerMgr(&core.power);
    }
#if ENV_INCLUDE_GPS == 1
    // Apply stored GPS prefs to hardware (initBasicGPS leaves GPS off until this).
    if (NodePrefs *p = the_mesh.getNodePrefs())
    {
        core.biz.setGpsEnabled(p->gps_enabled != 0);
        if (p->gps_interval > 0)
        {
            char interval_str[12];
            snprintf(interval_str, sizeof(interval_str), "%u", (unsigned)p->gps_interval);
            sensors.setSettingValue("gps_interval", interval_str);
        }
        if (p->gps_track_armed && p->gps_enabled)
        {
            core.biz.setGpsTrackRecording(true);
        }
    }
#endif
    if (hasDisplay)
    {
        if (NodePrefs *p = the_mesh.getNodePrefs())
        {
            core.ui.setDisplayAutoOffMs((uint32_t)p->display_auto_off_sec * 1000u);
        }
    }

    Heltec::MeshCore::Ui::uiTask().begin(the_mesh.getNodePrefs());
    requestRadioPresetOverlayIfUnconfigured(hasDisplay);
}

void loop()
{
    MeshAppRT &core = MeshAppRT::instance();
    the_mesh.loop();
    sensors.loop();
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    // Sample the stable GPS layer after the provider has consumed this loop's
    // NMEA data, so non-UI consumers do not depend on a page being open.
    HeltecMesh::StableGpsFixSnapshot stable_gps{};
    (void)the_mesh.getStableGpsFix(stable_gps);
#endif
    core.pollBackend();
    Heltec::MeshCore::Ui::uiTask().loop();
    core.ui.tick();
    core.power.loop(millis());
    rtc_clock.tick();

#if defined(NRF52_PLATFORM)
    // Preserve RAM and the active LoRa receive path while waiting for the next
    // interrupt. This restores MeshCore's native nRF52 System ON idle model.
    board.sleep(0);
#else
    const auto &power_snapshot = core.power.snapshot();
    const bool peripherals_active = power_snapshot.display_on || power_snapshot.gps_powered ||
                                    Heltec::MeshCore::Ui::uiTask().isBuzzerPlaying() ||
                                    Heltec::MeshCore::Dal::MomentaryButton::hasPendingGesture();
    if (peripherals_active)
    {
        // Touch and GPS are polled peripherals on ESP32. Keep the cooperative
        // loop running while either one is active, and let FreeRTOS idle briefly.
        delay(5);
    }
    else
    {
#if defined(PIN_BUZZER) && PIN_BUZZER >= 0
        digitalWrite(PIN_BUZZER, LOW);
#endif
        // A fixed short tick keeps software timers simple and bounded while
        // GPIO wake sources still provide immediate response.
        board.sleepMillis(250U);
    }
#endif
}
