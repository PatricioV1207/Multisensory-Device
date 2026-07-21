#include "diagnostics/ModuleTestRunner.h"

#include <Wire.h>
#include <cmath>
#include <cstring>
#include "calibration/BarometerCalibrationStore.h"
#include "config.h"
#include "pins.h"
#include "telemetry/TelemetryBuilder.h"
#include "utils/I2CScanner.h"
#include "utils/Logger.h"

namespace {
float magnitude3(float x, float y, float z) {
  return std::sqrt((x * x) + (y * y) + (z * z));
}

bool hasRealValue(const char* value) {
  return value != nullptr && value[0] != '\0' &&
         strncmp(value, "YOUR_", 5) != 0;
}
}  // namespace

bool ModuleTestRunner::usesI2C() const {
  return APP_MODE == APP_MODE_TEST_I2C_SCANNER ||
         APP_MODE == APP_MODE_TEST_ADXL345 || APP_MODE == APP_MODE_TEST_L3G4200D ||
         APP_MODE == APP_MODE_TEST_HMC5883L || APP_MODE == APP_MODE_TEST_BMP180 ||
         APP_MODE == APP_MODE_TEST_GY801 ||
         APP_MODE == APP_MODE_CALIBRATE_BMP180 ||
         APP_MODE == APP_MODE_TEST_BH1750;
}

void ModuleTestRunner::begin() {
  Logger::info("TEST", "Module test mode=" + String(APP_MODE));
  if (usesI2C()) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setTimeOut(I2C_TIMEOUT_MS);
  }

  if (APP_MODE == APP_MODE_TEST_DHT11) {
    _dht.begin();
  } else if (APP_MODE == APP_MODE_TEST_GPS) {
    _gps.begin();
  } else if (APP_MODE == APP_MODE_TEST_I2C_SCANNER) {
    I2CScanner::scan();
    _lastScanMs = millis();
  } else if (APP_MODE == APP_MODE_TEST_ADXL345) {
    _accel.begin();
  } else if (APP_MODE == APP_MODE_TEST_L3G4200D) {
    _gyro.begin();
  } else if (APP_MODE == APP_MODE_TEST_HMC5883L) {
    _mag.begin();
  } else if (APP_MODE == APP_MODE_TEST_BMP180) {
    const StoredBarometerCalibration calibration =
        BarometerCalibrationStore::load();
    _barometer.setCalibration(calibration.seaLevelPressureHpa,
                              BARO_PRESSURE_OFFSET_HPA, calibration.source);
    _barometer.begin();
  } else if (APP_MODE == APP_MODE_TEST_GY801) {
    I2CScanner::scan();
    const StoredBarometerCalibration calibration =
        BarometerCalibrationStore::load();
    _gy801.setBarometerCalibration(calibration.seaLevelPressureHpa,
                                   BARO_PRESSURE_OFFSET_HPA,
                                   calibration.source);
    _gy801.begin();
  } else if (APP_MODE == APP_MODE_CALIBRATE_BMP180) {
    _barometerCalibrationRunner.begin();
  } else if (APP_MODE == APP_MODE_TEST_BH1750) {
    _light.begin();
  } else if (APP_MODE == APP_MODE_TEST_MICROSD) {
    _storage.begin();
  } else if (APP_MODE == APP_MODE_TEST_SIM800L_AT) {
    _modem.begin(SIM800LMode::AtOnly);
  } else if (APP_MODE == APP_MODE_TEST_SIM800L_GPRS) {
    _modem.begin(SIM800LMode::Gprs);
  } else if (APP_MODE == APP_MODE_TEST_SIM800L_MQTT ||
             APP_MODE == APP_MODE_TEST_SIM800L_MQTT_TLS) {
    _modem.begin(SIM800LMode::Gprs);
    const bool tls = APP_MODE == APP_MODE_TEST_SIM800L_MQTT_TLS;
    const MqttConnectionConfig connection =
        tls ? MqttConnectionConfig{CELLULAR_MQTT_HOST, CELLULAR_MQTT_PORT,
                                   CELLULAR_MQTT_USERNAME,
                                   CELLULAR_MQTT_PASSWORD,
                                   CELLULAR_MQTT_TOPIC}
            : MqttConnectionConfig{MQTT_TEST_HOST, MQTT_TEST_PORT,
                                   MQTT_TEST_USERNAME, MQTT_TEST_PASSWORD,
                                   MQTT_TEST_TOPIC};
    _mqtt.begin(_modem.networkClient(tls), connection);
  } else if (APP_MODE == APP_MODE_TEST_LOCAL_WEB ||
             APP_MODE == APP_MODE_TEST_LOCAL_OTA) {
    _web.begin();
    updateLocalWebMock(millis());
  } else if (APP_MODE == APP_MODE_TEST_WIFI ||
             APP_MODE == APP_MODE_TEST_MQTT_WIFI) {
    _wifi.begin();
    if (APP_MODE == APP_MODE_TEST_MQTT_WIFI) {
      _mqtt.begin(_wifiClient);
    }
  }
}

void ModuleTestRunner::update(uint32_t nowMs) {
  if (APP_MODE == APP_MODE_TEST_DHT11) {
    _dht.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_GPS) {
    _gps.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_I2C_SCANNER) {
    if (static_cast<uint32_t>(nowMs - _lastScanMs) >= I2C_SCAN_INTERVAL_MS) {
      _lastScanMs = nowMs;
      I2CScanner::scan();
    }
  } else if (APP_MODE == APP_MODE_TEST_ADXL345) {
    _accel.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_L3G4200D) {
    _gyro.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_HMC5883L) {
    _mag.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_BMP180) {
    _barometer.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_GY801) {
    _gy801.update(nowMs);
  } else if (APP_MODE == APP_MODE_CALIBRATE_BMP180) {
    _barometerCalibrationRunner.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_BH1750) {
    _light.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_MICROSD) {
    _storage.update(nowMs);
    if (_storage.isReady() &&
        static_cast<uint32_t>(nowMs - _lastStorageTestMs) >=
            TEST_OUTPUT_INTERVAL_MS) {
      _lastStorageTestMs = nowMs;
      char record[160];
      const int count = snprintf(
          record, sizeof(record),
          "{\"schema_version\":2,\"diagnostic\":true,\"uptime_ms\":%lu}",
          static_cast<unsigned long>(nowMs));
      if (count > 0) {
        _storage.appendJsonLine(record, static_cast<size_t>(count), nowMs);
      }
    }
  } else if (APP_MODE == APP_MODE_TEST_SIM800L_AT ||
             APP_MODE == APP_MODE_TEST_SIM800L_GPRS) {
    _modem.update(nowMs);
    if (APP_MODE == APP_MODE_TEST_SIM800L_GPRS) {
      if (!_modem.isGprsConnected()) {
        _tcpTestAttempted = false;
        _tcpTestOk = false;
      } else if (hasRealValue(MQTT_TEST_HOST) && MQTT_TEST_PORT > 0 &&
                 (!_tcpTestAttempted ||
                  static_cast<uint32_t>(nowMs - _lastTcpTestMs) >=
                      SIM800_TCP_TEST_INTERVAL_MS)) {
        _lastTcpTestMs = nowMs;
        _tcpTestAttempted = true;
        Client& tcpClient = _modem.networkClient(false);
        _tcpTestOk = tcpClient.connect(MQTT_TEST_HOST, MQTT_TEST_PORT) == 1;
        tcpClient.stop();
        Logger::info("SIM800", _tcpTestOk ? "TCP probe succeeded" :
                                             "TCP probe failed");
      }
    }
  } else if (APP_MODE == APP_MODE_TEST_SIM800L_MQTT ||
             APP_MODE == APP_MODE_TEST_SIM800L_MQTT_TLS) {
    _modem.update(nowMs);
    _mqtt.update(nowMs, _modem.isGprsConnected());
    publishMqttTest(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_LOCAL_WEB ||
             APP_MODE == APP_MODE_TEST_LOCAL_OTA) {
    updateLocalWebMock(nowMs);
    _web.update(nowMs);
  } else if (APP_MODE == APP_MODE_TEST_WIFI ||
             APP_MODE == APP_MODE_TEST_MQTT_WIFI) {
    _wifi.update(nowMs);
    if (APP_MODE == APP_MODE_TEST_MQTT_WIFI) {
      _mqtt.update(nowMs, _wifi.isConnected());
      publishMqttTest(nowMs);
    }
  }

  if (static_cast<uint32_t>(nowMs - _lastOutputMs) >= TEST_OUTPUT_INTERVAL_MS) {
    _lastOutputMs = nowMs;
    printReadings(nowMs);
  }
}

void ModuleTestRunner::printReadings(uint32_t nowMs) {
  (void)nowMs;
  if (APP_MODE == APP_MODE_TEST_DHT11) {
    const DHT11Data& d = _dht.getData();
    Serial.printf("[TEST_DHT11] %s temp=%.2fC humidity=%.2f%%\n",
                  d.valid ? "PASS" : "WAIT/FAIL", d.temperatureC,
                  d.humidityPercent);
  } else if (APP_MODE == APP_MODE_TEST_GPS) {
    const GPSData& d = _gps.getData();
    Serial.printf("[TEST_GPS] fix=%d stream=%d chars=%lu lat=%.6f "
                  "lon=%.6f sats=%lu hdop=%.2f\n",
                  d.valid, d.streamSeen, static_cast<unsigned long>(d.charsProcessed),
                  d.latitude, d.longitude,
                  static_cast<unsigned long>(d.satellites), d.hdop);
  } else if (APP_MODE == APP_MODE_TEST_ADXL345) {
    const AccelData& d = _accel.getData();
    const float magnitudeRaw = magnitude3(d.rawX, d.rawY, d.rawZ);
    const float magnitudeCal = magnitude3(d.x, d.y, d.z);
    Serial.printf("[TEST_ADXL345] valid=%d accel_raw_mps2=%.3f,%.3f,%.3f "
                  "accel_cal_mps2=%.3f,%.3f,%.3f magnitude_raw_mps2=%.3f "
                  "magnitude_cal_mps2=%.3f\n",
                  d.valid, d.rawX, d.rawY, d.rawZ, d.x, d.y, d.z,
                  magnitudeRaw, magnitudeCal);
  } else if (APP_MODE == APP_MODE_TEST_L3G4200D) {
    const GyroData& d = _gyro.getData();
    Serial.printf("[TEST_L3G4200D] valid=%d gyro_rad_s=%.4f,%.4f,%.4f\n",
                  d.valid, d.x, d.y, d.z);
  } else if (APP_MODE == APP_MODE_TEST_HMC5883L) {
    const MagData& d = _mag.getData();
    Serial.printf("[TEST_HMC5883L] valid=%d mag_uT=%.2f,%.2f,%.2f\n",
                  d.valid, d.x, d.y, d.z);
  } else if (APP_MODE == APP_MODE_TEST_BMP180) {
    const BarometerData& d = _barometer.getData();
    Serial.printf("[TEST_BMP180] valid=%d pressure_raw_hpa=%.2f "
                  "pressure_local_hpa=%.2f sea_level_pressure_hpa=%.2f "
                  "temperature_c=%.2f altitude_m=%.2f "
                  "calibration_source=%s\n",
                  d.valid, d.rawPressureHpa, d.pressureHpa,
                  d.seaLevelPressureHpa, d.temperatureC, d.altitudeM,
                  BarometerCalibrationStore::sourceName(
                      d.calibrationSource));
  } else if (APP_MODE == APP_MODE_TEST_GY801) {
    const GY801Data& d = _gy801.getData();
    Serial.printf("[TEST_GY801] imu=%d accel=%d gyro=%d mag=%d baro=%d\n",
                  d.imuValid, d.accel.valid, d.gyro.valid, d.mag.valid,
                  d.barometer.valid);
  } else if (APP_MODE == APP_MODE_TEST_BH1750) {
    const LightData& d = _light.getData();
    Serial.printf("[TEST_BH1750] valid=%d light_lux=%.2f\n", d.valid,
                  d.lux);
  } else if (APP_MODE == APP_MODE_TEST_MICROSD) {
    const StorageStatus& d = _storage.getStatus();
    Serial.printf("[TEST_MICROSD] mounted=%d last_write_ok=%d "
                  "boot=%lu segment=%u free_bytes=%llu\n",
                  d.mounted, d.lastWriteOk,
                  static_cast<unsigned long>(d.bootSession),
                  static_cast<unsigned>(d.segment),
                  static_cast<unsigned long long>(d.freeBytes));
  } else if (APP_MODE == APP_MODE_TEST_SIM800L_AT ||
             APP_MODE == APP_MODE_TEST_SIM800L_GPRS ||
             APP_MODE == APP_MODE_TEST_SIM800L_MQTT ||
             APP_MODE == APP_MODE_TEST_SIM800L_MQTT_TLS) {
    const CellularStatus& d = _modem.getStatus();
    const int tcpStatus = APP_MODE == APP_MODE_TEST_SIM800L_GPRS
                              ? (_tcpTestAttempted ? (_tcpTestOk ? 1 : 0) : -1)
                              : -1;
    Serial.printf("[TEST_SIM800L] modem=%d model=%s sim=%d network=%d "
                  "gprs=%d operator=%s ip=%s csq=%d dbm=%d error=%d "
                  "tcp=%d mqtt=%d state=%d\n",
                  d.modemPresent, d.modemName, d.simReady, d.networkRegistered,
                  d.gprsConnected, d.operatorName, d.localIp,
                  d.signalQuality, d.signalDbm, d.lastError,
                  tcpStatus, _mqtt.isConnected(), _mqtt.state());
  } else if (APP_MODE == APP_MODE_TEST_LOCAL_WEB ||
             APP_MODE == APP_MODE_TEST_LOCAL_OTA) {
    Serial.printf("[TEST_LOCAL_WEB] running=%d ip=%s ota=%d\n",
                  _web.isRunning(), _web.localIp().c_str(),
                  _web.getOtaStatus().enabled);
  } else if (APP_MODE == APP_MODE_TEST_WIFI ||
             APP_MODE == APP_MODE_TEST_MQTT_WIFI) {
    Serial.printf("[TEST_NET] wifi=%d ip=%s rssi=%ld mqtt=%d state=%d\n",
                  _wifi.isConnected(), _wifi.localIp().c_str(),
                  static_cast<long>(_wifi.rssi()), _mqtt.isConnected(), _mqtt.state());
  }
}

void ModuleTestRunner::publishMqttTest(uint32_t nowMs) {
  if (!_mqtt.isConnected() ||
      static_cast<uint32_t>(nowMs - _lastPublishMs) < TELEMETRY_INTERVAL_MS) {
    return;
  }
  _lastPublishMs = nowMs;
  char payload[256];
  const char* transport =
      APP_MODE == APP_MODE_TEST_SIM800L_MQTT_TLS ? "cellular_tls" :
      (APP_MODE == APP_MODE_TEST_SIM800L_MQTT ? "cellular_tcp" : "wifi");
  const int count = snprintf(
      payload, sizeof(payload),
      "{\"schema_version\":2,\"device_id\":\"%s\",\"diagnostic\":true,"
      "\"transport\":\"%s\",\"uptime_ms\":%lu}",
      DEVICE_ID, transport, static_cast<unsigned long>(nowMs));
  if (count <= 0 || static_cast<size_t>(count) >= sizeof(payload)) {
    Logger::error("JSON", "Could not build synthetic MQTT test payload");
    return;
  }
  Logger::info("MQTT", _mqtt.publish(payload) ? "Test payload published" :
                                                "Test publish failed");
}

void ModuleTestRunner::updateLocalWebMock(uint32_t nowMs) {
  _localWebData.deviceId = DEVICE_ID;
  _localWebData.vehicleId = VEHICLE_ID;
  _localWebData.firmwareVersion = FIRMWARE_VERSION;
  _localWebData.simulated = true;
  _localWebData.uptimeMs = nowMs;
  _localWebData.dht.temperatureC = 25.4F;
  _localWebData.dht.humidityPercent = 61.0F;
  _localWebData.dht.valid = true;
  _localWebData.dht.updatedAtMs = nowMs;
  _localWebData.light.lux = 340.0F;
  _localWebData.light.valid = true;
  _localWebData.light.updatedAtMs = nowMs;
  _localWebData.gps.latitude = -2.189412;
  _localWebData.gps.longitude = -79.889066;
  _localWebData.gps.altitudeM = 12.4F;
  _localWebData.gps.speedKmh = 36.2F;
  _localWebData.gps.satellites = 10;
  _localWebData.gps.hdop = 0.9F;
  _localWebData.gps.valid = true;
  _localWebData.gps.streamSeen = true;
  _localWebData.gps.charsProcessed = 5000;
  _localWebData.accel.x = 0.1F;
  _localWebData.accel.y = 0.2F;
  _localWebData.accel.z = 9.8F;
  _localWebData.accel.valid = true;
  _localWebData.accel.updatedAtMs = nowMs;
  _localWebData.gyro.x = 0.01F;
  _localWebData.gyro.y = 0.02F;
  _localWebData.gyro.z = 0.03F;
  _localWebData.gyro.valid = true;
  _localWebData.gyro.updatedAtMs = nowMs;
  _localWebData.mag.x = 12.0F;
  _localWebData.mag.y = 18.0F;
  _localWebData.mag.z = 25.0F;
  _localWebData.mag.valid = true;
  _localWebData.mag.updatedAtMs = nowMs;
  _localWebData.barometer.pressureHpa = 1006.6F;
  _localWebData.barometer.temperatureC = 25.0F;
  _localWebData.barometer.altitudeM = 10.0F;
  _localWebData.barometer.valid = true;
  _localWebData.barometer.updatedAtMs = nowMs;
  _localWebData.imuValid = true;
  _localWebData.storage.initialized = true;
  _localWebData.storage.mounted = true;
  _localWebData.storage.lastWriteOk = true;
  _localWebData.cellular.modemPresent = true;
  _localWebData.cellular.simReady = true;
  _localWebData.cellular.networkRegistered = true;
  _localWebData.cellular.gprsConnected = true;
  _localWebData.cellular.signalQuality = 20;
  _localWebData.cellular.signalDbm = -73;
  _localWebData.mqtt.configured = true;
  _localWebData.mqtt.connected = true;
  _localWebData.mqtt.lastPublishOk = true;
  _localWebData.mqtt.lastPublishSuccessMs = nowMs;
  _localWebData.mqtt.lastPublishAckMs = nowMs;
  _localWebData.offline.ready = true;
  _localWebData.offline.queued = 3;
  _localWebData.offline.replayed = 4;
  _localWebData.offline.dropped = 0;
  _localWebData.offline.oldestAgeSeconds = 25;
  _localWebData.offline.bytes = 4096;
  _localWebData.wifi.accessPointRunning = _web.isRunning();
  const String accessPointIp = _web.localIp();
  if (_web.isRunning()) {
    accessPointIp.toCharArray(_localWebData.wifi.accessPointIp,
                              sizeof(_localWebData.wifi.accessPointIp));
  }
  _localWebData.ota = _web.getOtaStatus();
  _web.setData(_localWebData);
}
