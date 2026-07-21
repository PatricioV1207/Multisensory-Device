#include "acoustic/AcousticFeatureAccumulator.h"

#include <algorithm>
#include <cmath>

AcousticFeatureAccumulator::AcousticFeatureAccumulator(uint16_t targetFrames)
    : _targetFrames(targetFrames == 0U ? 1U : targetFrames) {}

bool AcousticFeatureAccumulator::add(const AudioFeatures& features,
                                     AudioFeatures& aggregate) {
  ++_frames;
  if (features.valid) {
    ++_validFrames;
    if (features.signalResponsive) {
      ++_responsiveFrames;
    }
    _sampleRateHz = features.sampleRateHz;
    _durationMs += features.windowDurationMs;
    _linearPowerSum +=
        std::pow(10.0, static_cast<double>(features.relativeLevelDbfs) / 10.0);
    _maximumPeakDbfs = std::max(_maximumPeakDbfs, features.peakDbfs);
    _clippingRatioSum += features.clippingRatio;
    _crestFactorSum += features.crestFactor;
    _zeroCrossingSum += features.zeroCrossingRate;
    _centroidSum += features.spectralCentroidHz;
    _flatnessSum += features.spectralFlatness;
    _rolloffSum += features.spectralRolloffHz;
    _bandSums.hz80To250 += features.bands.hz80To250;
    _bandSums.hz250To800 += features.bands.hz250To800;
    _bandSums.hz800To2000 += features.bands.hz800To2000;
    _bandSums.hz2000To4000 += features.bands.hz2000To4000;
    _bandSums.hz4000To8000 += features.bands.hz4000To8000;
  }
  if (_frames < _targetFrames) {
    return false;
  }

  aggregate = AudioFeatures{};
  aggregate.valid = _validFrames >= (_targetFrames * 3U) / 4U &&
                    _validFrames > 0U;
  aggregate.signalResponsive =
      _responsiveFrames >= (_targetFrames + 1U) / 2U;
  aggregate.sampleRateHz = _sampleRateHz;
  aggregate.windowDurationMs = _durationMs;
  if (_validFrames > 0U) {
    const float inverse = 1.0F / static_cast<float>(_validFrames);
    aggregate.relativeLevelDbfs = static_cast<float>(
        10.0 * std::log10(std::max(1.0e-16,
                                  _linearPowerSum / _validFrames)));
    aggregate.peakDbfs = _maximumPeakDbfs;
    aggregate.clippingRatio = static_cast<float>(_clippingRatioSum * inverse);
    aggregate.crestFactor = static_cast<float>(_crestFactorSum * inverse);
    aggregate.zeroCrossingRate =
        static_cast<float>(_zeroCrossingSum * inverse);
    aggregate.spectralCentroidHz =
        static_cast<float>(_centroidSum * inverse);
    aggregate.spectralFlatness = static_cast<float>(_flatnessSum * inverse);
    aggregate.spectralRolloffHz = static_cast<float>(_rolloffSum * inverse);
    aggregate.bands = {_bandSums.hz80To250 * inverse,
                       _bandSums.hz250To800 * inverse,
                       _bandSums.hz800To2000 * inverse,
                       _bandSums.hz2000To4000 * inverse,
                       _bandSums.hz4000To8000 * inverse};
  }
  reset();
  return true;
}

void AcousticFeatureAccumulator::reset() {
  _frames = 0U;
  _validFrames = 0U;
  _responsiveFrames = 0U;
  _sampleRateHz = 0U;
  _durationMs = 0U;
  _linearPowerSum = 0.0;
  _maximumPeakDbfs = -160.0F;
  _clippingRatioSum = 0.0;
  _crestFactorSum = 0.0;
  _zeroCrossingSum = 0.0;
  _centroidSum = 0.0;
  _flatnessSum = 0.0;
  _rolloffSum = 0.0;
  _bandSums = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
}
