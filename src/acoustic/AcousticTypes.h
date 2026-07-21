#pragma once

#include <cmath>
#include <cstdint>

struct AcousticBandEnergy {
  AcousticBandEnergy() = default;
  AcousticBandEnergy(float band1, float band2, float band3, float band4,
                     float band5)
      : hz80To250(band1),
        hz250To800(band2),
        hz800To2000(band3),
        hz2000To4000(band4),
        hz4000To8000(band5) {}

  float hz80To250 = NAN;
  float hz250To800 = NAN;
  float hz800To2000 = NAN;
  float hz2000To4000 = NAN;
  float hz4000To8000 = NAN;
};

struct AudioFeatures {
  bool valid = false;
  bool signalResponsive = false;
  uint32_t sampleRateHz = 0;
  uint32_t windowDurationMs = 0;
  float relativeLevelDbfs = NAN;
  float peakDbfs = NAN;
  float clippingRatio = NAN;
  float crestFactor = NAN;
  float zeroCrossingRate = NAN;
  float spectralCentroidHz = NAN;
  float spectralFlatness = NAN;
  float spectralRolloffHz = NAN;
  AcousticBandEnergy bands;
};

struct AcousticData {
  float relativeLevelDbfs = NAN;
  float peakDbfs = NAN;
  float confidence = NAN;
  bool microphoneValid = false;
  bool analysisValid = false;
  bool clipping = false;
  const char* category = "unknown";
  const char* classifierVersion = "";
  uint32_t updatedAtMs = 0;
  uint32_t sampleRateHz = 0;
  uint32_t windowDurationMs = 0;
  float clippingRatio = NAN;
  float crestFactor = NAN;
  float zeroCrossingRate = NAN;
  float spectralCentroidHz = NAN;
  float spectralFlatness = NAN;
  float spectralRolloffHz = NAN;
  AcousticBandEnergy bands;
};

struct AcousticClassification {
  AcousticClassification() = default;
  AcousticClassification(const char* categoryValue, float confidenceValue,
                          const char* versionValue)
      : category(categoryValue),
        confidence(confidenceValue),
        classifierVersion(versionValue) {}

  const char* category = "unknown";
  float confidence = 0.0F;
  const char* classifierVersion = "heuristic-1";
};

struct AcousticAlertStatus {
  bool active = false;
  bool triggered = false;
  const char* eventType = "";
  const char* severity = "info";
  float confidence = 0.0F;
  uint32_t durationMs = 0;
  uint32_t updatedAtMs = 0;
};
