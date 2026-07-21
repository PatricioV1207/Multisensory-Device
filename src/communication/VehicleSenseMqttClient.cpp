#include "communication/VehicleSenseMqttClient.h"

#include <esp_crt_bundle.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "utils/Logger.h"

namespace {
bool hasRealValue(const char* value) {
  return value != nullptr && value[0] != '\0' &&
         strncmp(value, "YOUR_", 5) != 0;
}

bool reached(uint32_t nowMs, uint32_t deadlineMs) {
  return static_cast<int32_t>(nowMs - deadlineMs) >= 0;
}
}

bool VehicleSenseMqttClient::begin(uint32_t bootId) {
  if (!hasRealValue(HIVEMQ_HOST) || HIVEMQ_PORT == 0U ||
      !hasRealValue(HIVEMQ_USERNAME) || !hasRealValue(HIVEMQ_PASSWORD)) {
    Logger::error("CONFIG", "HiveMQ host/credentials missing in secrets.h");
    return false;
  }
  if (!_topics.build(VEHICLE_ID, DEVICE_ID) ||
      !buildIdentityAndStatus(bootId)) {
    Logger::error("MQTT", "Invalid VehicleSense identity or topic length");
    return false;
  }

  esp_mqtt_client_config_t mqttConfig = {};
  mqttConfig.event_handle = &VehicleSenseMqttClient::eventHandler;
  mqttConfig.host = HIVEMQ_HOST;
  mqttConfig.port = HIVEMQ_PORT;
  mqttConfig.client_id = _clientId;
  mqttConfig.username = HIVEMQ_USERNAME;
  mqttConfig.password = HIVEMQ_PASSWORD;
  mqttConfig.lwt_topic = _topics.status();
  mqttConfig.lwt_msg = _offlineStatus;
  mqttConfig.lwt_qos = 1;
  mqttConfig.lwt_retain = 1;
  mqttConfig.lwt_msg_len = strlen(_offlineStatus);
  mqttConfig.keepalive = MQTT_KEEPALIVE_SECONDS;
  mqttConfig.disable_auto_reconnect = true;
  mqttConfig.user_context = this;
  mqttConfig.task_stack = VEHICLESENSE_MQTT_TASK_STACK_SIZE;
  mqttConfig.buffer_size = VEHICLESENSE_MQTT_BUFFER_SIZE;
  mqttConfig.out_buffer_size = VEHICLESENSE_MQTT_BUFFER_SIZE;
  mqttConfig.transport = MQTT_TRANSPORT_OVER_SSL;
  mqttConfig.crt_bundle_attach = arduino_esp_crt_bundle_attach;
  mqttConfig.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
  mqttConfig.skip_cert_common_name_check = false;
  mqttConfig.network_timeout_ms = MQTT_SOCKET_TIMEOUT_SECONDS * 1000U;
  mqttConfig.reconnect_timeout_ms = RECONNECT_BACKOFF_MIN_MS;

  _client = esp_mqtt_client_init(&mqttConfig);
  if (_client == nullptr) {
    Logger::error("MQTT", "ESP-MQTT initialization failed");
    return false;
  }
  _configured = true;
  _nextAttemptMs = 0U;
  Logger::info("MQTT", "HiveMQ TLS configured; hostname verification enabled");
  return true;
}

bool VehicleSenseMqttClient::buildIdentityAndStatus(uint32_t bootId) {
  const uint64_t chipId = ESP.getEfuseMac();
  const int clientCount =
      snprintf(_clientId, sizeof(_clientId), "vs-%s-%08lX", DEVICE_ID,
               static_cast<unsigned long>(chipId & 0xFFFFFFFFULL));
  const int offlineCount = snprintf(
      _offlineStatus, sizeof(_offlineStatus),
      "{\"schema_version\":1,\"message_type\":\"status\","
      "\"device_id\":\"%s\",\"vehicle_id\":\"%s\",\"boot_id\":%lu,"
      "\"state\":\"offline\",\"reason\":\"unexpected_disconnect\","
      "\"simulated\":false}",
      DEVICE_ID, VEHICLE_ID, static_cast<unsigned long>(bootId));
  const int onlineCount = snprintf(
      _onlineStatus, sizeof(_onlineStatus),
      "{\"schema_version\":1,\"message_type\":\"status\","
      "\"device_id\":\"%s\",\"vehicle_id\":\"%s\",\"boot_id\":%lu,"
      "\"state\":\"online\",\"reason\":\"connected\","
      "\"firmware_version\":\"%s\",\"simulated\":false}",
      DEVICE_ID, VEHICLE_ID, static_cast<unsigned long>(bootId),
      FIRMWARE_VERSION);
  return clientCount > 0 &&
         static_cast<size_t>(clientCount) < sizeof(_clientId) &&
         offlineCount > 0 &&
         static_cast<size_t>(offlineCount) < sizeof(_offlineStatus) &&
         onlineCount > 0 &&
         static_cast<size_t>(onlineCount) < sizeof(_onlineStatus);
}

void VehicleSenseMqttClient::update(uint32_t nowMs, bool networkReady) {
  expirePendingTelemetry(nowMs);
  if (!_configured || _client == nullptr || !networkReady) {
    return;
  }

  if (!_started) {
    if (!reached(nowMs, _nextAttemptMs)) {
      return;
    }
    const esp_err_t result = esp_mqtt_client_start(_client);
    if (result == ESP_OK) {
      _started = true;
      _attemptInProgress = true;
      _attemptStartedMs = nowMs;
      Logger::info("MQTT", "Starting TLS connection to HiveMQ");
    } else {
      _lastError = result;
      scheduleReconnect(nowMs);
    }
    return;
  }

  if (!_connected && !_attemptInProgress && reached(nowMs, _nextAttemptMs)) {
    const esp_err_t result = esp_mqtt_client_reconnect(_client);
    if (result == ESP_OK) {
      _attemptInProgress = true;
      _attemptStartedMs = nowMs;
      Logger::info("MQTT", "Reconnecting to HiveMQ");
    } else {
      _lastError = result;
      scheduleReconnect(nowMs);
    }
  }
}

esp_err_t VehicleSenseMqttClient::eventHandler(
    esp_mqtt_event_handle_t event) {
  if (event == nullptr || event->user_context == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  return static_cast<VehicleSenseMqttClient*>(event->user_context)
      ->handleEvent(event);
}

esp_err_t VehicleSenseMqttClient::handleEvent(
    esp_mqtt_event_handle_t event) {
  const uint32_t nowMs = millis();
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      handleConnected();
      break;
    case MQTT_EVENT_DISCONNECTED:
      handleDisconnected(nowMs);
      break;
    case MQTT_EVENT_PUBLISHED:
      handlePublished(event->msg_id, nowMs);
      break;
    case MQTT_EVENT_DATA:
      handleIncomingData(event);
      break;
    case MQTT_EVENT_ERROR:
      if (event->error_handle != nullptr) {
        if (event->error_handle->error_type ==
            MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
          _lastError = 100 + event->error_handle->connect_return_code;
        } else {
          _lastError = event->error_handle->esp_tls_last_esp_err;
          Logger::warn(
              "MQTT", "TLS/network error=" + String(_lastError) +
                          " verify_flags=" +
                          String(event->error_handle->esp_tls_cert_verify_flags,
                                 HEX));
        }
      }
      break;
    default:
      break;
  }
  return ESP_OK;
}

void VehicleSenseMqttClient::handleConnected() {
  _connected = true;
  _attemptInProgress = false;
  _backoffMs = RECONNECT_BACKOFF_MIN_MS;
  _lastError = 0;
  const int subscriptionId =
      esp_mqtt_client_subscribe(_client, _topics.commands(), 1);
  if (subscriptionId < 0) {
    Logger::warn("MQTT", "Command topic subscription rejected");
  }
  if (!publishStatus()) {
    Logger::warn("MQTT", "Retained online status was not queued");
  }
  Logger::info("MQTT", "HiveMQ connected with TLS; command ACL requested");
}

void VehicleSenseMqttClient::handleDisconnected(uint32_t nowMs) {
  const bool wasConnected = _connected;
  _connected = false;
  _attemptInProgress = false;
  scheduleReconnect(nowMs);
  if (wasConnected) {
    Logger::warn("MQTT", "HiveMQ connection lost; local acquisition continues");
  }
}

void VehicleSenseMqttClient::scheduleReconnect(uint32_t nowMs) {
  _nextAttemptMs = nowMs + _backoffMs;
  increaseBackoff();
}

void VehicleSenseMqttClient::increaseBackoff() {
  if (_backoffMs < RECONNECT_BACKOFF_MAX_MS) {
    _backoffMs *= 2U;
    if (_backoffMs > RECONNECT_BACKOFF_MAX_MS) {
      _backoffMs = RECONNECT_BACKOFF_MAX_MS;
    }
  }
}

bool VehicleSenseMqttClient::publishTo(const char* topic, const char* payload,
                                       size_t length, int qos, bool retain,
                                       int* messageId) {
  if (!_connected || topic == nullptr || payload == nullptr || length == 0U ||
      length > static_cast<size_t>(INT_MAX)) {
    return false;
  }
  const int id = esp_mqtt_client_enqueue(
      _client, topic, payload, static_cast<int>(length), qos, retain ? 1 : 0,
      false);
  if (id < 0) {
    return false;
  }
  if (messageId != nullptr) {
    *messageId = id;
  }
  return true;
}

bool VehicleSenseMqttClient::publishTelemetry(const char* payload,
                                              size_t length,
                                              uint32_t token) {
  portENTER_CRITICAL(&_sharedMux);
  const bool busy = _pendingTelemetryMessageId >= 0;
  portEXIT_CRITICAL(&_sharedMux);
  if (busy) {
    return false;
  }

  int messageId = -1;
  if (!publishTo(_topics.telemetry(), payload, length, 1, false, &messageId)) {
    return false;
  }
  portENTER_CRITICAL(&_sharedMux);
  _pendingTelemetryMessageId = messageId;
  _pendingTelemetryToken = token;
  _pendingTelemetryStartedMs = millis();
  portEXIT_CRITICAL(&_sharedMux);
  return true;
}

bool VehicleSenseMqttClient::publishCommandAck(const char* payload,
                                               size_t length) {
  return publishTo(_topics.commandAcks(), payload, length, 1, false);
}

bool VehicleSenseMqttClient::publishAcoustic(const char* payload,
                                             size_t length) {
  return publishTo(_topics.acoustic(), payload, length, 1, false);
}

bool VehicleSenseMqttClient::publishEvent(const char* payload, size_t length) {
  return publishTo(_topics.events(), payload, length, 1, false);
}

bool VehicleSenseMqttClient::publishStatus() {
  return publishTo(_topics.status(), _onlineStatus, strlen(_onlineStatus), 1,
                   true);
}

void VehicleSenseMqttClient::handlePublished(int messageId, uint32_t nowMs) {
  portENTER_CRITICAL(&_sharedMux);
  if (messageId == _pendingTelemetryMessageId) {
    _acknowledgedToken = _pendingTelemetryToken;
    _acknowledgedAtMs = nowMs;
    _telemetryAckReady = true;
    _pendingTelemetryMessageId = -1;
    _pendingTelemetryToken = 0U;
    _pendingTelemetryStartedMs = 0U;
  }
  portEXIT_CRITICAL(&_sharedMux);
}

bool VehicleSenseMqttClient::takeTelemetryAck(
    uint32_t& token, uint32_t& acknowledgedAtMs) {
  bool available = false;
  portENTER_CRITICAL(&_sharedMux);
  if (_telemetryAckReady) {
    token = _acknowledgedToken;
    acknowledgedAtMs = _acknowledgedAtMs;
    _telemetryAckReady = false;
    available = true;
  }
  portEXIT_CRITICAL(&_sharedMux);
  return available;
}

void VehicleSenseMqttClient::handleIncomingData(
    esp_mqtt_event_handle_t event) {
  if (event->current_data_offset == 0) {
    const bool topicMatches =
        event->topic != nullptr &&
        event->topic_len == static_cast<int>(strlen(_topics.commands())) &&
        strncmp(event->topic, _topics.commands(), event->topic_len) == 0;
    portENTER_CRITICAL(&_sharedMux);
    _commandReceiving = topicMatches && !_commandReady &&
                        event->total_data_len > 0 &&
                        static_cast<size_t>(event->total_data_len) <
                            sizeof(_commandBuffer);
    _commandLength = 0U;
    portEXIT_CRITICAL(&_sharedMux);
  }
  if (!_commandReceiving || event->data == nullptr || event->data_len <= 0 ||
      event->current_data_offset < 0) {
    return;
  }

  const size_t offset = static_cast<size_t>(event->current_data_offset);
  const size_t chunk = static_cast<size_t>(event->data_len);
  if (offset + chunk >= sizeof(_commandBuffer)) {
    _commandReceiving = false;
    return;
  }
  memcpy(_commandBuffer + offset, event->data, chunk);
  _commandLength = offset + chunk;
  if (_commandLength == static_cast<size_t>(event->total_data_len)) {
    _commandBuffer[_commandLength] = '\0';
    portENTER_CRITICAL(&_sharedMux);
    _commandReady = true;
    _commandReceiving = false;
    portEXIT_CRITICAL(&_sharedMux);
  }
}

bool VehicleSenseMqttClient::takeCommand(char* output, size_t outputSize,
                                         size_t* written) {
  if (written != nullptr) {
    *written = 0U;
  }
  if (output == nullptr || outputSize == 0U) {
    return false;
  }

  bool available = false;
  portENTER_CRITICAL(&_sharedMux);
  if (_commandReady && _commandLength + 1U <= outputSize) {
    memcpy(output, _commandBuffer, _commandLength + 1U);
    if (written != nullptr) {
      *written = _commandLength;
    }
    _commandReady = false;
    _commandLength = 0U;
    available = true;
  }
  portEXIT_CRITICAL(&_sharedMux);
  return available;
}

bool VehicleSenseMqttClient::isConfigured() const {
  return _configured;
}

bool VehicleSenseMqttClient::isStarted() const {
  return _started;
}

bool VehicleSenseMqttClient::isConnected() const {
  return _connected;
}

bool VehicleSenseMqttClient::isReconnecting() const {
  return _configured && _started && !_connected;
}

bool VehicleSenseMqttClient::hasPendingTelemetry() {
  bool pending = false;
  portENTER_CRITICAL(&_sharedMux);
  pending = _pendingTelemetryMessageId >= 0;
  portEXIT_CRITICAL(&_sharedMux);
  return pending;
}

void VehicleSenseMqttClient::expirePendingTelemetry(uint32_t nowMs) {
  bool expired = false;
  portENTER_CRITICAL(&_sharedMux);
  if (_pendingTelemetryMessageId >= 0 && _pendingTelemetryStartedMs != 0U &&
      static_cast<uint32_t>(nowMs - _pendingTelemetryStartedMs) >=
          MQTT_PUBACK_TIMEOUT_MS) {
    _pendingTelemetryMessageId = -1;
    _pendingTelemetryToken = 0U;
    _pendingTelemetryStartedMs = 0U;
    expired = true;
  }
  portEXIT_CRITICAL(&_sharedMux);
  if (expired) {
    Logger::warn("MQTT", "PUBACK timeout; durable record remains queued");
  }
}

int32_t VehicleSenseMqttClient::lastError() const {
  return _lastError;
}

const MqttTopics& VehicleSenseMqttClient::topics() const {
  return _topics;
}
