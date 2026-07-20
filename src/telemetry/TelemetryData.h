#pragma once

#include <cmath>
#include <cstdint>
#include "system/RuntimeStatus.h"

struct DHT11Data {
  float temperatureC = NAN;
  float humidityPercent = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
};

struct LightData {
  float lux = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
};

struct GPSData {
  double latitude = NAN;
  double longitude = NAN;
  float altitudeM = NAN;
  float speedKmh = NAN;
  uint32_t satellites = 0;
  bool valid = false;
  bool streamSeen = false;
  uint32_t charsProcessed = 0;
  uint32_t updatedAtMs = 0;
  float hdop = NAN;
};

struct AccelData {
  float x = NAN;
  float y = NAN;
  float z = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
  float rawX = NAN;
  float rawY = NAN;
  float rawZ = NAN;
};

struct GyroData {
  float x = NAN;
  float y = NAN;
  float z = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
};

struct MagData {
  float x = NAN;
  float y = NAN;
  float z = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
};

enum class BarometerCalibrationSource : uint8_t {
  Default = 0,
  KnownAltitude = 1,
  GPS = 2,
};

struct BarometerData {
  float pressureHpa = NAN;
  float temperatureC = NAN;
  float altitudeM = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
  float rawPressureHpa = NAN;
  float seaLevelPressureHpa = NAN;
  BarometerCalibrationSource calibrationSource =
      BarometerCalibrationSource::Default;
};

struct GY801Data {
  AccelData accel;
  GyroData gyro;
  MagData mag;
  BarometerData barometer;
  bool imuValid = false;
};

struct TelemetryData {
  const char* deviceId = "";
  uint64_t uptimeMs = 0;
  DHT11Data dht;
  GPSData gps;
  GY801Data gy801;
  LightData light;
  StorageStatus storage;
  CellularStatus cellular;
};
