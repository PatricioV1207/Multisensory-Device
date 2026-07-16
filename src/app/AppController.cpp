#include "app/AppController.h"

#include <Wire.h>
#include <esp_timer.h>
#include "calibration/BarometerCalibrationStore.h"
#include "config.h"
#include "pins.h"
#include "telemetry/TelemetryBuilder.h"
#include "telemetry/TelemetryValidator.h"
#include "utils/Logger.h"

void AppController::begin() {
  Logger::begin(SERIAL_BAUD);
  Logger::info("BOOT", "Starting " + String(DEVICE_ID) +
                           " app_mode=" + String(APP_MODE));

#if APP_MODE != APP_MODE_FULL_PROTOTYPE
  _testRunner.begin();
  return;
#endif

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  _dht.begin();
  _gps.begin();
  const StoredBarometerCalibration barometerCalibration =
      BarometerCalibrationStore::load();
  _gy801.setBarometerCalibration(
      barometerCalibration.seaLevelPressureHpa, BARO_PRESSURE_OFFSET_HPA,
      barometerCalibration.source);
  _gy801.begin();
  Logger::info(
      "BARO",
      "Reference p0=" + String(barometerCalibration.seaLevelPressureHpa, 3) +
          " hPa source=" +
          BarometerCalibrationStore::sourceName(barometerCalibration.source));
  _wifi.begin();
  _mqtt.begin(_wifiClient);
  Logger::info("BOOT", "Initialization complete; failures are non-fatal");
}

void AppController::update() {
  const uint32_t nowMs = millis();

#if APP_MODE != APP_MODE_FULL_PROTOTYPE
  _testRunner.update(nowMs);
  delay(1);
  return;
#endif

  _gps.update(nowMs);
  _dht.update(nowMs);
  _gy801.update(nowMs);
  _wifi.update(nowMs);
  _mqtt.update(nowMs, _wifi.isConnected());

  if (static_cast<uint32_t>(nowMs - _lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
    _lastTelemetryMs = nowMs;
    publishTelemetry(nowMs);
  }
  delay(1);
}

void AppController::buildSnapshot(uint32_t nowMs) {
  _telemetry.deviceId = DEVICE_ID;
  _telemetry.uptimeMs = static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
  _telemetry.dht = _dht.getData();
  _telemetry.gps = _gps.getData();
  _telemetry.gy801 = _gy801.getData();
  TelemetryValidator::validate(_telemetry, nowMs);
}

void AppController::publishTelemetry(uint32_t nowMs) {
  buildSnapshot(nowMs);
  size_t written = 0;
  if (!TelemetryBuilder::build(_telemetry, _payload, sizeof(_payload), &written)) {
    Logger::error("JSON", "Serialization failed or payload buffer is too small");
    return;
  }

  Logger::info("JSON", String(_payload));
  Logger::debug("JSON", "Payload bytes=" + String(written));
  if (!_mqtt.isConnected()) {
    Logger::warn("MQTT", "Telemetry not published; broker offline (no local queue)");
    return;
  }
  if (_mqtt.publish(_payload)) {
    Logger::info("MQTT", "Telemetry published");
  }
}
