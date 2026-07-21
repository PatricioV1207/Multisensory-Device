#pragma once

#include <Arduino.h>
#include "config.h"
#include "acoustic/AcousticClassifier.h"
#include "acoustic/AcousticFeatureAccumulator.h"
#include "acoustic/AudioFeatureExtractor.h"
#include "acoustic/AcousticTypes.h"

class INMP441Microphone {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  AcousticData getData() const;

 private:
  static void taskEntry(void* context);
  void run();
  void publishAggregate(const AudioFeatures& features, uint32_t nowMs);
  void publishInvalid(uint32_t nowMs);

  AudioFeatureExtractor _extractor;
  AcousticFeatureAccumulator _accumulator{ACOUSTIC_AGGREGATE_FRAMES};
  float _normalizedSamples[ACOUSTIC_FRAME_SAMPLES] = {0.0F};
  size_t _normalizedCount = 0U;
  mutable portMUX_TYPE _dataMux = portMUX_INITIALIZER_UNLOCKED;
  AcousticData _data;
  TaskHandle_t _taskHandle = nullptr;
  bool _driverInstalled = false;
  bool _started = false;
  uint32_t _lastBeginAttemptMs = 0;
};
