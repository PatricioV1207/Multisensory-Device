#pragma once

#include <cmath>
#include <cstdint>

struct DHT11Data {
  float temperatureC = NAN;
  float humidityPercent = NAN;
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
};

struct AccelData {
  float x = NAN;
  float y = NAN;
  float z = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
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

struct BarometerData {
  float pressureHpa = NAN;
  float temperatureC = NAN;
  float altitudeM = NAN;
  bool valid = false;
  uint32_t updatedAtMs = 0;
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
};

