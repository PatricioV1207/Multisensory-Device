#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include "config.h"
#include "acoustic/AcousticAlertEvaluator.h"
#include "acoustic/AcousticMessageBuilder.h"
#include "communication/MQTTClientCustom.h"
#include "communication/SIM800LModem.h"
#include "communication/VehicleSenseMqttClient.h"
#include "communication/WiFiManagerCustom.h"
#include "diagnostics/ModuleTestRunner.h"
#include "sensors/DHT11Sensor.h"
#include "sensors/GPSNeo6M.h"
#include "sensors/INMP441Microphone.h"
#include "sensors/BH1750Sensor.h"
#include "sensors/gy801/GY801IMU.h"
#include "storage/MicroSDLogger.h"
#include "storage/OfflineTelemetryQueue.h"
#include "system/DeviceIdentity.h"
#include "system/TimeService.h"
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
  void processVehicleSenseMqtt(uint32_t nowMs);
  void processAcoustic(uint32_t nowMs);
  void acknowledgeUnsupportedCommand(const char* commandId, uint32_t nowMs);

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
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  INMP441Microphone _microphone;
  OfflineTelemetryQueue _offlineQueue;
  VehicleSenseMqttClient _secureMqtt;
  AcousticAlertEvaluator _acousticAlerts;
  DeviceIdentity _identity;
  TimeService _time;
#endif
#if APP_MODE != APP_MODE_FULL_PROTOTYPE && \
    APP_MODE != APP_MODE_FULL_PROTOTYPE_CELLULAR && \
    APP_MODE != APP_MODE_VEHICLESENSE_WIFI
  ModuleTestRunner _testRunner;
#endif
  TelemetryData _telemetry;
  MqttRuntimeStatus _mqttStatus;
  uint32_t _lastTelemetryMs = 0;
  uint32_t _lastWebSnapshotMs = 0;
  char _payload[TELEMETRY_PAYLOAD_BUFFER_SIZE] = {0};
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  uint32_t _lastReplayAttemptMs = 0;
  uint32_t _lastAcousticProcessedMs = 0;
  char _replayPayload[TELEMETRY_PAYLOAD_BUFFER_SIZE] = {0};
  char _sampleId[128] = {0};
  char _measuredAt[32] = {0};
  char _commandBuffer[MQTT_COMMAND_BUFFER_SIZE] = {0};
  char _commandAckPayload[768] = {0};
  char _acousticPayload[TELEMETRY_PAYLOAD_BUFFER_SIZE] = {0};
  char _acousticSampleId[176] = {0};
  char _acousticBaseSampleId[144] = {0};
  char _acousticMeasuredAt[32] = {0};
  char _eventId[176] = {0};
  AcousticAlertStatus _acousticAlert;
#endif
};
