#include "sensors/gy801/HMC5883LMag.h"

#include <Wire.h>
#include <cmath>
#include "config.h"
#include "utils/Logger.h"

namespace {
bool readRegisters(uint8_t address, uint8_t reg, uint8_t* values, size_t count) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(address, static_cast<uint8_t>(count)) != count) {
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    values[i] = Wire.read();
  }
  return true;
}

bool addressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}
}  // namespace

bool HMC5883LMag::probe() const {
  uint8_t id[3] = {0, 0, 0};
  return readRegisters(HMC5883L_ADDRESS, 0x0A, id, sizeof(id)) &&
         id[0] == 'H' && id[1] == '4' && id[2] == '3';
}

bool HMC5883LMag::possibleQmcClone() const {
  return addressResponds(QMC5883L_POSSIBLE_ADDRESS);
}

bool HMC5883LMag::begin() {
  _lastBeginAttemptMs = millis();
  if (!probe()) {
    _started = false;
    _data.valid = false;
    if (possibleQmcClone()) {
      Logger::warn("GY801", "0x0D detected: possible QMC5883L; HMC driver not used");
    } else {
      Logger::warn("GY801", "HMC5883L missing or identity is not H43");
    }
    return false;
  }
  _started = _sensor.begin();
  if (!_started) {
    Logger::warn("GY801", "HMC5883L library initialization failed");
    return false;
  }
  Logger::info("GY801", "HMC5883L verified at 0x1E");
  return true;
}

void HMC5883LMag::update(uint32_t nowMs) {
  if (!_started) {
    if (static_cast<uint32_t>(nowMs - _lastBeginAttemptMs) >= SENSOR_RETRY_INTERVAL_MS) {
      begin();
    }
    return;
  }
  if (static_cast<uint32_t>(nowMs - _lastReadMs) < IMU_READ_INTERVAL_MS) {
    return;
  }
  _lastReadMs = nowMs;
  if (!probe()) {
    _data.valid = false;
    _started = false;
    _lastBeginAttemptMs = nowMs;
    Logger::warn("GY801", "HMC5883L stopped responding; retry scheduled");
    return;
  }
  sensors_event_t event;
  _sensor.getEvent(&event);
  _data.x = event.magnetic.x;
  _data.y = event.magnetic.y;
  _data.z = event.magnetic.z;
  _data.updatedAtMs = nowMs;
  _data.valid = std::isfinite(_data.x) && std::isfinite(_data.y) &&
                std::isfinite(_data.z);
}

bool HMC5883LMag::isValid() const {
  return _data.valid;
}

const MagData& HMC5883LMag::getData() const {
  return _data;
}
