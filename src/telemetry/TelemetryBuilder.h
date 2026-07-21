#pragma once

#include <cstddef>
#include "telemetry/TelemetryData.h"

class TelemetryBuilder {
 public:
  static bool build(const TelemetryData& data, char* output, size_t outputSize,
                    size_t* written = nullptr);
  static bool buildV3(const TelemetryData& data, char* output,
                      size_t outputSize, size_t* written = nullptr);
};
