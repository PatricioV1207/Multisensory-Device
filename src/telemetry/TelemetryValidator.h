#pragma once

#include <cstdint>
#include "telemetry/TelemetryData.h"

class TelemetryValidator {
 public:
  static void validate(TelemetryData& data, uint32_t nowMs);

 private:
  static bool isFresh(uint32_t nowMs, uint32_t updatedAtMs, uint32_t maxAgeMs);
};

