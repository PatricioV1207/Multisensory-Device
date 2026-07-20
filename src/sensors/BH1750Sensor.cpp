#include "sensors/BH1750Sensor.h"

#include <Wire.h>
#include <cmath>
#include "config.h"
#include "utils/Logger.h"

bool BH1750Sensor::probe(uint8_t address) const {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool BH1750Sensor::begin() {
  _lastBeginAttemptMs = millis();
  _address = probe(BH1750_ADDRESS_PRIMARY)
                 ? BH1750_ADDRESS_PRIMARY
                 : (probe(BH1750_ADDRESS_ALTERNATE)
                        ? BH1750_ADDRESS_ALTERNATE
                        : 0);
  if (_address == 0) {
    _started = false;
    _data.valid = false;
    Logger::warn("BH1750", "Sensor missing at 0x23 and 0x5C");
    return false;
  }
  _started = _sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, _address, &Wire);
  if (!_started) {
    _data.valid = false;
    Logger::warn("BH1750", "Library initialization failed");
    return false;
  }
  Logger::info("BH1750", "Detected at 0x" + String(_address, HEX));
  return true;
}

void BH1750Sensor::update(uint32_t nowMs) {
  if (!_started) {
    if (static_cast<uint32_t>(nowMs - _lastBeginAttemptMs) >=
        SENSOR_RETRY_INTERVAL_MS) {
      begin();
    }
    return;
  }
  if (static_cast<uint32_t>(nowMs - _lastReadMs) < LIGHT_READ_INTERVAL_MS) {
    return;
  }
  _lastReadMs = nowMs;

  if (!probe(_address)) {
    _started = false;
    _data.valid = false;
    _lastBeginAttemptMs = nowMs;
    Logger::warn("BH1750", "Sensor stopped responding; retry scheduled");
    return;
  }

  const float lux = _sensor.readLightLevel();
  _data.valid = std::isfinite(lux) && lux >= 0.0F && lux <= BH1750_MAX_LUX;
  _data.lux = _data.valid ? lux : NAN;
  _data.updatedAtMs = nowMs;
}

bool BH1750Sensor::isValid() const {
  return _data.valid;
}

const LightData& BH1750Sensor::getData() const {
  return _data;
}
