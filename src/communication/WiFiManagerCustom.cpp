#include "communication/WiFiManagerCustom.h"

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

bool WiFiManagerCustom::begin() {
  _configured = hasRealValue(WIFI_SSID);
  if (!_configured) {
    Logger::error("CONFIG", "WiFi credentials missing; copy secrets_example.h to secrets.h");
    return false;
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  startConnection(millis());
  return true;
}

void WiFiManagerCustom::startConnection(uint32_t nowMs) {
  Logger::info("WIFI", "Starting connection to configured SSID");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  _attemptStartedMs = nowMs;
  _connecting = true;
}

void WiFiManagerCustom::update(uint32_t nowMs) {
  if (!_configured) {
    return;
  }

  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    if (!_wasConnected) {
      Logger::info("WIFI", "Connected; IP=" + localIp() + " RSSI=" + String(rssi()));
    }
    _wasConnected = true;
    _connecting = false;
    _backoffMs = RECONNECT_BACKOFF_MIN_MS;
    return;
  }

  if (_wasConnected) {
    Logger::warn("WIFI", "Connection lost; acquisition will continue");
    _wasConnected = false;
    _nextAttemptMs = nowMs + _backoffMs;
  }

  if (_connecting &&
      static_cast<uint32_t>(nowMs - _attemptStartedMs) >= WIFI_CONNECT_TIMEOUT_MS) {
    _connecting = false;
    WiFi.disconnect();
    Logger::warn("WIFI", "Connection attempt timed out");
    _nextAttemptMs = nowMs + _backoffMs;
    increaseBackoff();
  }

  if (!_connecting && reached(nowMs, _nextAttemptMs)) {
    startConnection(nowMs);
  }
}

void WiFiManagerCustom::increaseBackoff() {
  if (_backoffMs < RECONNECT_BACKOFF_MAX_MS) {
    _backoffMs *= 2UL;
    if (_backoffMs > RECONNECT_BACKOFF_MAX_MS) {
      _backoffMs = RECONNECT_BACKOFF_MAX_MS;
    }
  }
}

bool WiFiManagerCustom::isConfigured() const {
  return _configured;
}

bool WiFiManagerCustom::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

int32_t WiFiManagerCustom::rssi() const {
  return isConnected() ? WiFi.RSSI() : 0;
}

String WiFiManagerCustom::localIp() const {
  return isConnected() ? WiFi.localIP().toString() : String("0.0.0.0");
}

