#pragma once

#include <cstdint>

struct StorageStatus {
  bool initialized = false;
  bool mounted = false;
  bool lastWriteOk = false;
  uint64_t totalBytes = 0;
  uint64_t usedBytes = 0;
  uint64_t freeBytes = 0;
  uint32_t lastWriteMs = 0;
  uint32_t bootSession = 0;
  uint16_t segment = 0;
};

struct CellularStatus {
  bool modemPresent = false;
  bool simReady = false;
  bool networkRegistered = false;
  bool gprsConnected = false;
  int16_t signalQuality = -1;
  int16_t signalDbm = 0;
  int16_t lastError = 0;
  uint32_t updatedAtMs = 0;
  char modemName[32] = {0};
  char operatorName[24] = {0};
  char localIp[24] = {0};
};

struct MqttRuntimeStatus {
  bool configured = false;
  bool connected = false;
  bool reconnecting = false;
  bool lastPublishOk = false;
  int16_t clientState = 0;
  uint32_t lastPublishAttemptMs = 0;
  uint32_t lastPublishSuccessMs = 0;
  uint32_t lastPublishAckMs = 0;
  uint32_t lastAcknowledgedToken = 0;
};

struct WiFiRuntimeStatus {
  bool configured = false;
  bool connected = false;
  bool accessPointRunning = false;
  int32_t rssiDbm = 0;
  char stationIp[16] = {0};
  char accessPointIp[16] = {0};
};

struct OtaStatus {
  bool enabled = false;
  bool inProgress = false;
  bool lastUpdateOk = false;
  uint32_t lastActivityMs = 0;
};
