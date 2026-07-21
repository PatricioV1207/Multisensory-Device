#pragma once

#include <cstdint>
#include "acoustic/AcousticTypes.h"

class AcousticAlertEvaluator {
 public:
  AcousticAlertStatus update(const AcousticData& data, uint32_t nowMs);
  const AcousticAlertStatus& getStatus() const;

 private:
  void clearCandidate();

  AcousticAlertStatus _status;
  bool _candidateActive = false;
  const char* _candidateType = "";
  uint32_t _candidateStartedMs = 0;
  uint32_t _lastEventMs = 0;
};
