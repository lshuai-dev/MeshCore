#include "ICMCompassProvider.h"
#if ENV_INCLUDE_COMPASS

#include "heltec/ui/core/biz_facade.hpp"
#include "heltec/ui/core/app_state_notifier.hpp"
#include <Wire.h>
#include <cmath>

// MMC5983MA output is 18-bit unsigned with midpoint 2^17 and +/-8 gauss full-scale.
static constexpr float MMC5983_ZERO_OFFSET = 131072.0f;
static constexpr float MMC5983_COUNTS_PER_GAUSS = 16384.0f;

#ifndef ICM42607_COMPASS_A_LAYOUT
#define ICM42607_COMPASS_A_LAYOUT 2
#endif
#ifndef ICM42607_COMPASS_M_LAYOUT
#define ICM42607_COMPASS_M_LAYOUT 0
#endif

#ifndef COMPASS_DECLINATION_DEG
#define COMPASS_DECLINATION_DEG 0.0f
#endif

#ifndef COMPASS_HEADING_ALIGNMENT_DEG
#define COMPASS_HEADING_ALIGNMENT_DEG 0.0f
#endif

#ifndef ICM_COMPASS_LOOP_MS
#define ICM_COMPASS_LOOP_MS 20
#endif

#ifndef COMPASS_NOTIFY_INTERVAL_MS
#define COMPASS_NOTIFY_INTERVAL_MS 200
#endif

#ifndef COMPASS_HEADING_FILTER_ALPHA
#define COMPASS_HEADING_FILTER_ALPHA 0.2f
#endif

#ifndef ICM_COMPASS_ACCEL_FAILURE_THRESHOLD
#define ICM_COMPASS_ACCEL_FAILURE_THRESHOLD 5
#endif

#ifndef ICM_COMPASS_MAG_FAILURE_THRESHOLD
#define ICM_COMPASS_MAG_FAILURE_THRESHOLD 3
#endif

#ifndef ICM_COMPASS_RECOVERY_INTERVAL_MS
#define ICM_COMPASS_RECOVERY_INTERVAL_MS 1000
#endif

static constexpr float kDeclinationDeg = (float)COMPASS_DECLINATION_DEG;
static constexpr float kHeadingAlignmentDeg = (float)COMPASS_HEADING_ALIGNMENT_DEG;
static constexpr int kALayout = (int)ICM42607_COMPASS_A_LAYOUT;
static constexpr int kMLayout = (int)ICM42607_COMPASS_M_LAYOUT;

// ICM-42607-P register map and scaling for +/-2 g accelerometer output.
static constexpr uint8_t ICM42607_I2C_ADDR_LOW = 0x68;
static constexpr uint8_t ICM42607_I2C_ADDR_HIGH = 0x69;
static constexpr uint8_t ICM42607_REG_ACCEL_DATA_X1 = 0x0B;
static constexpr uint8_t ICM42607_REG_DEVICE_CONFIG = 0x11;
static constexpr uint8_t ICM42607_REG_PWR_MGMT0 = 0x1F;
static constexpr uint8_t ICM42607_REG_ACCEL_CONFIG0 = 0x21;
static constexpr uint8_t ICM42607_REG_ACCEL_CONFIG1 = 0x24;
static constexpr uint8_t ICM42607_REG_WHO_AM_I = 0x75;
static constexpr uint8_t ICM42607_WHO_AM_I_VALUE = 0x60;
static constexpr uint8_t ICM42607_DEVICE_CONFIG_SOFT_RESET = 0x01;
static constexpr uint8_t ICM42607_PWR_MGMT0_ACCEL_LN = 0x03;
static constexpr uint8_t ICM42607_ACCEL_CONFIG0_FS_2G = 0x60;
static constexpr uint8_t ICM42607_ACCEL_CONFIG0_ODR_100HZ = 0x09;
static constexpr uint8_t ICM42607_ACCEL_CONFIG1_UI_FILT_34HZ = 0x05;
static constexpr float ICM42607_COUNTS_PER_G = 16384.0f;
static constexpr uint32_t kAccelFailureLogIntervalMs = 5000;
static constexpr uint32_t kMagFailureLogIntervalMs = 5000;

static inline bool magParamsEqual4(const float a[4], const float b[4], float eps = 1e-4f) {
  for (int i = 0; i < 4; ++i) {
    if (fabsf(a[i] - b[i]) > eps) return false;
  }
  return true;
}

void ICMCompassProvider::recordAccelerometerFailure(uint32_t now) {
  _accel_health.consecutive_failures++;
  _accel_health.total_failures++;

  const bool first_fail = (_accel_health.consecutive_failures == 1);
  const bool periodic_streak = (_accel_health.consecutive_failures % 20U) == 0U;
  const bool periodic_time =
      (_accel_health.last_log_ms == 0) ||
      (uint32_t)(now - _accel_health.last_log_ms) >= kAccelFailureLogIntervalMs;
  if (!first_fail && !periodic_streak && !periodic_time) return;

  _accel_health.last_log_ms = now;
  MESH_DEBUG_PRINTLN(
      "ICM42607 accel read failed: addr=0x%02x wire=%p consecutive=%lu total=%lu interval_ms=%u",
      _icmAddress, (void*)_wire, (unsigned long)_accel_health.consecutive_failures,
      (unsigned long)_accel_health.total_failures, (unsigned)ICM_COMPASS_LOOP_MS);

  uint8_t who = 0x00;
  if (readIcmRegisters(ICM42607_REG_WHO_AM_I, &who, 1)) {
    MESH_DEBUG_PRINTLN("ICM42607 WHO_AM_I=0x%02x (expected 0x%02x)", who,
                       ICM42607_WHO_AM_I_VALUE);
  } else {
    MESH_DEBUG_PRINTLN("ICM42607 WHO_AM_I unreadable after accel failure");
  }

  uint8_t pwr = 0x00;
  if (readIcmRegisters(ICM42607_REG_PWR_MGMT0, &pwr, 1)) {
    MESH_DEBUG_PRINTLN("ICM42607 PWR_MGMT0=0x%02x", pwr);
  }
}

void ICMCompassProvider::recordMagnetometerFailure(uint32_t now) {
  _mag_health.consecutive_failures++;
  _mag_health.total_failures++;

  const bool first_fail = (_mag_health.consecutive_failures == 1);
  const bool periodic_streak = (_mag_health.consecutive_failures % 20U) == 0U;
  const bool periodic_time =
      (_mag_health.last_log_ms == 0) ||
      (uint32_t)(now - _mag_health.last_log_ms) >= kMagFailureLogIntervalMs;
  if (!first_fail && !periodic_streak && !periodic_time) return;

  _mag_health.last_log_ms = now;
  MESH_DEBUG_PRINTLN(
      "MMC5983MA mag read failed: consecutive=%lu total=%lu interval_ms=%u",
      (unsigned long)_mag_health.consecutive_failures, (unsigned long)_mag_health.total_failures,
      (unsigned)ICM_COMPASS_LOOP_MS);

  MESH_DEBUG_PRINTLN("MMC5983MA connected=%d", _mag.isConnected() ? 1 : 0);
}

void ICMCompassProvider::resetRuntimeState() {
  _ready = false;
  _headingFilterReady = false;
  _last_loop_ms = 0;
  _last_notify_ms = 0;
  _last_notify_snapshot = false;
  _last_notify_heading_valid = false;
  _last_notify_quality = -1;
  _lastResult = {};
  memset(_accRawG, 0, sizeof(_accRawG));
  memset(_magRawG, 0, sizeof(_magRawG));
  _accel_health.reset();
  _mag_health.reset();
  _last_recovery_ms = 0;
  _recovery_count = 0;
}

bool ICMCompassProvider::initializeSensors() {
  _ready = false;
  if (!initAccelerometer()) return false;
  if (!initMagnetometer()) return false;
  if (!resetCompassAlgorithm()) return false;
  _ready = true;
  _accel_health.clearConsecutiveFailures();
  _mag_health.clearConsecutiveFailures();
  return true;
}

void ICMCompassProvider::begin() {
  resetRuntimeState();
  (void)initializeSensors();
}

bool ICMCompassProvider::getResult(CompassResult* result) {
  if (!result) return false;
  if (!_ready) return false;
  *result = _lastResult;
  return true;
}

bool ICMCompassProvider::fillUiSnapshot(heltec::meshcore::biz::CompassUi& ui) const {
  ui = {};
  if (!_ready) return false;
  ui.has_hardware = true;
  ui.heading_valid = true;
  ui.heading_deg = _lastResult.fFilteredHeadingDegrees;
  ui.azimuth_deg = _lastResult.fAzimuth;
  ui.mag_xyz[0] = _lastResult.coordinates[0];
  ui.mag_xyz[1] = _lastResult.coordinates[1];
  ui.mag_xyz[2] = _lastResult.coordinates[2];
  {
    int q = (int)(_lastResult.fQuality + 0.5f);
    if (q < 0) q = 0;
    if (q > 3) q = 3;
    ui.quality = q;
  }
  return true;
}

bool ICMCompassProvider::shouldNotifyUi(const heltec::meshcore::biz::CompassUi& ui, uint32_t now_ms) {
  const bool state_changed = !_last_notify_snapshot ||
      ui.heading_valid != _last_notify_heading_valid ||
      ui.quality != _last_notify_quality;
  const bool interval_due = _last_notify_ms == 0 ||
      (uint32_t)(now_ms - _last_notify_ms) >= (uint32_t)COMPASS_NOTIFY_INTERVAL_MS;
  if (!state_changed && !interval_due) return false;

  _last_notify_ms = now_ms;
  _last_notify_snapshot = true;
  _last_notify_heading_valid = ui.heading_valid;
  _last_notify_quality = ui.quality;
  return true;
}

void ICMCompassProvider::applyMagCalibration(const float hmm[4]) {
  if (!hmm) return;
  memcpy(_magHmm, hmm, sizeof(_magHmm));
  if (_ready) {
    if (InitialAlgorithm(_magSmm, _magHmm) != 1) {
      memcpy(_magHmm, _lastAppliedMagHmm, sizeof(_magHmm));
      return;
    }
    memcpy(_lastAppliedMagHmm, _magHmm, sizeof(_magHmm));
  }
}

bool ICMCompassProvider::exportMagCalibration(float hmm[4]) {
  if (!hmm || !_ready) return false;
  (void)GetCalPara(hmm);
  memcpy(_magHmm, hmm, sizeof(_magHmm));
  memcpy(_lastAppliedMagHmm, hmm, sizeof(_lastAppliedMagHmm));
  return true;
}

void ICMCompassProvider::setCalibrationState(bool bEnable) {
  if (_calibrating != bEnable) {
    _calibrating = bEnable;
    if (GetCalPara(&_magHmm[0])) {
      InitialAlgorithm(_magSmm, _magHmm);
      memcpy(_lastAppliedMagHmm, _magHmm, sizeof(_magHmm));
    }
    if (!bEnable) {
      _headingFilterReady = false;
    }
  }
}

void ICMCompassProvider::loop() {
  const uint32_t now = millis();
  if (!_ready) {
    reconnectSensorsIfNeeded(now);
    return;
  }

  if (_last_loop_ms != 0 && (uint32_t)(now - _last_loop_ms) < (uint32_t)ICM_COMPASS_LOOP_MS) {
    return;
  }
  _last_loop_ms = now;

  const bool accel_ok = readAccelerometerG(_accRawG);
  if (!accel_ok) {
    recordAccelerometerFailure(now);
  } else {
    if (_accel_health.consecutive_failures != 0) {
      MESH_DEBUG_PRINTLN("ICM42607 accel read recovered after consecutive=%lu total=%lu",
                         (unsigned long)_accel_health.consecutive_failures,
                         (unsigned long)_accel_health.total_failures);
    }
    _accel_health.clearConsecutiveFailures();
  }

  const bool mag_ok = readMagnetometerG(_magRawG);
  if (!mag_ok) {
    recordMagnetometerFailure(now);
  } else {
    if (_mag_health.consecutive_failures != 0) {
      MESH_DEBUG_PRINTLN("MMC5983MA mag read recovered after consecutive=%lu total=%lu",
                         (unsigned long)_mag_health.consecutive_failures,
                         (unsigned long)_mag_health.total_failures);
    }
    _mag_health.clearConsecutiveFailures();
  }

  if (!accel_ok || !mag_ok) {
    recoverSensorsIfNeeded(now);
    return;
  }

  _lastResult.coordinates[0] = _magRawG[0];
  _lastResult.coordinates[1] = _magRawG[1];
  _lastResult.coordinates[2] = _magRawG[2];

  static float magRealGauss[3];
  static float accRealG[3];
  static float caliMag[3];
  static float caliOri[3];
  mapAxes(kALayout, _accRawG, accRealG, false);
  mapAxes(kMLayout, _magRawG, magRealGauss, true);
  MainAlgorithmProcess(accRealG, magRealGauss, 1);
  memset(caliMag, 0, sizeof(caliMag));
  GetCalMag(caliMag);
  memset(caliOri, 0 , sizeof(caliOri));
  GetCalOri(accRealG, caliMag, caliOri);
  _lastResult.fQuality = (float)GetMagAccuracy();

  _lastResult.fAzimuth = caliOri[0];
  _lastResult.fPitch = caliOri[1];
  _lastResult.fRoll = caliOri[2];

  _lastResult.fSimpleHeadingDegrees = applyHeadingAlignment(compute2DHeadingDegrees(caliMag));
  updateHeadingFilter(applyHeadingAlignment(wrapAngle360(caliOri[0] + kDeclinationDeg)));

  updateCalibrationIfChanged();

  {
    heltec::meshcore::biz::CompassUi ui{};
    if (fillUiSnapshot(ui) && shouldNotifyUi(ui, now)) {
      heltec::meshcore::ui::AppStateEvent ev{};
      ev.type = heltec::meshcore::ui::AppStateEventType::CompassChanged;
      ev.compass.heading_valid = ui.heading_valid;
      ev.compass.heading_deg = ui.heading_deg;
      ev.compass.quality = ui.quality;
      heltec::meshcore::ui::app_state_notifier().notify(ev);
    }
  }
}

bool ICMCompassProvider::probeI2CAddress(TwoWire& wire, uint8_t address) {
  wire.beginTransmission(address);
  return (wire.endTransmission() == 0);
}

bool ICMCompassProvider::detectIcm42607Address(TwoWire& wire, uint8_t& outAddress) {
  if (probeI2CAddress(wire, ICM42607_I2C_ADDR_LOW)) {
    outAddress = ICM42607_I2C_ADDR_LOW;
    return true;
  }
  if (probeI2CAddress(wire, ICM42607_I2C_ADDR_HIGH)) {
    outAddress = ICM42607_I2C_ADDR_HIGH;
    return true;
  }
  return false;
}

bool ICMCompassProvider::initAccelerometer() {
  TwoWire* wire = &Wire;
  uint8_t addr = ICM42607_I2C_ADDR_LOW;
  if (!detectIcm42607Address(Wire, addr)) {
#if defined(ENV_PIN_SDA) && ENV_PIN_SDA >=0 && defined(ENV_PIN_SCL) && ENV_PIN_SCL >=0
    wire = &Wire1;
    if (!detectIcm42607Address(Wire1, addr)) {
      MESH_DEBUG_PRINTLN("ICM42607 not found on 0x68/0x69");
      return false;
    }
#else
    MESH_DEBUG_PRINTLN("ICM42607 not found on 0x68/0x69");
    return false;
#endif
  }

  _wire = wire;
  _icmAddress = addr;

  uint8_t who = 0x00;
  if (!readIcmRegisters(ICM42607_REG_WHO_AM_I, &who, 1)) {
    MESH_DEBUG_PRINTLN("ICM42607 WHO_AM_I read failed");
    return false;
  }
  if (who != ICM42607_WHO_AM_I_VALUE) {
    MESH_DEBUG_PRINTLN("ICM42607 unexpected WHO_AM_I=0x%02x", who);
    return false;
  }

  return configureAccelerometer();
}

bool ICMCompassProvider::configureAccelerometer() {
  const uint8_t accel_config0 = (uint8_t)(ICM42607_ACCEL_CONFIG0_FS_2G | ICM42607_ACCEL_CONFIG0_ODR_100HZ);
  if (!writeIcmRegister(ICM42607_REG_ACCEL_CONFIG0, accel_config0)) {
    MESH_DEBUG_PRINTLN("ICM42607 ACCEL_CONFIG0 write failed");
    return false;
  }
  if (!writeIcmRegister(ICM42607_REG_ACCEL_CONFIG1, ICM42607_ACCEL_CONFIG1_UI_FILT_34HZ)) {
    MESH_DEBUG_PRINTLN("ICM42607 ACCEL_CONFIG1 write failed");
    return false;
  }
  if (!writeIcmRegister(ICM42607_REG_PWR_MGMT0, ICM42607_PWR_MGMT0_ACCEL_LN)) {
    MESH_DEBUG_PRINTLN("ICM42607 PWR_MGMT0 write failed");
    return false;
  }
  delay(10);
  return true;
}

bool ICMCompassProvider::initMagnetometer() {
  _mag.setErrorCallback(&ICMCompassProvider::handleError);
  if (!_mag.begin()) return false;
  if (!_mag.softReset()) return false;
  return configureMagnetometer();
}

bool ICMCompassProvider::configureMagnetometer() {
  bool ok = _mag.setFilterBandwidth(100);
  ok = _mag.enableAutomaticSetReset() && ok;
  ok = _mag.performSetOperation() && ok;
  return ok;
}

bool ICMCompassProvider::resetAccelerometer() {
  if (!_wire) return false;
  if (!writeIcmRegister(ICM42607_REG_DEVICE_CONFIG, ICM42607_DEVICE_CONFIG_SOFT_RESET)) {
    MESH_DEBUG_PRINTLN("ICM42607 soft reset write failed");
    return false;
  }
  delay(10);

  uint8_t who = 0x00;
  if (!readIcmRegisters(ICM42607_REG_WHO_AM_I, &who, 1)) {
    MESH_DEBUG_PRINTLN("ICM42607 WHO_AM_I unreadable after reset");
    return false;
  }
  if (who != ICM42607_WHO_AM_I_VALUE) {
    MESH_DEBUG_PRINTLN("ICM42607 WHO_AM_I after reset=0x%02x", who);
    return false;
  }
  return configureAccelerometer();
}

bool ICMCompassProvider::resetMagnetometer() {
  return _mag.softReset() && configureMagnetometer();
}

bool ICMCompassProvider::reconnectAccelerometer() {
  MESH_DEBUG_PRINTLN("ICM42607 reconnecting");
  return initAccelerometer();
}

bool ICMCompassProvider::reconnectMagnetometer() {
  MESH_DEBUG_PRINTLN("MMC5983MA reconnecting");
  return initMagnetometer();
}

bool ICMCompassProvider::reconnectSensorsIfNeeded(uint32_t now) {
  if (_ready) return true;

  if (_last_recovery_ms != 0 &&
      (uint32_t)(now - _last_recovery_ms) < (uint32_t)ICM_COMPASS_RECOVERY_INTERVAL_MS) {
    return false;
  }

  _last_recovery_ms = now;
  _recovery_count++;
  MESH_DEBUG_PRINTLN("Compass sensors reconnect #%lu", (unsigned long)_recovery_count);

  const bool ok = initializeSensors();
  MESH_DEBUG_PRINTLN("Compass sensors reconnect %s", ok ? "complete" : "failed");
  return ok;
}

bool ICMCompassProvider::recoverSensorsIfNeeded(uint32_t now) {
  const bool recover_accel =
      _accel_health.consecutive_failures >= (uint32_t)ICM_COMPASS_ACCEL_FAILURE_THRESHOLD;
  const bool recover_mag =
      _mag_health.consecutive_failures >= (uint32_t)ICM_COMPASS_MAG_FAILURE_THRESHOLD;
  if (!recover_accel && !recover_mag) return false;

  if (_last_recovery_ms != 0 &&
      (uint32_t)(now - _last_recovery_ms) < (uint32_t)ICM_COMPASS_RECOVERY_INTERVAL_MS) {
    return false;
  }

  _last_recovery_ms = now;
  _recovery_count++;
  MESH_DEBUG_PRINTLN("Compass sensor recovery #%lu accel=%d mag=%d",
                     (unsigned long)_recovery_count,
                     recover_accel ? 1 : 0, recover_mag ? 1 : 0);

  bool ok = true;
  if (recover_accel) {
    bool accel_ok = resetAccelerometer();
    if (!accel_ok) accel_ok = reconnectAccelerometer();
    if (accel_ok) _accel_health.clearConsecutiveFailures();
    ok = accel_ok && ok;
  }
  if (recover_mag) {
    bool mag_ok = resetMagnetometer();
    if (!mag_ok) mag_ok = reconnectMagnetometer();
    if (mag_ok) _mag_health.clearConsecutiveFailures();
    ok = mag_ok && ok;
  }

  if (ok) {
    ok = resetCompassAlgorithm();
  }

  if (ok) {
    MESH_DEBUG_PRINTLN("Compass sensor recovery complete");
  } else {
    _ready = false;
    MESH_DEBUG_PRINTLN("Compass sensor recovery failed");
  }
  return ok;
}

bool ICMCompassProvider::writeIcmRegister(uint8_t reg, uint8_t value) {
  if (!_wire) return false;
  _wire->beginTransmission(_icmAddress);
  _wire->write(reg);
  _wire->write(value);
  return (_wire->endTransmission() == 0);
}

bool ICMCompassProvider::readIcmRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
  if (!_wire || !buffer || length == 0) return false;
  _wire->beginTransmission(_icmAddress);
  _wire->write(reg);
  const int tx_err = _wire->endTransmission(false);
  if (tx_err != 0) return false;
  const size_t received = _wire->requestFrom((int)_icmAddress, (int)length);
  if (received != length) return false;
  for (size_t i = 0; i < length; i++) {
    if (!_wire->available()) return false;
    buffer[i] = (uint8_t)_wire->read();
  }
  return true;
}

bool ICMCompassProvider::readAccelerometerG(float* acc_g) {
  if (!acc_g || !_wire) return false;

  static uint8_t raw_bytes[6] = {0, 0, 0, 0, 0, 0};
  if (!readIcmRegisters(ICM42607_REG_ACCEL_DATA_X1, raw_bytes, sizeof(raw_bytes))) return false;

  const int16_t raw_x = (int16_t)(((uint16_t)raw_bytes[0] << 8) | raw_bytes[1]);
  const int16_t raw_y = (int16_t)(((uint16_t)raw_bytes[2] << 8) | raw_bytes[3]);
  const int16_t raw_z = (int16_t)(((uint16_t)raw_bytes[4] << 8) | raw_bytes[5]);
  acc_g[0] = (float)raw_x / ICM42607_COUNTS_PER_G;
  acc_g[1] = (float)raw_y / ICM42607_COUNTS_PER_G;
  acc_g[2] = (float)raw_z / ICM42607_COUNTS_PER_G;
  return true;
}

bool ICMCompassProvider::readMagnetometerG(float* mag_g) {
  if (!mag_g) return false;

  uint32_t raw_x = 0;
  uint32_t raw_y = 0;
  uint32_t raw_z = 0;
  if (!_mag.getMeasurementXYZ(&raw_x, &raw_y, &raw_z)) return false;

  mag_g[0] = ((float)raw_x - MMC5983_ZERO_OFFSET) / MMC5983_COUNTS_PER_GAUSS;
  mag_g[1] = ((float)raw_y - MMC5983_ZERO_OFFSET) / MMC5983_COUNTS_PER_GAUSS;
  mag_g[2] = ((float)raw_z - MMC5983_ZERO_OFFSET) / MMC5983_COUNTS_PER_GAUSS;
  return true;
}

bool ICMCompassProvider::resetCompassAlgorithm() {
  _headingFilterReady = false;
  if (InitialAlgorithm(_magSmm, _magHmm) != 1) return false;
  memcpy(_lastAppliedMagHmm, _magHmm, sizeof(_magHmm));
  return true;
}

void ICMCompassProvider::updateCalibrationIfChanged() {
  static float newMagPara[4];
  if (GetCalPara(newMagPara) != 1) return;
  if (magParamsEqual4(newMagPara, _lastAppliedMagHmm)) return;
  memcpy(_magHmm, newMagPara, sizeof(_magHmm));
  InitialAlgorithm(_magSmm, _magHmm);
  memcpy(_lastAppliedMagHmm, _magHmm, sizeof(_magHmm));
}

float ICMCompassProvider::wrapAngle360(float angle) {
  while (angle < 0.0f) angle += 360.0f;
  while (angle >= 360.0f) angle -= 360.0f;
  return angle;
}

void ICMCompassProvider::handleError(SF_MMC5983MA_ERROR errorCode) {
  MESH_DEBUG_PRINTLN("MMC5983 error code:%d", errorCode);
}

float ICMCompassProvider::compute2DHeadingDegrees(const float* mag_gauss) {
  if (!mag_gauss) return 0.0f;
  float heading = atan2f(mag_gauss[0], -mag_gauss[1]) * 180.0f / PI;
  heading += 180.0f;
  heading += kDeclinationDeg;
  return wrapAngle360(heading);
}

float ICMCompassProvider::applyHeadingAlignment(float headingDegrees) {
  return wrapAngle360(headingDegrees + kHeadingAlignmentDeg);
}

void ICMCompassProvider::updateHeadingFilter(float headingDegrees) {
  if (!_headingFilterReady) {
    _lastResult.fFilteredHeadingDegrees = headingDegrees;
    _headingFilterReady = true;
    return;
  }
  float delta = headingDegrees - _lastResult.fFilteredHeadingDegrees;
  if (delta > 180.0f) {
    delta -= 360.0f;
  }
  if (delta < -180.0f) {
    delta += 360.0f;
  }
  _lastResult.fFilteredHeadingDegrees =
      wrapAngle360(_lastResult.fFilteredHeadingDegrees +
                   ((float)COMPASS_HEADING_FILTER_ALPHA * delta));
}

void ICMCompassProvider::mapAxes(int layout, const float* in, float* out, bool invert_default_z) {
  if (!in || !out) return;
  switch (layout) {
    case 0:
      out[0] = in[0];
      out[1] = in[1];
      out[2] = in[2];
      break;
    case 1:
      out[0] = -in[1];
      out[1] = in[0];
      out[2] = in[2];
      break;
    case 2:
      out[0] = -in[0];
      out[1] = -in[1];
      out[2] = in[2];
      break;
    case 3:
      out[0] = in[1];
      out[1] = -in[0];
      out[2] = in[2];
      break;
    case 4:
      out[0] = in[1];
      out[1] = in[0];
      out[2] = -in[2];
      break;
    case 5:
      out[0] = -in[0];
      out[1] = in[1];
      out[2] = -in[2];
      break;
    case 6:
      out[0] = -in[1];
      out[1] = -in[0];
      out[2] = -in[2];
      break;
    case 7:
      out[0] = in[0];
      out[1] = -in[1];
      out[2] = -in[2];
      break;
    default:
      out[0] = in[0];
      out[1] = in[1];
      out[2] = invert_default_z ? -in[2] : in[2];
      break;
  }
}

#endif  // ENV_INCLUDE_COMPASS
