#pragma once

#include <stdint.h>

#include "MeshCore.h"

namespace heltec::meshcore::power {

enum class SystemPowerState : uint8_t {
  Running = 0,
  ShuttingDown,
  Rebooting,
  Fault,
};

enum class PowerFault : uint8_t {
  None = 0,
  DisplayApplyFailed,
  GpsApplyFailed,
  PowerOffReturned,
  RebootReturned,
};

enum class GpsDemand : uint16_t {
  Track = 1U << 0,
  LocationContinuous = 1U << 1,
  LocationAcquire = 1U << 2,
  GpsScreen = 1U << 3,
  MapScreen = 1U << 4,
  FindFriend = 1U << 5,
  ExternalPower = 1U << 6,
};

enum class PowerChange : uint16_t {
  Battery = 1U << 0,
  Source = 1U << 1,
  Display = 1U << 2,
  Gps = 1U << 3,
  System = 1U << 4,
  Fault = 1U << 5,
};

using PowerChangeMask = uint16_t;

constexpr uint16_t mask(GpsDemand demand) {
  return static_cast<uint16_t>(demand);
}

constexpr PowerChangeMask mask(PowerChange change) {
  return static_cast<PowerChangeMask>(change);
}

constexpr bool hasChange(PowerChangeMask changes, PowerChange change) {
  return (changes & mask(change)) != 0;
}

struct PowerConfig {
  uint32_t battery_poll_ms = 10000;
  uint32_t source_poll_ms = 1000;
  uint32_t gps_retry_ms = 1000;
  uint32_t gps_screen_off_grace_ms = 30000;
  uint32_t shutdown_feedback_timeout_ms = 2500;
  uint16_t battery_min_mv = 3000;
  uint16_t battery_max_mv = 4200;
  bool display_available = true;
};

struct PowerSnapshot {
  uint16_t battery_mv = 0;
  uint8_t battery_percent = 0;
  mesh::PowerSource source = mesh::PowerSource::Unknown;
  bool display_on = false;
  bool gps_available = false;
  bool gps_allowed = false;
  bool gps_powered = false;
  uint16_t gps_demands = 0;
  SystemPowerState system_state = SystemPowerState::Running;
  PowerFault fault = PowerFault::None;
};

class PowerRuntimePort {
 public:
  virtual ~PowerRuntimePort() = default;

  virtual mesh::PowerSource getPowerSource() const = 0;
  virtual bool isDisplayOn() const = 0;
  virtual void setDisplayOn(bool on) = 0;

  virtual bool isGpsAvailable() const = 0;
  virtual bool isGpsPowered() const = 0;
  virtual bool setGpsPowered(bool on) = 0;

  virtual void startShutdownFeedback() = 0;
  virtual bool shutdownFeedbackDone() = 0;
  virtual void powerOffRadio() = 0;
};

class PowerListener {
 public:
  virtual ~PowerListener() = default;
  virtual void onPowerChanged(PowerChangeMask changes,
                              const PowerSnapshot& snapshot) = 0;
};

PowerConfig defaultPowerConfig();

class PowerMgr {
 public:
  PowerMgr(mesh::MainBoard& board, PowerRuntimePort& port)
      : _board(board), _port(port) {}

  void setListener(PowerListener* listener) { _listener = listener; }
  void begin(uint32_t now_ms, const PowerConfig& config = defaultPowerConfig());
  void loop(uint32_t now_ms);

  const PowerSnapshot& snapshot() const { return _snapshot; }
  uint16_t batteryMilliVolts() const { return _snapshot.battery_mv; }

  void notifyUserActivity(uint32_t now_ms);
  void setDisplayAutoOffMs(uint32_t timeout_ms, uint32_t now_ms);
  void setDisplayTimeoutInhibited(uint16_t reason, bool inhibited,
                                  uint32_t now_ms);
  bool setDisplayOn(bool on, uint32_t now_ms);

  void setGpsAllowed(bool allowed);
  void setGpsDemand(GpsDemand demand, bool active);
  bool reconcileGpsPower(uint32_t now_ms);

  void requestPowerOff(uint32_t now_ms);
  void requestReboot(uint32_t delay_ms, uint32_t now_ms);

 private:
  static bool deadlineReached(uint32_t now_ms, uint32_t deadline_ms);
  uint8_t batteryPercent(uint16_t mv) const;
  uint16_t effectiveGpsDemands(uint32_t now_ms) const;
  void pollBattery(uint32_t now_ms, bool force = false);
  void pollPowerSource(uint32_t now_ms, bool force = false);
  void processDisplayTimeout(uint32_t now_ms);
  void processSystemTransition(uint32_t now_ms);
  void finishPowerOff();
  void finishReboot();
  void setFault(PowerFault fault);
  void notify(PowerChangeMask changes);

  mesh::MainBoard& _board;
  PowerRuntimePort& _port;
  PowerListener* _listener = nullptr;
  PowerConfig _config{};
  PowerSnapshot _snapshot{};
  uint16_t _requested_gps_demands = 0;
  uint16_t _display_inhibit_mask = 0;
  uint32_t _display_auto_off_ms = 0;
  uint32_t _display_last_activity_ms = 0;
  uint32_t _display_off_since_ms = 0;
  uint32_t _battery_last_poll_ms = 0;
  uint32_t _source_last_poll_ms = 0;
  uint32_t _gps_retry_due_ms = 0;
  uint32_t _system_transition_due_ms = 0;
  bool _battery_polled = false;
  bool _source_polled = false;
  bool _display_activity_valid = false;
  bool _gps_policy_changed = false;
  bool _begun = false;
};

}  // namespace heltec::meshcore::power
