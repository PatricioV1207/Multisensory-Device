#pragma once

#include <cstddef>
#include <cstdint>
#include "acoustic/AcousticTypes.h"

struct AcousticMessageContext {
  const char* deviceId = "";
  const char* vehicleId = "";
  uint32_t bootId = 0;
  uint32_t sequence = 0;
  const char* sampleId = "";
  uint64_t uptimeMs = 0;
  bool timeValid = false;
  const char* measuredAt = "";
  bool simulated = false;
};

class AcousticMessageBuilder {
 public:
  static bool buildAggregate(const AcousticMessageContext& context,
                             const AcousticData& data, char* output,
                             size_t outputSize, size_t* written = nullptr);
  static bool buildEvent(const AcousticMessageContext& context,
                         const AcousticData& data,
                         const AcousticAlertStatus& alert,
                         const char* eventId, char* output,
                         size_t outputSize, size_t* written = nullptr);
};
