#pragma once

#include <cstddef>
#include "web/LocalWebData.h"

class LocalWebJsonBuilder {
 public:
  static bool buildStatus(const LocalWebData& data, char* output,
                          size_t outputSize);
  static bool buildBasicTelemetry(const LocalWebData& data, char* output,
                                  size_t outputSize);
};
