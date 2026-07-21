#pragma once

#include <cstddef>
#include <cstdint>
#include "acoustic/AcousticTypes.h"

class AudioFeatureExtractor {
 public:
  static constexpr size_t MAX_SAMPLES = 1024U;

  bool analyze(const float* normalizedSamples, size_t sampleCount,
               uint32_t sampleRateHz, AudioFeatures& output);

 private:
  bool fft(size_t sampleCount);

  float _real[MAX_SAMPLES] = {0.0F};
  float _imaginary[MAX_SAMPLES] = {0.0F};
};
