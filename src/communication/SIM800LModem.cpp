#include "communication/SIM800LModem.h"

#include <cstring>
#include "config.h"
#include "pins.h"
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

SIM800LModem::SIM800LModem()
    : _modem(_serial),
      _plainClient(_modem, 0),
      _secureClient(_modem, 1) {}

bool SIM800LModem::begin(SIM800LMode mode) {
  _mode = mode;
  _configured = mode == SIM800LMode::AtOnly || hasRealValue(CELLULAR_APN);
  _serial.begin(SIM800_BAUD, SERIAL_8N1, PIN_SIM800_RX, PIN_SIM800_TX);
  _state = State::AtSync;
  _nextActionMs = millis();
  _backoffMs = RECONNECT_BACKOFF_MIN_MS;
  _status = CellularStatus{};
  Logger::info("SIM800", "UART1 started at " + String(SIM800_BAUD) +
                             " baud on RX16/TX17");
  if (!_configured) {
    Logger::error("CONFIG", "CELLULAR_APN is missing in secrets.h");
  }
  return true;
}

void SIM800LModem::update(uint32_t nowMs) {
  if (_state == State::Idle || !reached(nowMs, _nextActionMs)) {
    return;
  }
  performStep(nowMs);
}

void SIM800LModem::performStep(uint32_t nowMs) {
  switch (_state) {
    case State::AtSync:
      if (_modem.testAT(1000)) {
        _status.modemPresent = true;
        _status.lastError = 0;
        const String modemName = _modem.getModemName();
        modemName.toCharArray(_status.modemName, sizeof(_status.modemName));
        _state = State::SimCheck;
        _nextActionMs = nowMs;
        Logger::info("SIM800", "AT response received");
      } else {
        _status.modemPresent = false;
        scheduleBackoff(nowMs, 1);
      }
      break;

    case State::SimCheck: {
      SimStatus simStatus = _modem.getSimStatus();
      if (simStatus != SIM_READY && hasRealValue(CELLULAR_SIM_PIN)) {
        _modem.simUnlock(CELLULAR_SIM_PIN);
        simStatus = _modem.getSimStatus();
      }
      _status.simReady = simStatus == SIM_READY;
      updateSignal(nowMs);
      if (!_status.simReady) {
        scheduleBackoff(nowMs, 2);
      } else if (_mode == SIM800LMode::AtOnly) {
        _state = State::AtReady;
        _nextActionMs = nowMs + SIM800_STATUS_INTERVAL_MS;
        Logger::info("SIM800", "SIM ready; AT-only diagnostic active");
      } else {
        _state = State::Registering;
        _nextActionMs = nowMs;
      }
      break;
    }

    case State::Registering:
      _status.networkRegistered = _modem.isNetworkConnected();
      updateSignal(nowMs);
      if (_status.networkRegistered) {
        const String operatorName = _modem.getOperator();
        operatorName.toCharArray(_status.operatorName,
                                 sizeof(_status.operatorName));
        _state = State::GprsConnecting;
        _nextActionMs = nowMs;
        Logger::info("SIM800", "Registered on GSM network");
      } else {
        _nextActionMs = nowMs + SIM800_ACTION_INTERVAL_MS;
      }
      break;

    case State::GprsConnecting:
      if (!_configured) {
        scheduleBackoff(nowMs, 3);
      } else if (_modem.gprsConnect(CELLULAR_APN, CELLULAR_APN_USER,
                                    CELLULAR_APN_PASSWORD)) {
        _status.gprsConnected = true;
        const String localIp = _modem.getLocalIP();
        localIp.toCharArray(_status.localIp, sizeof(_status.localIp));
        _state = State::Online;
        _nextActionMs = nowMs + SIM800_STATUS_INTERVAL_MS;
        _backoffMs = RECONNECT_BACKOFF_MIN_MS;
        Logger::info("SIM800", "GPRS context connected");
      } else {
        scheduleBackoff(nowMs, 4);
      }
      break;

    case State::Online:
      _status.networkRegistered = _modem.isNetworkConnected();
      _status.gprsConnected = _modem.isGprsConnected();
      updateSignal(nowMs);
      if (!_status.networkRegistered || !_status.gprsConnected) {
        _plainClient.stop();
        _secureClient.stop();
        _modem.gprsDisconnect();
        scheduleBackoff(nowMs, 5);
      } else {
        _nextActionMs = nowMs + SIM800_STATUS_INTERVAL_MS;
      }
      break;

    case State::AtReady:
      _status.simReady = _modem.getSimStatus() == SIM_READY;
      updateSignal(nowMs);
      _nextActionMs = nowMs + SIM800_STATUS_INTERVAL_MS;
      break;

    case State::Backoff:
      _state = State::AtSync;
      _nextActionMs = nowMs;
      break;

    case State::Idle:
    default:
      break;
  }
  _status.updatedAtMs = nowMs;
}

void SIM800LModem::updateSignal(uint32_t nowMs) {
  if (_lastStatusMs != 0 &&
      static_cast<uint32_t>(nowMs - _lastStatusMs) <
          SIM800_STATUS_INTERVAL_MS) {
    return;
  }
  _lastStatusMs = nowMs;
  const int16_t quality = _modem.getSignalQuality();
  _status.signalQuality = quality >= 0 && quality <= 31 ? quality : -1;
  _status.signalDbm = _status.signalQuality >= 0
                          ? static_cast<int16_t>(-113 + 2 * quality)
                          : 0;
}

void SIM800LModem::scheduleBackoff(uint32_t nowMs, int16_t errorCode) {
  _status.gprsConnected = false;
  _status.localIp[0] = '\0';
  _status.lastError = errorCode;
  _state = State::Backoff;
  _nextActionMs = nowMs + _backoffMs;
  Logger::warn("SIM800", "State failed; error=" + String(errorCode) +
                             " retry_ms=" + String(_backoffMs));
  _backoffMs *= 2UL;
  if (_backoffMs > RECONNECT_BACKOFF_MAX_MS) {
    _backoffMs = RECONNECT_BACKOFF_MAX_MS;
  }
}

bool SIM800LModem::isConfigured() const {
  return _configured;
}

bool SIM800LModem::isGprsConnected() const {
  return _status.gprsConnected;
}

Client& SIM800LModem::networkClient(bool secure) {
  return secure ? static_cast<Client&>(_secureClient)
                : static_cast<Client&>(_plainClient);
}

Stream& SIM800LModem::serialPort() {
  return _serial;
}

const CellularStatus& SIM800LModem::getStatus() const {
  return _status;
}
