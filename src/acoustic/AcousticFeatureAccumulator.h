#pragma once

#include <cstdint>
#include "acoustic/AcousticTypes.h"

class AcousticFeatureAccumulator {
 public:
  explicit AcousticFeatureAccumulator(uint16_t targetFrames = 16U);

  bool add(const AudioFeatures& features, AudioFeatures& aggregate);
  void reset();

 private:
  uint16_t _targetFrames;
  uint16_t _frames = 0;
  uint16_t _validFrames = 0;
  uint16_t _responsiveFrames = 0;
  uint32_t _sampleRateHz = 0;
  uint32_t _durationMs = 0;
  double _linearPowerSum = 0.0;
  float _maximumPeakDbfs = -160.0F;
  double _clippingRatioSum = 0.0;
  double _crestFactorSum = 0.0;
  double _zeroCrossingSum = 0.0;
  double _centroidSum = 0.0;
  double _flatnessSum = 0.0;
  double _rolloffSum = 0.0;
  AcousticBandEnergy _bandSums{0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
};
