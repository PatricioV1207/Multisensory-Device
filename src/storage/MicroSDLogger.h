#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SPI.h>
#include "system/RuntimeStatus.h"

class MicroSDLogger {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool appendJsonLine(const char* json, size_t length, uint32_t nowMs);
  bool isReady() const;
  const StorageStatus& getStatus() const;

 private:
  bool initializeSession();
  bool openSegment();
  bool rotateIfNeeded(size_t nextRecordBytes);
  void markUnavailable(uint32_t nowMs);
  void refreshCapacity();

  SPIClass _spi{VSPI};
  File _file;
  StorageStatus _status;
  char _path[64] = {0};
  uint32_t _lastBeginAttemptMs = 0;
  bool _spiStarted = false;
  bool _sessionInitialized = false;
};
