#include "HeltecEnvironmentSensorManager.h"
#if ENV_INCLUDE_COMPASS
#include "CompassProvider.h"
#endif
#if ENV_INCLUDE_GPS
#include <stdio.h>
#endif
#if ENV_INCLUDE_GPS && defined(GPS_UC6580) && defined(GPS_UC6580_CONFIGURE) && GPS_UC6580_CONFIGURE
#include "Uc6580Config.h"
#endif
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
#include "heltec/ui/core/app_state_notifier.hpp"
#endif

#if defined(ENV_PIN_SDA) && ENV_PIN_SDA >= 0 && defined(ENV_PIN_SCL) && ENV_PIN_SCL >= 0
#define TELEM_WIRE (&Wire1)
#else
#define TELEM_WIRE (&Wire)
#endif

#ifdef ENV_INCLUDE_BME680
#ifndef TELEM_BME680_ADDRESS
#define TELEM_BME680_ADDRESS 0x76
#endif
#define TELEM_BME680_SEALEVELPRESSURE_HPA (1013.25)
#include <Adafruit_BME680.h>
static Adafruit_BME680 BME680;
#endif

#ifdef ENV_INCLUDE_BMP085
#define TELEM_BMP085_SEALEVELPRESSURE_HPA (1013.25)
#include <Adafruit_BMP085.h>
static Adafruit_BMP085 BMP085;
#endif

#if ENV_INCLUDE_AHTX0
#define TELEM_AHTX_ADDRESS      0x38      // AHT10, AHT20 temperature and humidity sensor I2C address
#include <Adafruit_AHTX0.h>
static Adafruit_AHTX0 AHTX0;
#endif

#if ENV_INCLUDE_BME280
#ifndef TELEM_BME280_ADDRESS
#define TELEM_BME280_ADDRESS    0x76      // BME280 environmental sensor I2C address
#endif
#define TELEM_BME280_SEALEVELPRESSURE_HPA (1013.25)    // Athmospheric pressure at sea level
#include <Adafruit_BME280.h>
static Adafruit_BME280 BME280;
#endif

#if ENV_INCLUDE_BMP280
#ifndef TELEM_BMP280_ADDRESS
#define TELEM_BMP280_ADDRESS    0x76      // BMP280 environmental sensor I2C address
#endif
#define TELEM_BMP280_SEALEVELPRESSURE_HPA (1013.25)    // Athmospheric pressure at sea level
#include <Adafruit_BMP280.h>
static Adafruit_BMP280 BMP280(TELEM_WIRE);
#endif

#if ENV_INCLUDE_SHTC3
#include <Adafruit_SHTC3.h>
static Adafruit_SHTC3 SHTC3;
#endif

#if ENV_INCLUDE_SHT4X
#define TELEM_SHT4X_ADDRESS 0x44  //0x44 - 0x46
#include <SensirionI2cSht4x.h>
static SensirionI2cSht4x SHT4X;
#endif

#if ENV_INCLUDE_LPS22HB
#include <Arduino_LPS22HB.h>
LPS22HBClass LPS22HB(*TELEM_WIRE);
#endif

#if ENV_INCLUDE_INA3221
#define TELEM_INA3221_ADDRESS   0x42      // INA3221 3 channel current sensor I2C address
#define TELEM_INA3221_SHUNT_VALUE 0.100 // most variants will have a 0.1 ohm shunts
#define TELEM_INA3221_NUM_CHANNELS 3
#include <Adafruit_INA3221.h>
static Adafruit_INA3221 INA3221;
#endif

#if ENV_INCLUDE_INA219
#define TELEM_INA219_ADDRESS    0x40      // INA219 single channel current sensor I2C address
#include <Adafruit_INA219.h>
static Adafruit_INA219 INA219(TELEM_INA219_ADDRESS);
#endif

#if ENV_INCLUDE_INA260
#define TELEM_INA260_ADDRESS    0x41      // INA260 single channel current sensor I2C address
#include <Adafruit_INA260.h>
static Adafruit_INA260 INA260;
#endif

#if ENV_INCLUDE_INA226
#define TELEM_INA226_ADDRESS    0x44
#define TELEM_INA226_SHUNT_VALUE 0.100
#define TELEM_INA226_MAX_AMP 0.8
#include <INA226.h>
static INA226 INA226(TELEM_INA226_ADDRESS, TELEM_WIRE);
#endif

#if ENV_INCLUDE_MLX90614
#define TELEM_MLX90614_ADDRESS 0x5A      // MLX90614 IR temperature sensor I2C address
#include <Adafruit_MLX90614.h>
static Adafruit_MLX90614 MLX90614;
#endif

#if ENV_INCLUDE_VL53L0X
#define TELEM_VL53L0X_ADDRESS 0x29      // VL53L0X time-of-flight distance sensor I2C address
#include <Adafruit_VL53L0X.h>
static Adafruit_VL53L0X VL53L0X;
#endif

#if ENV_INCLUDE_GPS && defined(RAK_BOARD) && !defined(RAK_WISMESH_TAG)
#define RAK_WISBLOCK_GPS
#endif

#ifdef RAK_WISBLOCK_GPS
static uint32_t gpsResetPin = 0;
static bool i2cGPSFlag = false;
static bool serialGPSFlag = false;
#define TELEM_RAK12500_ADDRESS   0x42     //RAK12500 Ublox GPS via i2c
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
static SFE_UBLOX_GNSS ublox_GNSS;

class RAK12500LocationProvider : public LocationProvider {
  long _lat = 0;
  long _lng = 0;
  long _alt = 0;
  int _sats = 0;
  long _epoch = 0;
  bool _fix = false;
public:
  long getLatitude() override { return _lat; }
  long getLongitude() override { return _lng; }
  long getAltitude() override { return _alt; }
  long satellitesCount() override { return _sats; }
  bool isValid() override { return _fix; }
  long getTimestamp() override { return _epoch; }
  void sendSentence(const char * sentence) override { }
  void reset() override { }
  void begin() override { }
  void stop() override { }
  void loop() override {
    if (ublox_GNSS.getGnssFixOk(8)) {
      _fix = true;
      _lat = ublox_GNSS.getLatitude(2) / 10;
      _lng = ublox_GNSS.getLongitude(2) / 10;
      _alt = ublox_GNSS.getAltitude(2);
      _sats = ublox_GNSS.getSIV(2);
    } else {
      _fix = false;
    }
    _epoch = ublox_GNSS.getUnixEpoch(2);
  }
  bool isEnabled() override { return true; }
};

static RAK12500LocationProvider RAK12500_provider;
#endif

bool HeltecEnvironmentSensorManager::begin() {
#if defined(MESH_DEBUG) && MESH_DEBUG && defined(ARDUINO_ARCH_ESP32)
  MESH_DEBUG_PRINTLN("[env] sensor probe: TELEM_WIRE=Wire timeout=%lu ms t=%lu",
                     (unsigned long)Wire.getTimeOut(), (unsigned long)millis());
#endif
  #if ENV_INCLUDE_GPS
  #ifdef RAK_WISBLOCK_GPS
  rakGPSInit();   //probe base board/sockets for GPS
  #else
  initBasicGPS();
  #endif
  #endif
  #if ENV_INCLUDE_AHTX0
  if (AHTX0.begin(TELEM_WIRE, 0, TELEM_AHTX_ADDRESS)) {
    MESH_DEBUG_PRINTLN("Found AHT10/AHT20 at address: %02X", TELEM_AHTX_ADDRESS);
    AHTX0_initialized = true;
  } else {
    AHTX0_initialized = false;
    MESH_DEBUG_PRINTLN("AHT10/AHT20 was not found at I2C address %02X", TELEM_AHTX_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BME680
  if (BME680.begin(TELEM_BME680_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found BME680 at address: %02X", TELEM_BME680_ADDRESS);
    BME680_initialized = true;
  } else {
    BME680_initialized = false;
    MESH_DEBUG_PRINTLN("BME680 was not found at I2C address %02X", TELEM_BME680_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BME280
  if (BME280.begin(TELEM_BME280_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found BME280 at address: %02X", TELEM_BME280_ADDRESS);
    MESH_DEBUG_PRINTLN("BME sensor ID: %02X", BME280.sensorID());
    // Reduce self-heating: single-shot conversions, light oversampling, long standby.
    BME280.setSampling(Adafruit_BME280::MODE_FORCED,
                       Adafruit_BME280::SAMPLING_X1,   // temperature
                       Adafruit_BME280::SAMPLING_X1,   // pressure
                       Adafruit_BME280::SAMPLING_X1,   // humidity
                       Adafruit_BME280::FILTER_OFF,
                       Adafruit_BME280::STANDBY_MS_1000);
    BME280_initialized = true;
  } else {
    BME280_initialized = false;
    MESH_DEBUG_PRINTLN("BME280 was not found at I2C address %02X", TELEM_BME280_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BMP280
  if (BMP280.begin(TELEM_BMP280_ADDRESS)) {
    MESH_DEBUG_PRINTLN("Found BMP280 at address: %02X", TELEM_BMP280_ADDRESS);
    MESH_DEBUG_PRINTLN("BMP sensor ID: %02X", BMP280.sensorID());
    BMP280_initialized = true;
  } else {
    BMP280_initialized = false;
    MESH_DEBUG_PRINTLN("BMP280 was not found at I2C address %02X", TELEM_BMP280_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_SHTC3
  if (SHTC3.begin(TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found sensor: SHTC3");
    SHTC3_initialized = true;
  } else {
    SHTC3_initialized = false;
    MESH_DEBUG_PRINTLN("SHTC3 was not found at I2C address %02X", 0x70);
  }
  #endif


  #if ENV_INCLUDE_SHT4X
  SHT4X.begin(*TELEM_WIRE, TELEM_SHT4X_ADDRESS);
  uint32_t serialNumber = 0;
  if (SHT4X.serialNumber(serialNumber) == 0) {
    MESH_DEBUG_PRINTLN("Found SHT4X at address: %02X", TELEM_SHT4X_ADDRESS);
    SHT4X_initialized = true;
  } else {
    SHT4X_initialized = false;
    MESH_DEBUG_PRINTLN("SHT4X was not found at I2C address %02X", TELEM_SHT4X_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_LPS22HB
#if defined(MESH_DEBUG) && MESH_DEBUG
  MESH_DEBUG_PRINTLN("[env] probing LPS22HB @0x5C t=%lu", (unsigned long)millis());
#endif
  if (LPS22HB.begin()) {
    MESH_DEBUG_PRINTLN("Found sensor: LPS22HB");
    LPS22HB_initialized = true;
  } else {
    LPS22HB_initialized = false;
    MESH_DEBUG_PRINTLN("LPS22HB was not found at I2C address %02X", 0x5C);
  }
  #endif

  #if ENV_INCLUDE_INA3221
  if (INA3221.begin(TELEM_INA3221_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found INA3221 at address: %02X", TELEM_INA3221_ADDRESS);
    MESH_DEBUG_PRINTLN("%04X %04X", INA3221.getDieID(), INA3221.getManufacturerID());

    for(int i = 0; i < 3; i++) {
      INA3221.setShuntResistance(i, TELEM_INA3221_SHUNT_VALUE);
    }
    INA3221_initialized = true;
  } else {
    INA3221_initialized = false;
    MESH_DEBUG_PRINTLN("INA3221 was not found at I2C address %02X", TELEM_INA3221_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_INA219
  if (INA219.begin(TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found INA219 at address: %02X", TELEM_INA219_ADDRESS);
    INA219_initialized = true;
  } else {
    INA219_initialized = false;
    MESH_DEBUG_PRINTLN("INA219 was not found at I2C address %02X", TELEM_INA219_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_INA260
  if (INA260.begin(TELEM_INA260_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found INA260 at address: %02X", TELEM_INA260_ADDRESS);
    INA260_initialized = true;
  } else {
    INA260_initialized = false;
    MESH_DEBUG_PRINTLN("INA260 was not found at I2C address %02X", TELEM_INA260_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_INA226
  if (INA226.begin()) {
    MESH_DEBUG_PRINTLN("Found INA226 at address: %02X", TELEM_INA226_ADDRESS);
    INA226.setMaxCurrentShunt(TELEM_INA226_MAX_AMP, TELEM_INA226_SHUNT_VALUE);
    INA226_initialized = true;
  } else {
    INA226_initialized = false;
    MESH_DEBUG_PRINTLN("INA226 was not found at I2C address %02X", TELEM_INA226_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_MLX90614
  if (MLX90614.begin(TELEM_MLX90614_ADDRESS, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found MLX90614 at address: %02X", TELEM_MLX90614_ADDRESS);
    MLX90614_initialized = true;
  } else {
    MLX90614_initialized = false;
    MESH_DEBUG_PRINTLN("MLX90614 was not found at I2C address %02X", TELEM_MLX90614_ADDRESS);
  }
  #endif
  #if ENV_INCLUDE_VL53L0X
#if defined(MESH_DEBUG) && MESH_DEBUG
  MESH_DEBUG_PRINTLN("[env] probing VL53L0X @0x%02X t=%lu", TELEM_VL53L0X_ADDRESS,
                     (unsigned long)millis());
#endif
  if (VL53L0X.begin(TELEM_VL53L0X_ADDRESS, false, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found VL53L0X at address: %02X", TELEM_VL53L0X_ADDRESS);
    VL53L0X_initialized = true;
  } else {
    VL53L0X_initialized = false;
    MESH_DEBUG_PRINTLN("VL53L0X was not found at I2C address %02X", TELEM_VL53L0X_ADDRESS);
  }
  #endif

  #if ENV_INCLUDE_BMP085
  // First argument is  MODE (aka oversampling)
  // choose ULTRALOWPOWER
  if (BMP085.begin(0, TELEM_WIRE)) {
    MESH_DEBUG_PRINTLN("Found sensor BMP085");
    BMP085_initialized = true;
  } else {
    BMP085_initialized = false;
    MESH_DEBUG_PRINTLN("BMP085 was not found at I2C address %02X", 0x77);
  }
  #endif

  #if ENV_INCLUDE_COMPASS
  if (_compass) _compass->begin();
  #endif

// #if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH && defined(ENV_PIN_SDA) && ENV_PIN_SDA && \
//     defined(ENV_PIN_SCL) && ENV_PIN_SCL && defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL) && \
//     (ENV_PIN_SDA) == (PIN_BOARD_SDA) && (ENV_PIN_SCL) == (PIN_BOARD_SCL)
//   heltec::meshcore::dal::touch_port::reinitAfterI2cProbe();
// #endif

  return true;
}

bool HeltecEnvironmentSensorManager::querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) {
  next_available_channel = TELEM_CHANNEL_SELF + 1;

  if (requester_permissions & TELEM_PERM_LOCATION && gps_active) {
    telemetry.addGPS(TELEM_CHANNEL_SELF, node_lat, node_lon, node_altitude); // allow lat/lon via telemetry even if no GPS is detected
  }

  if (requester_permissions & TELEM_PERM_ENVIRONMENT) {

    #if ENV_INCLUDE_AHTX0
    if (AHTX0_initialized) {
      sensors_event_t humidity, temp;
      AHTX0.getEvent(&humidity, &temp);
      telemetry.addTemperature(TELEM_CHANNEL_SELF, temp.temperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, humidity.relative_humidity);
    }
    #endif

    #if ENV_INCLUDE_BME680
    if (BME680_initialized) {
      if (BME680.performReading()) {
        telemetry.addTemperature(TELEM_CHANNEL_SELF, BME680.temperature);
        telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, BME680.humidity);
        telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BME680.pressure / 100);
        telemetry.addAltitude(TELEM_CHANNEL_SELF, 44330.0 * (1.0 - pow((BME680.pressure / 100) / TELEM_BME680_SEALEVELPRESSURE_HPA, 0.1903)));
        telemetry.addAnalogInput(next_available_channel, BME680.gas_resistance);
        next_available_channel++;
      }
    }
    #endif

    #if ENV_INCLUDE_BME280
    if (BME280_initialized) {
      if (BME280.takeForcedMeasurement()) {  // trigger a fresh reading in forced mode
        telemetry.addTemperature(TELEM_CHANNEL_SELF, BME280.readTemperature());
        telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, BME280.readHumidity());
        telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BME280.readPressure()/100);
        telemetry.addAltitude(TELEM_CHANNEL_SELF, BME280.readAltitude(TELEM_BME280_SEALEVELPRESSURE_HPA));
      }
    }
    #endif

    #if ENV_INCLUDE_BMP280
    if (BMP280_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, BMP280.readTemperature());
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BMP280.readPressure()/100);
      telemetry.addAltitude(TELEM_CHANNEL_SELF, BMP280.readAltitude(TELEM_BMP280_SEALEVELPRESSURE_HPA));
    }
    #endif

    #if ENV_INCLUDE_SHTC3
    if (SHTC3_initialized) {
      sensors_event_t humidity, temp;
      SHTC3.getEvent(&humidity, &temp);

      telemetry.addTemperature(TELEM_CHANNEL_SELF, temp.temperature);
      telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, humidity.relative_humidity);
    }
    #endif

    #if ENV_INCLUDE_SHT4X
    if (SHT4X_initialized) {
      float sht4x_humidity, sht4x_temperature;
      int16_t sht4x_error;
      sht4x_error = SHT4X.measureLowestPrecision(sht4x_temperature, sht4x_humidity);
      if (sht4x_error == 0) {
        telemetry.addTemperature(TELEM_CHANNEL_SELF, sht4x_temperature);
        telemetry.addRelativeHumidity(TELEM_CHANNEL_SELF, sht4x_humidity);
      }
    }
    #endif

    #if ENV_INCLUDE_LPS22HB
    if (LPS22HB_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, LPS22HB.readTemperature());
      telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, LPS22HB.readPressure() * 10); // convert kPa to hPa
    }
    #endif

    #if ENV_INCLUDE_INA3221
    if (INA3221_initialized) {
      for(int i = 0; i < TELEM_INA3221_NUM_CHANNELS; i++) {
        // add only enabled INA3221 channels to telemetry
        if (INA3221.isChannelEnabled(i)) {
          float voltage = INA3221.getBusVoltage(i);
          float current = INA3221.getCurrentAmps(i);
          telemetry.addVoltage(next_available_channel, voltage);
          telemetry.addCurrent(next_available_channel, current);
          telemetry.addPower(next_available_channel, voltage * current);
          next_available_channel++;
        }
      }
    }
    #endif

    #if ENV_INCLUDE_INA219
    if (INA219_initialized) {
      telemetry.addVoltage(next_available_channel, INA219.getBusVoltage_V());
      telemetry.addCurrent(next_available_channel, INA219.getCurrent_mA() / 1000);
      telemetry.addPower(next_available_channel, INA219.getPower_mW() / 1000);
      next_available_channel++;
    }
    #endif

    #if ENV_INCLUDE_INA260
    if (INA260_initialized) {
      telemetry.addVoltage(next_available_channel, INA260.readBusVoltage() / 1000);
      telemetry.addCurrent(next_available_channel, INA260.readCurrent() / 1000);
      telemetry.addPower(next_available_channel, INA260.readPower() / 1000);
      next_available_channel++;
    }
    #endif

    #if ENV_INCLUDE_INA226
    if (INA226_initialized) {
      telemetry.addVoltage(next_available_channel, INA226.getBusVoltage());
      telemetry.addCurrent(next_available_channel, INA226.getCurrent_mA() / 1000.0);
      telemetry.addPower(next_available_channel, INA226.getPower_mW() / 1000.0);
      next_available_channel++;
    }
    #endif

    #if ENV_INCLUDE_MLX90614
    if (MLX90614_initialized) {
      telemetry.addTemperature(TELEM_CHANNEL_SELF, MLX90614.readObjectTempC());
      telemetry.addTemperature(TELEM_CHANNEL_SELF + 1, MLX90614.readAmbientTempC());
    }
    #endif

    #if ENV_INCLUDE_VL53L0X
    if (VL53L0X_initialized) {
      VL53L0X_RangingMeasurementData_t measure;
      VL53L0X.rangingTest(&measure, false); // pass in 'true' to get debug data
      if (measure.RangeStatus != 4) { // phase failures
        telemetry.addDistance(TELEM_CHANNEL_SELF, measure.RangeMilliMeter / 1000.0f); // convert mm to m
      } else {
        telemetry.addDistance(TELEM_CHANNEL_SELF, 0.0f); // no valid measurement
      }
    }
    #endif

    #if ENV_INCLUDE_BMP085
    if (BMP085_initialized) {
        telemetry.addTemperature(TELEM_CHANNEL_SELF, BMP085.readTemperature());
        telemetry.addBarometricPressure(TELEM_CHANNEL_SELF, BMP085.readPressure() / 100);
        telemetry.addAltitude(TELEM_CHANNEL_SELF, BMP085.readAltitude(TELEM_BMP085_SEALEVELPRESSURE_HPA * 100));
    }
    #endif

  }

  return true;
}


int HeltecEnvironmentSensorManager::getNumSettings() const {
  int settings = 0;
  #if ENV_INCLUDE_GPS
    if (gps_detected) settings++;  // only show GPS setting if GPS is detected
  #endif
  return settings;
}

const char* HeltecEnvironmentSensorManager::getSettingName(int i) const {
  int settings = 0;
  #if ENV_INCLUDE_GPS
    if (gps_detected && i == settings++) {
      return "gps";
    }
  #endif
  // convenient way to add params (needed for some tests)
//  if (i == settings++) return "param.2";
  return NULL;
}

const char* HeltecEnvironmentSensorManager::getSettingValue(int i) const {
  int settings = 0;
  #if ENV_INCLUDE_GPS
    if (gps_detected && i == settings++) {
      return gps_active ? "1" : "0";
    }
  #endif
  // convenient way to add params ...
//  if (i == settings++) return "2";
  return NULL;
}

bool HeltecEnvironmentSensorManager::setSettingValue(const char* name, const char* value) {
  #if ENV_INCLUDE_GPS
  if (strcmp(name, "gps") == 0) {
    if (!_location) return false;
    if (strcmp(value, "0") == 0) {
      stop_gps();
    } else {
      if (!gps_detected) gps_detected = true;
      start_gps();
    }
    return true;
  }
  if (strcmp(name, "gps_interval") == 0) {
    uint32_t interval_seconds = atoi(value);
    if (interval_seconds > 0) {
      gps_update_interval_sec = interval_seconds;
    } else {
      gps_update_interval_sec = GPS_NOTIFY_INTERVAL_SEC;
    }
    return true;
  }
  #endif
  return false;  // not supported
}

#if ENV_INCLUDE_GPS
namespace {

#if defined(GPS_UC6580) && defined(GPS_L76K)
#error "Select only one GPS module type: GPS_UC6580 or GPS_L76K"
#endif

static constexpr uint16_t kGpsNmeaRateHz = 1;
static constexpr uint16_t kGpsNmeaIntervalMs = 1000;

void initGpsSerial() {
  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);
  MESH_DEBUG_PRINTLN("Setting GPS pins to %d, %d", PIN_GPS_TX, PIN_GPS_RX);
  #ifdef GPS_BAUD_RATE
  Serial1.begin(GPS_BAUD_RATE);
  #else
  Serial1.begin(9600);
  #endif
}

void waitForGps(LocationProvider* location, uint16_t wait_ms) {
  if (!location || wait_ms == 0) return;

  const uint32_t started_at = millis();
  while ((uint32_t)(millis() - started_at) < wait_ms) {
    location->loop();
    delay(10);
  }
}

void sendGpsCommand(LocationProvider* location, const char* label, const char* sentence,
                    uint16_t settle_ms) {
  if (!location || !label || !sentence) return;
  MESH_DEBUG_PRINTLN("[gps] L76K %s TX", label);
  location->sendSentence(sentence);
  waitForGps(location, settle_ms);
  MESH_DEBUG_PRINTLN("[gps] L76K %s settled", label);
}

#if defined(GPS_L76K)
void configureL76K(LocationProvider* location) {
  if (!location || !location->isEnabled()) return;

  char interval_cmd[24] = {0};
  snprintf(interval_cmd, sizeof(interval_cmd), "$PCAS02,%u", (unsigned)kGpsNmeaIntervalMs);

  MESH_DEBUG_PRINTLN("[gps] configure L76K: GGA+RMC %uHz interval=%ums",
                     (unsigned)kGpsNmeaRateHz, (unsigned)kGpsNmeaIntervalMs);

  // V4 R8 TFT powers the L76K immediately before this call. The bounded GPS
  // poll keeps startup noise from monopolising the ESP32 task while the CASIC
  // command parser becomes ready.
  MESH_DEBUG_PRINTLN("[gps] L76K startup wait begin");
  waitForGps(location, 1200);
  MESH_DEBUG_PRINTLN("[gps] L76K startup wait done");

  // Only configure the requested update interval and sentence set. Constellation
  // and navigation-mode commands are intentionally left untouched.
  sendGpsCommand(location, "PCAS02", interval_cmd, 100);
  sendGpsCommand(location, "PCAS03", "$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0", 100);

  MESH_DEBUG_PRINTLN("[gps] L76K config sequence sent");
}
#endif

void configureGpsModule(LocationProvider* location) {
#if defined(GPS_UC6580) && defined(GPS_UC6580_CONFIGURE) && GPS_UC6580_CONFIGURE
  Uc6580Config::apply(location);
#elif defined(GPS_L76K)
  configureL76K(location);
#else
  (void)location;
#endif
}

}  // namespace

void HeltecEnvironmentSensorManager::initBasicGPS() {

  initGpsSerial();

  // Try to detect if GPS is physically connected to determine if we should expose the setting
  _location->begin();
  _location->reset();

  #ifndef PIN_GPS_EN
    MESH_DEBUG_PRINTLN("No GPS wake/reset pin found for this board. Continuing on...");
  #endif

#ifdef ENV_SKIP_GPS_DETECT
  gps_detected = true;
  #ifdef PERSISTANT_GPS
  gps_active = true;
  configureGpsModule(_location);
  MESH_DEBUG_PRINTLN("GPS detected (ENV_SKIP_GPS_DETECT)");
  return;
  #endif
  _location->stop();
  gps_active = false;
  MESH_DEBUG_PRINTLN("GPS detected (ENV_SKIP_GPS_DETECT)");
  return;
#endif

  // Give GPS time to power up; pump NMEA while waiting.
  const unsigned long detect_until = millis() + 3000;
  while ((long)(detect_until - millis()) > 0) {
    _location->loop();
    if (Serial1.available() > 0) break;
    delay(20);
  }

  // Consider GPS present if serial data arrived during detect window.
  gps_detected = (Serial1.available() > 0);

  if (gps_detected) {
    MESH_DEBUG_PRINTLN("GPS detected");
    #ifdef PERSISTANT_GPS
      configureGpsModule(_location);
      gps_active = true;
      return;
    #endif
  } else {
    MESH_DEBUG_PRINTLN("No GPS detected");
  }
  _location->stop();
  gps_active = false; //Set GPS visibility off until setting is changed
}

// gps code for rak might be moved to MicroNMEALoactionProvider
// or make a new location provider ...
#ifdef RAK_WISBLOCK_GPS
void HeltecEnvironmentSensorManager::rakGPSInit(){

  Serial1.setPins(PIN_GPS_TX, PIN_GPS_RX);

  #ifdef GPS_BAUD_RATE
  Serial1.begin(GPS_BAUD_RATE);
  #else
  Serial1.begin(9600);
  #endif

  //search for the correct IO standby pin depending on socket used
  if(gpsIsAwake(WB_IO2)){
  //  MESH_DEBUG_PRINTLN("RAK base board is RAK19007/10");
  //  MESH_DEBUG_PRINTLN("GPS is installed on Socket A");
  }
  else if(gpsIsAwake(WB_IO4)){
  //  MESH_DEBUG_PRINTLN("RAK base board is RAK19003/9");
  //  MESH_DEBUG_PRINTLN("GPS is installed on Socket C");
  }
  else if(gpsIsAwake(WB_IO5)){
  //  MESH_DEBUG_PRINTLN("RAK base board is RAK19001/11");
  //  MESH_DEBUG_PRINTLN("GPS is installed on Socket F");
  }
  else{
    MESH_DEBUG_PRINTLN("No GPS found");
    gps_active = false;
    gps_detected = false;
    Serial1.end();
    return;
  }

  #ifndef FORCE_GPS_ALIVE // for use with repeaters, until GPS toggle is implimented
  //Now that GPS is found and set up, set to sleep for initial state
  stop_gps();
  #endif
}

bool HeltecEnvironmentSensorManager::gpsIsAwake(uint8_t ioPin){

  //set initial waking state
  pinMode(ioPin,OUTPUT);
  digitalWrite(ioPin,LOW);
  delay(500);
  digitalWrite(ioPin,HIGH);
  delay(500);

  //Try to init RAK12500 on I2C
  if (ublox_GNSS.begin(Wire) == true){
    MESH_DEBUG_PRINTLN("RAK12500 GPS init correctly with pin %i",ioPin);
    ublox_GNSS.setI2COutput(COM_TYPE_UBX);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_GPS);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_GALILEO);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_GLONASS);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_SBAS);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_BEIDOU);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_IMES);
    ublox_GNSS.enableGNSS(true, SFE_UBLOX_GNSS_ID_QZSS);
    ublox_GNSS.setMeasurementRate(1000);
    ublox_GNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
    gpsResetPin = ioPin;
    i2cGPSFlag = true;
    gps_active = true;
    gps_detected = true;

    _location = &RAK12500_provider;
    return true;
  } else if (Serial1.available()) {
    MESH_DEBUG_PRINTLN("Serial GPS init correctly and is turned on");
    if(PIN_GPS_EN){
      gpsResetPin = PIN_GPS_EN;
    }
    serialGPSFlag = true;
    gps_active = true;
    gps_detected = true;
    return true;
  }

  pinMode(ioPin, INPUT);
  MESH_DEBUG_PRINTLN("GPS did not init with this IO pin... try the next");
  return false;
}
#endif

void HeltecEnvironmentSensorManager::start_gps() {
  gps_active = true;
  #ifdef RAK_WISBLOCK_GPS
    pinMode(gpsResetPin, OUTPUT);
    digitalWrite(gpsResetPin, HIGH);
    return;
  #endif

  initGpsSerial();
  _location->begin();
  _location->reset();

#ifndef PIN_GPS_RESET
  MESH_DEBUG_PRINTLN("Start GPS is N/A on this board. Actual GPS state unchanged");
#endif
  configureGpsModule(_location);
}

void HeltecEnvironmentSensorManager::stop_gps() {
  gps_active = false;
  #ifdef RAK_WISBLOCK_GPS
    pinMode(gpsResetPin, OUTPUT);
    digitalWrite(gpsResetPin, LOW);
    return;
  #endif

  _location->stop();

  #ifndef PIN_GPS_EN
  MESH_DEBUG_PRINTLN("Stop GPS is N/A on this board. Actual GPS state unchanged");
  #endif
}

#endif  // ENV_INCLUDE_GPS

void HeltecEnvironmentSensorManager::loop() {
#if ENV_INCLUDE_GPS
  static long next_gps_update = 0;
  static bool last_enabled = false;
  static bool last_fix = false;
  static long last_lat = 0;
  static long last_lon = 0;
  static long last_alt = 0;
  static long last_sats = 0;
  if (_location) {
    _location->loop();
  }
  if (millis() > next_gps_update) {
    if (gps_active) {
#ifdef RAK_WISBLOCK_GPS
      if ((i2cGPSFlag || serialGPSFlag) && _location->isValid()) {
        node_lat = ((double)_location->getLatitude()) / 1000000.;
        node_lon = ((double)_location->getLongitude()) / 1000000.;
        MESH_DEBUG_PRINTLN("lat %f lon %f", node_lat, node_lon);
        node_altitude = ((double)_location->getAltitude()) / 1000.0;
      }
#else
      if (_location->isValid()) {
        node_lat = ((double)_location->getLatitude()) / 1000000.;
        node_lon = ((double)_location->getLongitude()) / 1000000.;
        node_altitude = ((double)_location->getAltitude()) / 1000.0;
      }
#endif
    }
#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
    if (_location) {
      const bool enabled = gps_active && _location->isEnabled();
      const bool fix = enabled && _location->isValid();
      const long lat = fix ? _location->getLatitude() : 0;
      const long lon = fix ? _location->getLongitude() : 0;
      const long alt = fix ? _location->getAltitude() : 0;
      const long sats = enabled ? _location->satellitesCount() : 0;
      if (enabled != last_enabled || fix != last_fix || lat != last_lat || lon != last_lon ||
          alt != last_alt || sats != last_sats) {
        last_enabled = enabled;
        last_fix = fix;
        last_lat = lat;
        last_lon = lon;
        last_alt = alt;
        last_sats = sats;
        heltec::meshcore::ui::AppStateEvent ev{};
        ev.type = heltec::meshcore::ui::AppStateEventType::GpsChanged;
        ev.gps.enabled = enabled;
        ev.gps.available = true;
        ev.gps.fix_valid = fix;
        ev.gps.satellites = (sats < 0) ? 0 : (sats > 255 ? 255 : (uint8_t)sats);
        ev.gps.lat_micro = lat;
        ev.gps.lon_micro = lon;
        ev.gps.lat_deg = lat / 1000000.0;
        ev.gps.lon_deg = lon / 1000000.0;
        ev.gps.alt_m = alt / 1000.0;
        heltec::meshcore::ui::app_state_notifier().notify(ev);
      }
    }
#endif
    next_gps_update = millis() + (gps_update_interval_sec * 1000);
  }
#endif
#if ENV_INCLUDE_COMPASS
  if (_compass) {
    _compass->loop();
  }
#endif
}

