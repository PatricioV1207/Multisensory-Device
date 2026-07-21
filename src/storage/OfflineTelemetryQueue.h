#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include "telemetry/TelemetryData.h"

class OfflineTelemetryQueue {
 public:
  bool begin(uint32_t currentBootId);
  void update(uint32_t nowMs, uint64_t nowEpochSeconds, bool storageReady);

  bool enqueue(const char* payload, size_t length, uint32_t nowMs,
               uint64_t nowEpochSeconds, bool deferred);
  bool peek(char* output, size_t outputSize, size_t* written,
            uint32_t* recordId, bool* replayed);
  bool markAttempted(uint32_t recordId, bool replayed);
  bool acknowledge(uint32_t recordId);

  bool isReady() const;
  bool hasPending() const;
  const OfflineDeliveryStatus& getStatus() const;

 private:
  bool scanRecords();
  bool findOldest();
  bool ensureCapacity(size_t nextRecordBytes);
  bool removeOldest(bool countAsDropped);
  bool readHeader(uint32_t recordId, void* header, size_t headerSize,
                  size_t* fileSize = nullptr) const;
  void refreshOldestAge(uint64_t nowEpochSeconds);
  void setUnavailable(uint32_t nowMs);

  OfflineDeliveryStatus _status;
  uint32_t _currentBootId = 0;
  uint32_t _nextRecordId = 1;
  uint32_t _oldestRecordId = 0;
  uint32_t _lastBeginAttemptMs = 0;
  uint32_t _lastMaintenanceMs = 0;
};
