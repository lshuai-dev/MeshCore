#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>
#include "HeltecMesh.h"
#include "app/mesh_app_ui.hpp"
#include "ui/app/ui_app.hpp"
#include "heltec/drivers/display/display_port.hpp"
#include "lvgl.h"
#include "config/NodePrefs.h"
#include "MeshCore.h"
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include "config/heltec_license.h"
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include <heltec/sensors/ICMCompassProvider.h>
#endif

static uint32_t _atoi(const char* sp) {
  uint32_t n = 0;
  while (*sp && *sp >= '0' && *sp <= '9') {
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
  //#ifdef WIFI_SSID
  //  #include <helpers/rp2040/SerialWifiInterface.h>
  //  SerialWifiInterface serial_interface;
  //  #ifndef TCP_PORT
  //    #define TCP_PORT 5000
  //  #endif
  // #elif defined(BLE_PIN_CODE)
  //   #include <helpers/rp2040/SerialBLEInterface.h>
  //   SerialBLEInterface serial_interface;
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

#include "ui/core/ui_task.hpp"
static heltec::meshcore::ui::UiTask s_ui_task(&board, &serial_interface);
namespace heltec::meshcore::ui {
  UiTask& ui_task() { return s_ui_task; }
}

/* GLOBAL OBJECTS */

StdRNG fast_rng;
SimpleMeshTables tables;

HeltecMesh the_mesh(radio_driver, fast_rng, rtc_clock, tables, store,&heltec::meshcore::ui::ui_task());

/* END GLOBAL OBJECTS */

namespace {

struct ApplicationRuntime {
  heltec::meshcore::biz::MeshAppUi biz;
  heltec::meshcore::ui::UiApp ui{biz};

  void pollBackend() { biz.pollRuntime(); }
};

ApplicationRuntime& applicationRuntime() {
  static ApplicationRuntime runtime;
  return runtime;
}

}  // namespace

static void requestRadioPresetOverlayIfUnconfigured(bool hasDisplay) {
  if (!hasDisplay) return;
  NodePrefs* p = the_mesh.getNodePrefs();
  if (p && p->lora_band_configured == 0) {
    applicationRuntime().biz.requestRadioParamPresetPicker();
  }
}

void halt() {
  while (1) ;
}

void setup() {
  Serial.begin(115200);
#if defined(MESH_DEBUG) && MESH_DEBUG
  {
    unsigned long start_time = millis();
    while (!Serial && (millis() - start_time < 5000)) {
      delay(10);
    }
  }
#endif
  lv_init();
  board.begin();
  heltec::meshcore::dal::display_port::setBacklightOn(true);
  ApplicationRuntime& runtime = applicationRuntime();
  runtime.ui.init();
  const bool hasDisplay = runtime.ui.isReady();
  if (hasDisplay) {
    for (uint8_t i = 0; i < 8; ++i) {
      lv_timer_handler();
      if (lv_disp_t* d = lv_disp_get_default()) {
        lv_refr_now(d);
      }
    }
  }
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  InternalFS.begin();
  #if defined(QSPIFLASH)
    if (!QSPIFlash.begin()) {
      MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: failed to initialize");
    } else {
      MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: initialized successfully");
    }
  #else
  #if defined(EXTRAFS)
    ExtraFS.begin();
  #endif
  #endif
#endif
#if defined(ESP32)
  // Recover an unmountable SPIFFS volume by formatting it once during boot.
  const bool fs_ok = SPIFFS.begin(true);
  if (!fs_ok) {
    MESH_DEBUG_PRINTLN("[fs] SPIFFS mount/format failed");
  } else {
    MESH_DEBUG_PRINTLN("[fs] SPIFFS mounted");
  }
#endif
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#if defined(NRF52_PLATFORM)
  heltecLicenseSetFilesystem(
#if defined(EXTRAFS)
      &ExtraFS
#else
      &InternalFS
#endif
  );
#elif defined(ESP32)
  heltecLicenseSetFilesystem(&SPIFFS);
#endif
#if HELTEC_LICENSE_BOOT_GATE
  if (!heltecLicenseBootGate(hasDisplay)) {
    halt();
  }
#endif
#endif
  if (!radio_init()) { halt(); }
  fast_rng.begin(radio_get_rng_seed());
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  store.begin();
  the_mesh.begin(hasDisplay);
  // request_radio_preset_overlay_if_unconfigured(hasDisplay);

#ifdef BLE_PIN_CODE
  serial_interface.begin(BLE_NAME_PREFIX, the_mesh.getNodePrefs()->node_name, the_mesh.getBLEPin());
#else
  serial_interface.begin(Serial);
#endif
  the_mesh.startInterface(serial_interface);
  if (NodePrefs* p = the_mesh.getNodePrefs()) {
    HeltecMesh::setLocShareAdvertIntervalSec(p->loc_share_adv_sec);
    if (0 == p->companion_link_enabled) {
      heltec::meshcore::ui::ui_task().disableSerial();
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

  //#ifdef WIFI_SSID
  //  WiFi.begin(WIFI_SSID, WIFI_PWD);
  //  serial_interface.begin(TCP_PORT);
  // #elif defined(BLE_PIN_CODE)
  //   char dev_name[32+16];
  //   sprintf(dev_name, "%s%s", BLE_NAME_PREFIX, the_mesh.getNodeName());
  //   serial_interface.begin(dev_name, the_mesh.getBLEPin());
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
  serial_interface.begin(BLE_NAME_PREFIX, the_mesh.getNodePrefs()->node_name, the_mesh.getBLEPin());
#elif defined(SERIAL_RX)
  companion_serial.setPins(SERIAL_RX, SERIAL_TX);
  companion_serial.begin(115200);
  serial_interface.begin(companion_serial);
#else
  serial_interface.begin(Serial);
#endif
  the_mesh.startInterface(serial_interface);
  if (NodePrefs* p = the_mesh.getNodePrefs()) {
    HeltecMesh::setLocShareAdvertIntervalSec(p->loc_share_adv_sec);
    if (0 == p->companion_link_enabled) {
      heltec::meshcore::ui::ui_task().disableSerial();
    }
  }
#else
  #error "need to define filesystem"
#endif
  store.notifyBootRegionMapStorageDone();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  {
    extern ICMCompassProvider compassProvider;
    float hmm[4];
    if (store.loadCompassMagCal(hmm)) {
      compassProvider.applyMagCalibration(hmm);
    }
  }
#endif
  sensors.begin();
#if ENV_INCLUDE_GPS == 1
  // Apply stored GPS prefs to hardware (initBasicGPS leaves GPS off until this).
  if (NodePrefs* p = the_mesh.getNodePrefs()) {
    runtime.biz.setGpsEnabled(p->gps_enabled != 0);
    if (p->gps_interval > 0) {
      char interval_str[12];
      snprintf(interval_str, sizeof(interval_str), "%u", (unsigned)p->gps_interval);
      sensors.setSettingValue("gps_interval", interval_str);
    }
    if (p->gps_track_armed && p->gps_enabled) {
      runtime.biz.setGpsTrackRecording(true);
    }
  }
#endif
  if (hasDisplay) {
    if (NodePrefs* p = the_mesh.getNodePrefs()) {
      runtime.ui.setDisplayAutoOffMs((uint32_t)p->display_auto_off_sec * 1000u);
    }
  }

  heltec::meshcore::ui::ui_task().begin(the_mesh.getNodePrefs());
  requestRadioPresetOverlayIfUnconfigured(hasDisplay);
}

void loop() {
  ApplicationRuntime& runtime = applicationRuntime();
  the_mesh.loop();
  HeltecMesh::pollLocShareAdvert(the_mesh);
  sensors.loop();
  runtime.pollBackend();
  heltec::meshcore::ui::ui_task().loop();
  runtime.ui.tick();
  runtime.biz.reconcileGpsPower();
  rtc_clock.tick();

  // Both supported Arduino cores run loop() from a FreeRTOS task.  Without a
  // blocking point this low-priority task remains permanently runnable, so
  // the idle task cannot enter ESP32/nRF52 low-power idle between events.
  // One tick keeps radio/UI latency bounded while allowing tickless idle.
  delay(5);
}
