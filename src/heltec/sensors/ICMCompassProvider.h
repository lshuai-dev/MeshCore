#pragma once
#if ENV_INCLUDE_COMPASS
#include <Arduino.h>
#include <Wire.h>
#include "CompassProvider.h"

namespace heltec::meshcore::biz {
struct CompassUi;
}

#include <SparkFun_MMC5983MA_Arduino_Library.h>
#include <MemsicAlgo.h>
#include <MemsicCompass.h>
#include <helpers/RefCountedDigitalPin.h>

class ICMCompassProvider : public CompassProvider {
public:
  explicit ICMCompassProvider() {}

  void begin() override;
  void loop() override;
  bool getResult(CompassResult* result) override;
  bool fillUiSnapshot(heltec::meshcore::biz::CompassUi& ui) const;
  void setCalibrationState(bool bEnable) override;
  bool hasHardware() const override { return _ready; }
  void applyMagCalibration(const float hmm[4]);
  bool exportMagCalibration(float hmm[4]);

private:
  static bool probeI2CAddress(TwoWire& wire, uint8_t address);
  static bool detectIcm42607Address(TwoWire& wire, uint8_t& outAddress);
  static float wrapAngle360(float angle);
  static float applyHeadingAlignment(float headingDegrees);
  static float compute2DHeadingDegrees(const float* mag_gauss);
  static void mapAxes(int layout, const float* in, float* out, bool invert_default_z);
  static void handleError(SF_MMC5983MA_ERROR errorCode);

  struct SensorHealth {
    uint32_t consecutive_failures = 0;
    uint32_t total_failures = 0;
    uint32_t last_log_ms = 0;

    void reset() {
      consecutive_failures = 0;
      total_failures = 0;
      last_log_ms = 0;
    }

    void clearConsecutiveFailures() { consecutive_failures = 0; }
  };

  bool initializeSensors();
  bool initAccelerometer();
  bool initMagnetometer();
  bool configureAccelerometer();
  bool configureMagnetometer();
  bool resetAccelerometer();
  bool resetMagnetometer();
  bool reconnectAccelerometer();
  bool reconnectMagnetometer();
  bool reconnectSensorsIfNeeded(uint32_t now_ms);
  bool recoverSensorsIfNeeded(uint32_t now_ms);
  bool writeIcmRegister(uint8_t reg, uint8_t value);
  bool readIcmRegisters(uint8_t reg, uint8_t* buffer, size_t length);
  bool readAccelerometerG(float* acc_g);
  bool readMagnetometerG(float* mag_g);
  void updateHeadingFilter(float headingDegrees);
  void updateCalibrationIfChanged();
  bool shouldNotifyUi(const heltec::meshcore::biz::CompassUi& ui, uint32_t now_ms);
  void resetRuntimeState();
  bool resetCompassAlgorithm();
  void recordAccelerometerFailure(uint32_t now_ms);
  void recordMagnetometerFailure(uint32_t now_ms);

  SFE_MMC5983MA           _mag;
  TwoWire*                _wire = &Wire;
  uint8_t                 _icmAddress = 0x68;
  bool                    _ready = false;
  float                   _magSmm[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  float                   _magHmm[4] = {0.0f, 0.0f, 0.0f, 0.5f};
  float                   _lastAppliedMagHmm[4] = {0.0f, 0.0f, 0.0f, 0.5f};
  bool                    _headingFilterReady = false;
  CompassResult           _lastResult = {0};
  bool                    _calibrating = false;
  float                   _accRawG[3] = {0.0f, 0.0f, 0.0f};
  float                   _magRawG[3] = {0.0f, 0.0f, 0.0f};
  uint32_t                _last_loop_ms = 0;
  uint32_t                _last_good_sample_ms = 0;
  uint32_t                _last_notify_ms = 0;
  bool                    _last_notify_snapshot = false;
  bool                    _last_notify_heading_valid = false;
  int                     _last_notify_quality = -1;
  SensorHealth            _accel_health;
  SensorHealth            _mag_health;
  uint32_t                _last_recovery_ms = 0;
  uint32_t                _recovery_count = 0;
};

#endif // ENV_INCLUDE_COMPASS
