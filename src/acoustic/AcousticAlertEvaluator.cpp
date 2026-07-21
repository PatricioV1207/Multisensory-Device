#include "acoustic/AcousticAlertEvaluator.h"

#include <cstring>
#include "config.h"

namespace {
const char* eventForCategory(const char* category) {
  if (strcmp(category, "traffic") == 0) {
    return "acoustic_traffic";
  }
  if (strcmp(category, "horn") == 0) {
    return "acoustic_horn";
  }
  if (strcmp(category, "siren") == 0) {
    return "acoustic_siren";
  }
  return "";
}
}  // namespace

AcousticAlertStatus AcousticAlertEvaluator::update(const AcousticData& data,
                                                    uint32_t nowMs) {
  _status.triggered = false;
  _status.updatedAtMs = nowMs;
  const char* eventType = eventForCategory(data.category);
  const bool eligible = data.microphoneValid && data.analysisValid &&
                        !data.clipping && data.relativeLevelDbfs >=
                                              ACOUSTIC_ALERT_MIN_LEVEL_DBFS &&
                        data.confidence >= ACOUSTIC_ALERT_MIN_CONFIDENCE &&
                        eventType[0] != '\0';
  if (!eligible) {
    clearCandidate();
    _status.active = false;
    _status.durationMs = 0U;
    return _status;
  }

  if (!_candidateActive || strcmp(_candidateType, eventType) != 0) {
    _candidateActive = true;
    _candidateType = eventType;
    _candidateStartedMs = nowMs;
  }
  _status.active = true;
  _status.eventType = eventType;
  _status.severity = strcmp(eventType, "acoustic_siren") == 0 ? "high" :
                     strcmp(eventType, "acoustic_horn") == 0 ? "medium" :
                                                                "medium";
  _status.confidence = data.confidence;
  _status.durationMs = nowMs - _candidateStartedMs;
  const bool cooldownElapsed =
      _lastEventMs == 0U ||
      static_cast<uint32_t>(nowMs - _lastEventMs) >=
          ACOUSTIC_ALERT_COOLDOWN_MS;
  if (_status.durationMs >= ACOUSTIC_ALERT_REQUIRED_DURATION_MS &&
      cooldownElapsed) {
    _status.triggered = true;
    _lastEventMs = nowMs;
  }
  return _status;
}

const AcousticAlertStatus& AcousticAlertEvaluator::getStatus() const {
  return _status;
}

void AcousticAlertEvaluator::clearCandidate() {
  _candidateActive = false;
  _candidateType = "";
  _candidateStartedMs = 0U;
}
