#pragma once

#include <Arduino.h>
#include "acoustic/AcousticTypes.h"

class AcousticDatasetLogger {
 public:
  bool begin();
  bool append(const char* label, const AcousticData& data, uint32_t nowMs);
  bool isReady() const;
  static bool isValidLabel(const char* label);

 private:
  bool _ready = false;
};
