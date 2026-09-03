#pragma once

#include <Arduino.h>
#include <stdint.h>

struct INMP441RawSlotStats {
  uint32_t samples = 0U;
  uint32_t nonzero = 0U;
  uint32_t changes = 0U;
  int32_t minimum = 0;
  int32_t maximum = 0;
  uint64_t absoluteSum = 0U;
};

struct INMP441RawSnapshot {
  bool driverReady = false;
  uint32_t successfulReads = 0U;
  uint32_t failedReads = 0U;
  uint32_t bytesRead = 0U;
  INMP441RawSlotStats slots[2];
};

class INMP441RawDiagnostic {
 public:
  bool begin();
  void update();
  INMP441RawSnapshot takeSnapshot();

 private:
  struct SlotAccumulator {
    uint32_t samples = 0U;
    uint32_t nonzero = 0U;
    uint32_t changes = 0U;
    int32_t minimum = INT32_MAX;
    int32_t maximum = INT32_MIN;
    int32_t previous = 0;
    uint64_t absoluteSum = 0U;
    bool hasPrevious = false;
  };

  static void addSample(SlotAccumulator& slot, int32_t sample);
  static INMP441RawSlotStats finishSlot(const SlotAccumulator& slot);
  void resetCounters();

  SlotAccumulator _slots[2];
  uint32_t _successfulReads = 0U;
  uint32_t _failedReads = 0U;
  uint32_t _bytesRead = 0U;
  bool _driverInstalled = false;
  bool _started = false;
};
