#include "communication/MQTTClientCustom.h"

#include <WiFi.h>
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
}  // namespace

bool MQTTClientCustom::begin(Client& networkClient) {
  _networkClient = &networkClient;
  _configured = hasRealValue(MQTT_HOST) && MQTT_PORT > 0 &&
                hasRealValue(MQTT_TOPIC);
  if (!_configured) {
    Logger::error("CONFIG", "MQTT host/topic missing in secrets.h");
    return false;
  }
  _client.setClient(networkClient);
  _client.setServer(MQTT_HOST, MQTT_PORT);
  _client.setBufferSize(MQTT_BUFFER_SIZE);
  _client.setKeepAlive(MQTT_KEEPALIVE_SECONDS);
  _client.setSocketTimeout(MQTT_SOCKET_TIMEOUT_SECONDS);
  return true;
}

void MQTTClientCustom::update(uint32_t nowMs, bool networkReady) {
  if (!_configured || _networkClient == nullptr) {
    return;
  }
  if (!networkReady) {
    if (_client.connected()) {
      _client.disconnect();
    }
    _wasConnected = false;
    return;
  }

  if (_client.connected()) {
    _client.loop();
    if (!_wasConnected) {
      Logger::info("MQTT", "Connected to broker");
    }
    _wasConnected = true;
    _backoffMs = RECONNECT_BACKOFF_MIN_MS;
    return;
  }

  if (_wasConnected) {
    Logger::warn("MQTT", "Broker connection lost");
    _wasConnected = false;
  }

  if (reached(nowMs, _nextAttemptMs)) {
    if (connect()) {
      _backoffMs = RECONNECT_BACKOFF_MIN_MS;
      _nextAttemptMs = nowMs;
    } else {
      Logger::warn("MQTT", "Connect failed; state=" + String(_client.state()));
      _nextAttemptMs = nowMs + _backoffMs;
      increaseBackoff();
    }
  }
}

bool MQTTClientCustom::connect() {
  const uint64_t chipId = ESP.getEfuseMac();
  char clientId[64];
  snprintf(clientId, sizeof(clientId), "%s-%04X", DEVICE_ID,
           static_cast<unsigned int>(chipId & 0xFFFFU));

  if (MQTT_USERNAME[0] == '\0') {
    return _client.connect(clientId);
  }
  return _client.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD);
}

void MQTTClientCustom::increaseBackoff() {
  if (_backoffMs < RECONNECT_BACKOFF_MAX_MS) {
    _backoffMs *= 2UL;
    if (_backoffMs > RECONNECT_BACKOFF_MAX_MS) {
      _backoffMs = RECONNECT_BACKOFF_MAX_MS;
    }
  }
}

bool MQTTClientCustom::publish(const char* payload) {
  if (!_client.connected() || payload == nullptr || payload[0] == '\0') {
    return false;
  }
  const bool ok = _client.publish(MQTT_TOPIC, payload, false);
  if (!ok) {
    Logger::warn("MQTT", "Publish rejected; verify packet buffer and broker");
  }
  return ok;
}

bool MQTTClientCustom::isConfigured() const {
  return _configured;
}

bool MQTTClientCustom::isConnected() {
  return _client.connected();
}

int MQTTClientCustom::state() {
  return _client.state();
}
