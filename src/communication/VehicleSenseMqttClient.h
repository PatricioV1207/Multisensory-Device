#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include <mqtt_client.h>
#include "communication/MqttTopics.h"
#include "config.h"

class VehicleSenseMqttClient {
 public:
  bool begin(uint32_t bootId);
  void update(uint32_t nowMs, bool networkReady);

  bool publishTelemetry(const char* payload, size_t length, uint32_t token);
  bool publishCommandAck(const char* payload, size_t length);
  bool publishStatus();
  bool takeTelemetryAck(uint32_t& token, uint32_t& acknowledgedAtMs);
  bool takeCommand(char* output, size_t outputSize, size_t* written = nullptr);

  bool isConfigured() const;
  bool isStarted() const;
  bool isConnected() const;
  bool isReconnecting() const;
  int32_t lastError() const;
  const MqttTopics& topics() const;

 private:
  static esp_err_t eventHandler(esp_mqtt_event_handle_t event);
  esp_err_t handleEvent(esp_mqtt_event_handle_t event);
  void handleConnected();
  void handleDisconnected(uint32_t nowMs);
  void handlePublished(int messageId, uint32_t nowMs);
  void handleIncomingData(esp_mqtt_event_handle_t event);
  void scheduleReconnect(uint32_t nowMs);
  void increaseBackoff();
  bool buildIdentityAndStatus(uint32_t bootId);
  bool publishTo(const char* topic, const char* payload, size_t length,
                 int qos, bool retain, int* messageId = nullptr);

  esp_mqtt_client_handle_t _client = nullptr;
  MqttTopics _topics;
  bool _configured = false;
  bool _started = false;
  volatile bool _connected = false;
  volatile bool _attemptInProgress = false;
  volatile int32_t _lastError = 0;
  uint32_t _nextAttemptMs = 0;
  uint32_t _attemptStartedMs = 0;
  uint32_t _backoffMs = RECONNECT_BACKOFF_MIN_MS;

  char _clientId[96] = {0};
  char _offlineStatus[384] = {0};
  char _onlineStatus[384] = {0};

  portMUX_TYPE _sharedMux = portMUX_INITIALIZER_UNLOCKED;
  int _pendingTelemetryMessageId = -1;
  uint32_t _pendingTelemetryToken = 0;
  bool _telemetryAckReady = false;
  uint32_t _acknowledgedToken = 0;
  uint32_t _acknowledgedAtMs = 0;

  bool _commandReceiving = false;
  bool _commandReady = false;
  size_t _commandLength = 0;
  char _commandBuffer[MQTT_COMMAND_BUFFER_SIZE] = {0};
};
