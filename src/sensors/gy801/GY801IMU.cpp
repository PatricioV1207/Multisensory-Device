#include "sensors/gy801/GY801IMU.h"

#include "utils/Logger.h"

bool GY801IMU::begin() {
  const bool accelOk = _accel.begin();
  const bool gyroOk = _gyro.begin();
  const bool magOk = _mag.begin();
  const bool baroOk = _barometer.begin();
  refreshData();
  Logger::info("GY801", String("Detected components: accel=") + accelOk +
                            " gyro=" + gyroOk + " mag=" + magOk +
                            " baro=" + baroOk);
  return accelOk || gyroOk || magOk || baroOk;
}

void GY801IMU::update(uint32_t nowMs) {
  _accel.update(nowMs);
  _gyro.update(nowMs);
  _mag.update(nowMs);
  _barometer.update(nowMs);
  refreshData();
}

void GY801IMU::refreshData() {
  _data.accel = _accel.getData();
  _data.gyro = _gyro.getData();
  _data.mag = _mag.getData();
  _data.barometer = _barometer.getData();
  _data.imuValid = _data.accel.valid && _data.gyro.valid && _data.mag.valid;
}

bool GY801IMU::isValid() const {
  return _data.imuValid;
}

const GY801Data& GY801IMU::getData() const {
  return _data;
}

