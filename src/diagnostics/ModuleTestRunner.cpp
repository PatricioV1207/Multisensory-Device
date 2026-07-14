#include "diagnostics/ModuleTestRunner.h"

#include <Wire.h>
#include <cmath>
#include "config.h"
#include "pins.h"
#include "telemetry/TelemetryBuilder.h"
#include "utils/I2CScanner.h"
#include "utils/Logger.h"

namespace {
float magnitude3(float x, float y, float z) {
  return std::sqrt((x * x) + (y * y) + (z * z));
}
}  // namespace

bool ModuleTestRunner::usesI2C() const {
  return APP_MODE == APP_MODE_TEST_I2C_SCANNER ||
         APP_MODE == APP_MODE_TEST_ADXL345 || APP_MODE == APP_MODE_TEST_L3G4200D ||
         APP_MODE == APP_MODE_TEST_HMC5883L || APP_MODE == APP_MODE_TEST_BMP180 ||
         APP_MODE == APP_MODE_TEST_GY801;
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
    _barometer.begin();
  } else if (APP_MODE == APP_MODE_TEST_GY801) {
    I2CScanner::scan();
    _gy801.begin();
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
    Serial.printf("[TEST_GPS] fix=%d stream=%d chars=%lu lat=%.6f lon=%.6f sats=%lu\n",
                  d.valid, d.streamSeen, static_cast<unsigned long>(d.charsProcessed),
                  d.latitude, d.longitude, static_cast<unsigned long>(d.satellites));
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
    Serial.printf("[TEST_BMP180] valid=%d pressure=%.2fhPa temp=%.2fC alt=%.2fm\n",
                  d.valid, d.pressureHpa, d.temperatureC, d.altitudeM);
  } else if (APP_MODE == APP_MODE_TEST_GY801) {
    const GY801Data& d = _gy801.getData();
    Serial.printf("[TEST_GY801] imu=%d accel=%d gyro=%d mag=%d baro=%d\n",
                  d.imuValid, d.accel.valid, d.gyro.valid, d.mag.valid,
                  d.barometer.valid);
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
  TelemetryData sample;
  sample.deviceId = DEVICE_ID;
  sample.uptimeMs = nowMs;
  char payload[TELEMETRY_PAYLOAD_BUFFER_SIZE];
  if (!TelemetryBuilder::build(sample, payload, sizeof(payload))) {
    Logger::error("JSON", "Could not build MQTT test payload");
    return;
  }
  Logger::info("MQTT", _mqtt.publish(payload) ? "Test payload published" :
                                                "Test publish failed");
}
