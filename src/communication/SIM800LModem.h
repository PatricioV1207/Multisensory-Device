#pragma once

#include <Arduino.h>
#include <Client.h>
#include <TinyGsmClient.h>
#include "config.h"
#include "system/RuntimeStatus.h"

enum class SIM800LMode : uint8_t {
  AtOnly,
  Gprs,
};

class SIM800LModem {
 public:
  SIM800LModem();
  bool begin(SIM800LMode mode = SIM800LMode::Gprs);
  void update(uint32_t nowMs);
  bool isConfigured() const;
  bool isGprsConnected() const;
  Client& networkClient(bool secure);
  Stream& serialPort();
  const CellularStatus& getStatus() const;

 private:
  enum class State : uint8_t {
    Idle,
    AtSync,
    SimCheck,
    Registering,
    GprsConnecting,
    Online,
    AtReady,
    Backoff,
  };

  void performStep(uint32_t nowMs);
  void updateSignal(uint32_t nowMs);
  void scheduleBackoff(uint32_t nowMs, int16_t errorCode);

  HardwareSerial _serial{1};
  TinyGsm _modem;
  TinyGsmClient _plainClient;
  TinyGsmClientSecure _secureClient;
  CellularStatus _status;
  SIM800LMode _mode = SIM800LMode::Gprs;
  State _state = State::Idle;
  bool _configured = false;
  uint32_t _nextActionMs = 0;
  uint32_t _lastStatusMs = 0;
  uint32_t _backoffMs = RECONNECT_BACKOFF_MIN_MS;
};
