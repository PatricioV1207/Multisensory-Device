#include "sensors/DHT11Sensor.h"

#include <cmath>
#include "config.h"
#include "pins.h"
#include "utils/Logger.h"

bool DHT11Sensor::begin() {
  _sensor.begin();
  _started = true;
  _data.valid = false;
  Logger::info("DHT", "DHT11 initialized on GPIO " + String(PIN_DHT11_DATA));
  return true;
}

void DHT11Sensor::update(uint32_t nowMs) {
  if (!_started || static_cast<uint32_t>(nowMs - _lastReadMs) < DHT_READ_INTERVAL_MS) {
    return;
  }
  _lastReadMs = nowMs;

  const float humidity = _sensor.readHumidity();
  const float temperature = _sensor.readTemperature();
  if (!std::isfinite(humidity) || !std::isfinite(temperature)) {
    _data.valid = false;
    if (_lastWarningMs == 0 ||
        static_cast<uint32_t>(nowMs - _lastWarningMs) >= SENSOR_RETRY_INTERVAL_MS) {
      _lastWarningMs = nowMs;
      Logger::warn("DHT", "Read failed; sensor disconnected or timing invalid");
    }
    return;
  }

  _data.temperatureC = temperature;
  _data.humidityPercent = humidity;
  _data.updatedAtMs = nowMs;
  _data.valid = true;
}

bool DHT11Sensor::isValid() const {
  return _data.valid;
}

const DHT11Data& DHT11Sensor::getData() const {
  return _data;
}
