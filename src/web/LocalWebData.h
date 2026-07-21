#pragma once

#include <cstdint>
#include "system/RuntimeStatus.h"
#include "telemetry/TelemetryData.h"

struct LocalWebData {
  const char* deviceId = "";
  const char* vehicleId = "";
  const char* firmwareVersion = "";
  const char* timeSource = "none";
  bool simulated = false;
  uint64_t uptimeMs = 0;
  DHT11Data dht;
  LightData light;
  GPSData gps;
  AccelData accel;
  GyroData gyro;
  MagData mag;
  BarometerData barometer;
  bool imuValid = false;
  StorageStatus storage;
  CellularStatus cellular;
  WiFiRuntimeStatus wifi;
  MqttRuntimeStatus mqtt;
  OtaStatus ota;
  OfflineDeliveryStatus offline;
  AcousticData acoustic;
  AcousticAlertStatus acousticAlert;
  bool alertsAvailable = false;
};
