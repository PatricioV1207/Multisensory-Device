#pragma once

#include "acoustic/AcousticTypes.h"

class AcousticClassifier {
 public:
  static constexpr const char* VERSION = "heuristic-1";

  static AcousticClassification classify(const AudioFeatures& features);
};
