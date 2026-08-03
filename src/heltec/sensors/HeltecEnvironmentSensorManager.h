#pragma once

#include <Mesh.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>
#if ENV_INCLUDE_COMPASS
#include "CompassProvider.h"
#endif

#ifndef GPS_NOTIFY_INTERVAL_SEC
#define GPS_NOTIFY_INTERVAL_SEC 1
#endif
#ifndef GPS_FIX_VALID_WINDOW_MS
#define GPS_FIX_VALID_WINDOW_MS 5000
#endif

class HeltecEnvironmentSensorManager : public SensorManager {
protected:
  int next_available_channel = TELEM_CHANNEL_SELF + 1;

  bool AHTX0_initialized = false;
  bool BME280_initialized = false;
  bool BMP280_initialized = false;
  bool INA3221_initialized = false;
  bool INA219_initialized = false;
  bool INA260_initialized = false;
  bool INA226_initialized = false;
  bool SHTC3_initialized = false;
  bool LPS22HB_initialized = false;
  bool MLX90614_initialized = false;
  bool VL53L0X_initialized = false;
  bool SHT4X_initialized = false;
  bool BME680_initialized = false;
  bool BMP085_initialized = false;

  bool gps_detected = false;
  bool gps_active = false;
  uint32_t gps_update_interval_sec = GPS_NOTIFY_INTERVAL_SEC;
  bool gps_fix_seen = false;
  uint32_t gps_last_fix_ms = 0;
  long gps_last_fix_timestamp = 0;
  long gps_last_fix_lat = 0;
  long gps_last_fix_lon = 0;
  long gps_last_fix_alt = 0;
  long gps_last_fix_sats = 0;
  uint8_t gps_config_step = 0;
  uint32_t gps_config_due_ms = 0;
  bool gps_config_pending = false;

#if ENV_INCLUDE_GPS
  LocationProvider* _location;
  void start_gps();
  void stop_gps();
  void initBasicGPS();
  void beginGpsModuleConfiguration();
  void pollGpsModuleConfiguration();
  void clearGpsFixState();
  void updateGpsFixState();
#ifdef RAK_BOARD
  void rakGPSInit();
  bool gpsIsAwake(uint8_t ioPin);
#endif
#endif

#if ENV_INCLUDE_COMPASS
  CompassProvider* _compass = nullptr;
#endif

public:
#if ENV_INCLUDE_GPS
  explicit HeltecEnvironmentSensorManager(LocationProvider& location) : _location(&location) {}
#if ENV_INCLUDE_COMPASS
  HeltecEnvironmentSensorManager(LocationProvider& location, CompassProvider& compass)
      : _location(&location), _compass(&compass) {}
#endif
#else
  HeltecEnvironmentSensorManager() {}
#if ENV_INCLUDE_COMPASS
  explicit HeltecEnvironmentSensorManager(CompassProvider& compass) : _compass(&compass) {}
#endif
#endif

#if ENV_INCLUDE_GPS
  LocationProvider* getLocationProvider() { return _location; }
  bool isGpsActive() const { return gps_active; }
  uint32_t gpsFixValidMs() const;
  bool hasFreshGpsFix() const {
    return gps_fix_seen && gpsFixValidMs() <= GPS_FIX_VALID_WINDOW_MS;
  }
#endif
#if ENV_INCLUDE_COMPASS
  CompassProvider* getCompassProvider() { return _compass; }
#endif

  bool begin() override;
  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
#if ENV_INCLUDE_GPS || ENV_INCLUDE_COMPASS
  void loop() override;
#endif
  int getNumSettings() const override;
  const char* getSettingName(int i) const override;
  const char* getSettingValue(int i) const override;
  bool setSettingValue(const char* name, const char* value) override;
};
