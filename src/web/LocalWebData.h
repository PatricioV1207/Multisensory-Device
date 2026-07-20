#pragma once

#include <cstdint>
#include "system/RuntimeStatus.h"
#include "telemetry/TelemetryData.h"

struct LocalWebData {
  const char* deviceId = "";
  const char* firmwareVersion = "";
  uint64_t uptimeMs = 0;
  DHT11Data dht;
  LightData light;
  AccelData accel;
  GyroData gyro;
  MagData mag;
  BarometerData barometer;
  bool imuValid = false;
  StorageStatus storage;
  CellularStatus cellular;
  MqttRuntimeStatus mqtt;
  OtaStatus ota;
};
