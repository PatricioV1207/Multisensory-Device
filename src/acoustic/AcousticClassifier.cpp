#include "acoustic/AcousticClassifier.h"

#include <algorithm>
#include <cmath>

namespace {
float clampConfidence(float value) {
  return std::max(0.0F, std::min(0.85F, value));
}

AcousticClassification result(const char* category, float confidence) {
  return {category, clampConfidence(confidence), AcousticClassifier::VERSION};
}
}  // namespace

AcousticClassification AcousticClassifier::classify(
    const AudioFeatures& features) {
  if (!features.valid || !features.signalResponsive ||
      !std::isfinite(features.relativeLevelDbfs) ||
      !std::isfinite(features.spectralFlatness) ||
      features.clippingRatio >= 0.01F) {
    return result("unknown", 0.0F);
  }
  if (features.relativeLevelDbfs <= -55.0F) {
    return result("quiet", 0.80F);
  }

  const float veryLow = features.bands.hz80To250;
  const float low = veryLow + features.bands.hz250To800;
  const float middle =
      features.bands.hz250To800 + features.bands.hz800To2000;
  const float presence =
      features.bands.hz800To2000 + features.bands.hz2000To4000;
  const float high =
      features.bands.hz2000To4000 + features.bands.hz4000To8000;

  if (low > 0.78F && features.spectralCentroidHz < 650.0F) {
    if (features.spectralFlatness > 0.35F ||
        features.zeroCrossingRate > 0.12F) {
      return result("wind", 0.50F + (low - 0.78F));
    }
    if (features.crestFactor < 4.5F && features.zeroCrossingRate < 0.12F) {
      return result("engine", 0.55F + (low - 0.78F));
    }
  }

  if (features.relativeLevelDbfs > -34.0F && presence > 0.72F &&
      features.spectralFlatness < 0.12F && features.crestFactor < 4.0F) {
    return result("horn", 0.66F + (presence - 0.72F) * 0.4F);
  }

  if (features.relativeLevelDbfs > -36.0F && middle > 0.70F &&
      features.spectralFlatness >= 0.12F &&
      features.spectralFlatness < 0.30F &&
      features.zeroCrossingRate > 0.08F) {
    // A single aggregate cannot prove temporal modulation, so siren confidence
    // deliberately remains below the default alert threshold.
    return result("siren", 0.55F);
  }

  if (features.relativeLevelDbfs > -40.0F &&
      features.spectralFlatness >= 0.30F && low >= 0.20F && low <= 0.70F &&
      high >= 0.12F && features.spectralCentroidHz >= 550.0F &&
      features.spectralCentroidHz <= 3500.0F) {
    const float confidence = 0.58F +
                             std::min(0.10F,
                                      (features.spectralFlatness - 0.30F) *
                                          0.5F) +
                             std::min(0.06F, high * 0.15F);
    return result("traffic", confidence);
  }

  if (middle > 0.58F && features.spectralFlatness >= 0.10F &&
      features.spectralFlatness <= 0.50F &&
      features.zeroCrossingRate >= 0.03F &&
      features.zeroCrossingRate <= 0.28F) {
    return result("speech", 0.50F + std::min(0.12F, middle - 0.58F));
  }

  if (features.spectralFlatness >= 0.08F &&
      features.spectralFlatness <= 0.38F && presence > 0.30F &&
      high > 0.08F && features.crestFactor < 6.0F) {
    return result("music", 0.45F + std::min(0.12F, presence * 0.15F));
  }

  return result("unknown", 0.20F);
}
