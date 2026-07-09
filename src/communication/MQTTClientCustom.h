#pragma once

#include <Arduino.h>
#include <Client.h>
#include <PubSubClient.h>
#include "config.h"

class MQTTClientCustom {
 public:
  bool begin(Client& networkClient);
  void update(uint32_t nowMs, bool networkReady);
  bool publish(const char* payload);
  bool isConfigured() const;
  bool isConnected();
  int state();

 private:
  bool connect();
  void increaseBackoff();

  PubSubClient _client;
  Client* _networkClient = nullptr;
  bool _configured = false;
  bool _wasConnected = false;
  uint32_t _nextAttemptMs = 0;
  uint32_t _backoffMs = RECONNECT_BACKOFF_MIN_MS;
};
