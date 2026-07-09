#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class WiFiManagerCustom {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isConfigured() const;
  bool isConnected() const;
  int32_t rssi() const;
  String localIp() const;

 private:
  void startConnection(uint32_t nowMs);
  void increaseBackoff();

  bool _configured = false;
  bool _connecting = false;
  bool _wasConnected = false;
  uint32_t _attemptStartedMs = 0;
  uint32_t _nextAttemptMs = 0;
  uint32_t _backoffMs = RECONNECT_BACKOFF_MIN_MS;
};
