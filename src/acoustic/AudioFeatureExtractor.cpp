#include "acoustic/AudioFeatureExtractor.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846F;
constexpr float kDbFloor = -160.0F;
constexpr float kHighPassCutoffHz = 90.0F;
constexpr float kClippingThreshold = 0.98F;
constexpr float kSignalVariationMinimum = 1.0e-7F;

float toDb(float amplitude) {
  return amplitude > 1.0e-8F
             ? std::max(kDbFloor,
                        std::min(0.0F, 20.0F * std::log10(amplitude)))
             : kDbFloor;
}

float clampSample(float sample) {
  return std::max(-1.0F, std::min(1.0F, sample));
}

bool isPowerOfTwo(size_t value) {
  return value >= 2U && (value & (value - 1U)) == 0U;
}

void addBandPower(float frequencyHz, float power, AcousticBandEnergy& bands) {
  if (frequencyHz >= 80.0F && frequencyHz < 250.0F) {
    bands.hz80To250 += power;
  } else if (frequencyHz < 800.0F) {
    bands.hz250To800 += power;
  } else if (frequencyHz < 2000.0F) {
    bands.hz800To2000 += power;
  } else if (frequencyHz < 4000.0F) {
    bands.hz2000To4000 += power;
  } else if (frequencyHz <= 8000.0F) {
    bands.hz4000To8000 += power;
  }
}
}  // namespace

bool AudioFeatureExtractor::analyze(const float* samples, size_t sampleCount,
                                    uint32_t sampleRateHz,
                                    AudioFeatures& output) {
  output = AudioFeatures{};
  if (samples == nullptr || sampleRateHz < 8000U || sampleRateHz > 48000U ||
      sampleCount < 128U || sampleCount > MAX_SAMPLES ||
      !isPowerOfTwo(sampleCount)) {
    return false;
  }

  double mean = 0.0;
  float minimum = clampSample(samples[0]);
  float maximum = minimum;
  size_t nonZeroSamples = 0U;
  size_t clippingSamples = 0U;
  for (size_t index = 0; index < sampleCount; ++index) {
    const float sample = clampSample(samples[index]);
    mean += sample;
    minimum = std::min(minimum, sample);
    maximum = std::max(maximum, sample);
    if (std::fabs(sample) > 1.0e-9F) {
      ++nonZeroSamples;
    }
    if (std::fabs(sample) >= kClippingThreshold) {
      ++clippingSamples;
    }
  }
  mean /= static_cast<double>(sampleCount);

  const float dt = 1.0F / static_cast<float>(sampleRateHz);
  const float rc = 1.0F / (2.0F * kPi * kHighPassCutoffHz);
  const float alpha = rc / (rc + dt);
  float previousInput = static_cast<float>(clampSample(samples[0]) - mean);
  float previousOutput = 0.0F;
  double sumSquares = 0.0;
  float peak = 0.0F;
  size_t zeroCrossings = 0U;
  float previousFiltered = 0.0F;
  for (size_t index = 0; index < sampleCount; ++index) {
    const float currentInput =
        static_cast<float>(clampSample(samples[index]) - mean);
    const float filtered =
        alpha * (previousOutput + currentInput - previousInput);
    previousInput = currentInput;
    previousOutput = filtered;
    sumSquares += static_cast<double>(filtered) * filtered;
    peak = std::max(peak, std::fabs(filtered));
    if (index > 0U && ((filtered >= 0.0F) != (previousFiltered >= 0.0F))) {
      ++zeroCrossings;
    }
    previousFiltered = filtered;
    const float hann = 0.5F -
                       0.5F * std::cos((2.0F * kPi * index) /
                                       static_cast<float>(sampleCount - 1U));
    _real[index] = filtered * hann;
    _imaginary[index] = 0.0F;
  }

  if (!fft(sampleCount)) {
    return false;
  }

  const float rms =
      std::sqrt(static_cast<float>(sumSquares / sampleCount));
  output.valid = std::isfinite(rms) && std::isfinite(peak);
  output.signalResponsive =
      nonZeroSamples >= sampleCount / 20U &&
      maximum - minimum > kSignalVariationMinimum;
  output.sampleRateHz = sampleRateHz;
  output.windowDurationMs = static_cast<uint32_t>(
      (sampleCount * 1000ULL) / static_cast<uint64_t>(sampleRateHz));
  output.relativeLevelDbfs = toDb(rms);
  output.peakDbfs = toDb(peak);
  output.clippingRatio =
      static_cast<float>(clippingSamples) / static_cast<float>(sampleCount);
  output.crestFactor = rms > 1.0e-8F ? peak / rms : 0.0F;
  output.zeroCrossingRate = static_cast<float>(zeroCrossings) /
                            static_cast<float>(sampleCount - 1U);

  AcousticBandEnergy powers{0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  double totalPower = 0.0;
  double weightedFrequency = 0.0;
  double logarithmicPower = 0.0;
  size_t spectralBins = 0U;
  for (size_t bin = 1U; bin <= sampleCount / 2U; ++bin) {
    const float frequency = static_cast<float>(bin) * sampleRateHz /
                            static_cast<float>(sampleCount);
    if (frequency < 80.0F || frequency > 8000.0F) {
      continue;
    }
    const float power = _real[bin] * _real[bin] +
                        _imaginary[bin] * _imaginary[bin];
    totalPower += power;
    weightedFrequency += static_cast<double>(frequency) * power;
    logarithmicPower += std::log(static_cast<double>(power) + 1.0e-20);
    ++spectralBins;
    addBandPower(frequency, power, powers);
  }

  if (totalPower <= 1.0e-20 || spectralBins == 0U) {
    output.spectralCentroidHz = 0.0F;
    output.spectralFlatness = 0.0F;
    output.spectralRolloffHz = 0.0F;
    output.bands = AcousticBandEnergy{0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    return output.valid;
  }

  output.spectralCentroidHz =
      static_cast<float>(weightedFrequency / totalPower);
  const double geometricMean = std::exp(logarithmicPower / spectralBins);
  const double arithmeticMean = totalPower / spectralBins;
  output.spectralFlatness = static_cast<float>(
      std::min(1.0, geometricMean / (arithmeticMean + 1.0e-20)));

  const double rolloffTarget = totalPower * 0.85;
  double cumulativePower = 0.0;
  output.spectralRolloffHz = 0.0F;
  for (size_t bin = 1U; bin <= sampleCount / 2U; ++bin) {
    const float frequency = static_cast<float>(bin) * sampleRateHz /
                            static_cast<float>(sampleCount);
    if (frequency < 80.0F || frequency > 8000.0F) {
      continue;
    }
    cumulativePower += _real[bin] * _real[bin] +
                       _imaginary[bin] * _imaginary[bin];
    if (cumulativePower >= rolloffTarget) {
      output.spectralRolloffHz = frequency;
      break;
    }
  }

  const float inverseTotal = 1.0F / static_cast<float>(totalPower);
  output.bands = {powers.hz80To250 * inverseTotal,
                  powers.hz250To800 * inverseTotal,
                  powers.hz800To2000 * inverseTotal,
                  powers.hz2000To4000 * inverseTotal,
                  powers.hz4000To8000 * inverseTotal};
  return output.valid;
}

bool AudioFeatureExtractor::fft(size_t sampleCount) {
  if (!isPowerOfTwo(sampleCount)) {
    return false;
  }
  for (size_t index = 1U, reversed = 0U; index < sampleCount; ++index) {
    size_t bit = sampleCount >> 1U;
    for (; reversed & bit; bit >>= 1U) {
      reversed ^= bit;
    }
    reversed ^= bit;
    if (index < reversed) {
      std::swap(_real[index], _real[reversed]);
      std::swap(_imaginary[index], _imaginary[reversed]);
    }
  }
  for (size_t length = 2U; length <= sampleCount; length <<= 1U) {
    const float angle = -2.0F * kPi / static_cast<float>(length);
    const float baseReal = std::cos(angle);
    const float baseImaginary = std::sin(angle);
    for (size_t offset = 0U; offset < sampleCount; offset += length) {
      float twiddleReal = 1.0F;
      float twiddleImaginary = 0.0F;
      for (size_t index = 0U; index < length / 2U; ++index) {
        const size_t even = offset + index;
        const size_t odd = even + length / 2U;
        const float oddReal = _real[odd] * twiddleReal -
                              _imaginary[odd] * twiddleImaginary;
        const float oddImaginary = _real[odd] * twiddleImaginary +
                                   _imaginary[odd] * twiddleReal;
        _real[odd] = _real[even] - oddReal;
        _imaginary[odd] = _imaginary[even] - oddImaginary;
        _real[even] += oddReal;
        _imaginary[even] += oddImaginary;
        const float nextReal = twiddleReal * baseReal -
                               twiddleImaginary * baseImaginary;
        twiddleImaginary = twiddleReal * baseImaginary +
                           twiddleImaginary * baseReal;
        twiddleReal = nextReal;
      }
    }
  }
  return true;
}
