#include "acoustic/AcousticClassifier.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kQuietMaximumDbfs = -88.0F;

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
  if (features.relativeLevelDbfs <= kQuietMaximumDbfs) {
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

  if (features.relativeLevelDbfs > -84.0F && presence > 0.72F &&
      features.spectralFlatness < 0.12F && features.crestFactor < 4.0F) {
    return result("horn", 0.66F + (presence - 0.72F) * 0.4F);
  }

  // Provisional cabin signatures derived from the first physical INMP441
  // session. Confidence deliberately remains below the 0.68 alert threshold
  // until a larger labelled dataset measures false positives and negatives.
  const bool externalHornCandidate =
      features.relativeLevelDbfs > -88.0F && presence > 0.50F &&
      features.bands.hz2000To4000 > 0.40F &&
      features.spectralFlatness >= 0.15F &&
      features.spectralFlatness <= 0.32F &&
      features.zeroCrossingRate >= 0.30F &&
      features.zeroCrossingRate <= 0.45F &&
      features.spectralCentroidHz >= 2500.0F &&
      features.spectralCentroidHz <= 3800.0F;
  if (externalHornCandidate) {
    return result("horn", 0.62F);
  }

  const bool ownHornCandidate =
      features.relativeLevelDbfs > -78.0F && low > 0.55F &&
      features.bands.hz250To800 > 0.35F &&
      features.spectralFlatness >= 0.15F &&
      features.spectralFlatness <= 0.32F &&
      features.zeroCrossingRate >= 0.15F &&
      features.zeroCrossingRate <= 0.30F &&
      features.spectralCentroidHz >= 1000.0F &&
      features.spectralCentroidHz <= 2500.0F;
  if (ownHornCandidate) {
    return result("horn", 0.64F);
  }

  if (features.relativeLevelDbfs > -36.0F && middle > 0.70F &&
      features.spectralFlatness >= 0.12F &&
      features.spectralFlatness < 0.30F &&
      features.zeroCrossingRate > 0.08F) {
    // A single aggregate cannot prove temporal modulation, so siren confidence
    // deliberately remains below the default alert threshold.
    return result("siren", 0.55F);
  }

  const bool speechCandidate =
      middle >= 0.34F && presence >= 0.40F &&
      features.spectralFlatness >= 0.08F &&
      features.spectralFlatness < 0.36F &&
      features.zeroCrossingRate >= 0.025F &&
      features.zeroCrossingRate < 0.32F &&
      features.spectralCentroidHz >= 300.0F &&
      features.spectralCentroidHz <= 3200.0F;
  if (speechCandidate) {
    const float confidence =
        0.48F + std::min(0.10F, (middle - 0.34F) * 0.30F);
    return result("speech", confidence);
  }

  const bool trafficCandidate =
      features.relativeLevelDbfs > -45.0F &&
      features.spectralCentroidHz >= 550.0F &&
      features.spectralCentroidHz <= 7000.0F && high >= 0.12F &&
      (features.spectralFlatness >= 0.28F ||
       features.zeroCrossingRate >= 0.30F || high >= 0.32F);
  if (trafficCandidate) {
    const float confidence =
        0.46F +
        std::min(0.08F,
                 std::max(0.0F, features.spectralFlatness - 0.28F) * 0.35F) +
        std::min(0.06F, high * 0.12F);
    return result("traffic", confidence);
  }

  if (features.spectralFlatness >= 0.08F &&
      features.spectralFlatness <= 0.38F && presence > 0.30F &&
      high > 0.08F && features.crestFactor < 6.0F) {
    return result("music", 0.45F + std::min(0.12F, presence * 0.15F));
  }

  // Keep valid but unmatched sound neutral. A room with a running 3D printer
  // measured roughly -87 to -66 dBFS, 2.6-3.7 kHz centroid and 0.36-0.50
  // flatness; that broad high-frequency background must not imply traffic.
  if (middle >= 0.28F && features.spectralCentroidHz <= 3600.0F) {
    return result("speech", 0.36F);
  }
  return result("noise", 0.40F);
}
