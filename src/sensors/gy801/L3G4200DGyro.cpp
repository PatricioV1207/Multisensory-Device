#include "sensors/gy801/L3G4200DGyro.h"

#include <Wire.h>
#include <cmath>
#include "config.h"
#include "utils/Logger.h"

namespace {
constexpr float L3G4200D_RAD_PER_SECOND_PER_LSB =
    0.00875F * 0.01745329251994329577F;

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

bool L3G4200DGyro::probe(uint8_t address) const {
  uint8_t whoAmI = 0;
  return readRegister(address, 0x0F, whoAmI) && whoAmI == 0xD3;
}

bool L3G4200DGyro::begin() {
  _lastBeginAttemptMs = millis();
  if (probe(L3G4200D_ADDRESS_PRIMARY)) {
    _address = L3G4200D_ADDRESS_PRIMARY;
  } else if (probe(L3G4200D_ADDRESS_ALTERNATE)) {
    _address = L3G4200D_ADDRESS_ALTERNATE;
  } else {
    _started = false;
    _data.valid = false;
    Logger::warn("GY801", "L3G4200D missing or WHO_AM_I is not 0xD3");
    return false;
  }

  const L3G::sa0State sa0 = _address == L3G4200D_ADDRESS_PRIMARY
                                ? L3G::sa0_high
                                : L3G::sa0_low;
  _started = _sensor.init(L3G::device_4200D, sa0);
  if (!_started) {
    Logger::warn("GY801", "L3G4200D library initialization failed");
    return false;
  }
  _sensor.enableDefault();
  _sensor.setTimeout(I2C_TIMEOUT_MS);
  Logger::info("GY801", "L3G4200D verified at 0x" + String(_address, HEX));
  return true;
}

void L3G4200DGyro::update(uint32_t nowMs) {
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
    Logger::warn("GY801", "L3G4200D stopped responding; retry scheduled");
    return;
  }
  _sensor.read();
  if (_sensor.timeoutOccurred()) {
    _data.valid = false;
    _started = false;
    _lastBeginAttemptMs = nowMs;
    Logger::warn("GY801", "L3G4200D I2C read timeout");
    return;
  }
  _data.x = _sensor.g.x * L3G4200D_RAD_PER_SECOND_PER_LSB;
  _data.y = _sensor.g.y * L3G4200D_RAD_PER_SECOND_PER_LSB;
  _data.z = _sensor.g.z * L3G4200D_RAD_PER_SECOND_PER_LSB;
  _data.updatedAtMs = nowMs;
  _data.valid = std::isfinite(_data.x) && std::isfinite(_data.y) &&
                std::isfinite(_data.z);
}

bool L3G4200DGyro::isValid() const {
  return _data.valid;
}

uint8_t L3G4200DGyro::address() const {
  return _address;
}

const GyroData& L3G4200DGyro::getData() const {
  return _data;
}
