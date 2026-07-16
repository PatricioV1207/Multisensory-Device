#pragma once

#include <Arduino.h>
#include "sensors/gy801/ADXL345Accel.h"
#include "sensors/gy801/BMP180Barometer.h"
#include "sensors/gy801/HMC5883LMag.h"
#include "sensors/gy801/L3G4200DGyro.h"
#include "telemetry/TelemetryData.h"

class GY801IMU {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  const GY801Data& getData() const;
  bool setBarometerCalibration(float seaLevelPressureHpa,
                               float pressureOffsetHpa,
                               BarometerCalibrationSource source);

 private:
  void refreshData();

  ADXL345Accel _accel;
  L3G4200DGyro _gyro;
  HMC5883LMag _mag;
  BMP180Barometer _barometer;
  GY801Data _data;
};
