#include "sensors/gy801/ADXL345Accel.h"

#include <Wire.h>
#include <cmath>
#include "config.h"
#include "utils/Logger.h"

namespace {
bool readRegister(uint8_t address, uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(address, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}
}  // namespace

bool ADXL345Accel::probe(uint8_t address) const {
  uint8_t id = 0;
  return readRegister(address, 0x00, id) && id == 0xE5;
}

bool ADXL345Accel::begin() {
  _lastBeginAttemptMs = millis();
  if (probe(ADXL345_ADDRESS_PRIMARY)) {
    _address = ADXL345_ADDRESS_PRIMARY;
  } else if (probe(ADXL345_ADDRESS_ALTERNATE)) {
    _address = ADXL345_ADDRESS_ALTERNATE;
  } else {
    _started = false;
    _data.valid = false;
    Logger::warn("GY801", "ADXL345 missing or DEVID is not 0xE5");
    return false;
  }

  _started = _sensor.begin(_address);
  if (!_started) {
    Logger::warn("GY801", "ADXL345 library initialization failed");
    return false;
  }
  _sensor.setRange(ADXL345_RANGE_16_G);
  Logger::info("GY801", "ADXL345 verified at 0x" + String(_address, HEX));
  return true;
}

void ADXL345Accel::update(uint32_t nowMs) {
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
  if (!probe(_address)) {
    _data.valid = false;
    _started = false;
    _lastBeginAttemptMs = nowMs;
    Logger::warn("GY801", "ADXL345 stopped responding; retry scheduled");
    return;
  }
  sensors_event_t event;
  _sensor.getEvent(&event);
  _data.x = event.acceleration.x;
  _data.y = event.acceleration.y;
  _data.z = event.acceleration.z;
  _data.updatedAtMs = nowMs;
  _data.valid = std::isfinite(_data.x) && std::isfinite(_data.y) &&
                std::isfinite(_data.z);
}

bool ADXL345Accel::isValid() const {
  return _data.valid;
}

uint8_t ADXL345Accel::address() const {
  return _address;
}

const AccelData& ADXL345Accel::getData() const {
  return _data;
}
