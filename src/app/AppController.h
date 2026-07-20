#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include "config.h"
#include "communication/MQTTClientCustom.h"
#include "communication/SIM800LModem.h"
#include "communication/WiFiManagerCustom.h"
#include "diagnostics/ModuleTestRunner.h"
#include "sensors/DHT11Sensor.h"
#include "sensors/GPSNeo6M.h"
#include "sensors/BH1750Sensor.h"
#include "sensors/gy801/GY801IMU.h"
#include "storage/MicroSDLogger.h"
#include "telemetry/TelemetryData.h"
#include "web/LocalWebServer.h"

class AppController {
 public:
  void begin();
  void update();

 private:
  void buildSnapshot(uint32_t nowMs);
  void publishTelemetry(uint32_t nowMs);
  void refreshLocalWebData(uint32_t nowMs);
  void beginSharedSensors();

  DHT11Sensor _dht;
  GPSNeo6M _gps;
  GY801IMU _gy801;
  BH1750Sensor _light;
  MicroSDLogger _storage;
  SIM800LModem _modem;
  LocalWebServer _web;
  WiFiManagerCustom _wifi;
  WiFiClient _wifiClient;
  MQTTClientCustom _mqtt;
  ModuleTestRunner _testRunner;
  TelemetryData _telemetry;
  MqttRuntimeStatus _mqttStatus;
  uint32_t _lastTelemetryMs = 0;
  uint32_t _lastWebSnapshotMs = 0;
  char _payload[TELEMETRY_PAYLOAD_BUFFER_SIZE] = {0};
};
