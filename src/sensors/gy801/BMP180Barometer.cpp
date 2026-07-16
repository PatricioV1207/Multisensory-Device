#include "sensors/gy801/BMP180Barometer.h"

#include <Wire.h>
#include <cmath>
#include "calibration/BarometerMath.h"
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

BMP180Barometer::BMP180Barometer()
    : _seaLevelPressureHpa(BARO_DEFAULT_SEA_LEVEL_PRESSURE_HPA),
      _pressureOffsetHpa(BARO_PRESSURE_OFFSET_HPA),
      _calibrationSource(BarometerCalibrationSource::Default) {
  _data.seaLevelPressureHpa = _seaLevelPressureHpa;
  _data.calibrationSource = _calibrationSource;
}

bool BMP180Barometer::probe() const {
  uint8_t id = 0;
  return readRegister(BMP180_ADDRESS, 0xD0, id) && id == 0x55;
}

bool BMP180Barometer::begin() {
  _lastBeginAttemptMs = millis();
  if (!probe()) {
    _started = false;
    _data.valid = false;
    Logger::warn("GY801", "BMP180/BMP085 missing or chip ID is not 0x55");
    return false;
  }
  _started = _sensor.begin(BMP085_MODE_STANDARD);
  if (!_started) {
    Logger::warn("GY801", "BMP180/BMP085 library initialization failed");
    return false;
  }
  Logger::info("GY801", "BMP180/BMP085 verified at 0x77");
  return true;
}

void BMP180Barometer::update(uint32_t nowMs) {
  if (!_started) {
    if (static_cast<uint32_t>(nowMs - _lastBeginAttemptMs) >= SENSOR_RETRY_INTERVAL_MS) {
      begin();
    }
    return;
  }
  if (static_cast<uint32_t>(nowMs - _lastReadMs) < BARO_READ_INTERVAL_MS) {
    return;
  }
  _lastReadMs = nowMs;

  if (!probe()) {
    _data.valid = false;
    _started = false;
    _lastBeginAttemptMs = nowMs;
    Logger::warn("GY801", "BMP180/BMP085 stopped responding; retry scheduled");
    return;
  }

  sensors_event_t event;
  _sensor.getEvent(&event);
  float temperature = NAN;
  _sensor.getTemperature(&temperature);
  if (!std::isfinite(event.pressure) || event.pressure <= 0.0F ||
      !std::isfinite(temperature)) {
    _data.valid = false;
    return;
  }

  _data.rawPressureHpa = event.pressure;
  _data.pressureHpa = BarometerMath::applyPressureOffset(
      _data.rawPressureHpa, _pressureOffsetHpa);
  _data.temperatureC = temperature;
  _data.seaLevelPressureHpa = _seaLevelPressureHpa;
  _data.calibrationSource = _calibrationSource;
  _data.altitudeM = BarometerMath::altitudeFromPressure(
      _data.pressureHpa, _data.seaLevelPressureHpa);
  _data.updatedAtMs = nowMs;
  _data.valid = std::isfinite(_data.pressureHpa) &&
                std::isfinite(_data.altitudeM);
}

bool BMP180Barometer::isValid() const {
  return _data.valid;
}

const BarometerData& BMP180Barometer::getData() const {
  return _data;
}

bool BMP180Barometer::setCalibration(float seaLevelPressureHpa,
                                    float pressureOffsetHpa,
                                    BarometerCalibrationSource source) {
  if (!BarometerMath::isValidSeaLevelPressure(seaLevelPressureHpa) ||
      !std::isfinite(pressureOffsetHpa)) {
    return false;
  }
  _seaLevelPressureHpa = seaLevelPressureHpa;
  _pressureOffsetHpa = pressureOffsetHpa;
  _calibrationSource = source;
  _data.seaLevelPressureHpa = seaLevelPressureHpa;
  _data.calibrationSource = source;
  if (std::isfinite(_data.rawPressureHpa)) {
    _data.pressureHpa = BarometerMath::applyPressureOffset(
        _data.rawPressureHpa, _pressureOffsetHpa);
    _data.altitudeM = BarometerMath::altitudeFromPressure(
        _data.pressureHpa, _seaLevelPressureHpa);
    _data.valid = std::isfinite(_data.pressureHpa) &&
                  std::isfinite(_data.altitudeM) &&
                  std::isfinite(_data.temperatureC);
  }
  return true;
}
