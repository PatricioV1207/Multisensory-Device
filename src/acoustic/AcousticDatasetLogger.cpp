#include "acoustic/AcousticDatasetLogger.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <cmath>
#include <cstring>
#include "config.h"
#include "utils/Logger.h"

namespace {
constexpr const char* kDatasetDirectory = "/acoustic";
constexpr const char* kDatasetPath = "/acoustic/features.jsonl";
constexpr const char* kLabels[] = {"quiet", "wind", "engine", "speech",
                                   "music", "horn", "siren", "traffic",
                                   "unknown"};
}

bool AcousticDatasetLogger::begin() {
  _ready = SD.cardType() != CARD_NONE &&
           (SD.exists(kDatasetDirectory) || SD.mkdir(kDatasetDirectory));
  if (_ready) {
    Logger::info("AUDIO", "Feature dataset path=" + String(kDatasetPath));
    if (ACOUSTIC_RAW_AUDIO_ENABLED == 0) {
      Logger::info("AUDIO", "Raw audio storage is disabled for privacy");
    }
  }
  return _ready;
}

bool AcousticDatasetLogger::append(const char* label,
                                   const AcousticData& data,
                                   uint32_t nowMs) {
  if (!_ready || !isValidLabel(label) || !data.microphoneValid ||
      !data.analysisValid || !std::isfinite(data.relativeLevelDbfs)) {
    return false;
  }
  JsonDocument document;
  document["schema_version"] = 1;
  document["label"] = label;
  document["captured_at_uptime_ms"] = nowMs;
  document["raw_audio_stored"] = false;
  document["sample_rate_hz"] = data.sampleRateHz;
  document["window_duration_ms"] = data.windowDurationMs;
  document["relative_level_dbfs"] = data.relativeLevelDbfs;
  document["peak_dbfs"] = data.peakDbfs;
  document["clipping"] = data.clipping;
  document["clipping_ratio"] = data.clippingRatio;
  document["crest_factor"] = data.crestFactor;
  document["zero_crossing_rate"] = data.zeroCrossingRate;
  document["spectral_centroid_hz"] = data.spectralCentroidHz;
  document["spectral_flatness"] = data.spectralFlatness;
  document["spectral_rolloff_hz"] = data.spectralRolloffHz;
  JsonObject bands = document["band_energy_ratio"].to<JsonObject>();
  bands["hz_80_250"] = data.bands.hz80To250;
  bands["hz_250_800"] = data.bands.hz250To800;
  bands["hz_800_2000"] = data.bands.hz800To2000;
  bands["hz_2000_4000"] = data.bands.hz2000To4000;
  bands["hz_4000_8000"] = data.bands.hz4000To8000;
  document["predicted_category"] = data.category;
  document["predicted_confidence"] = data.confidence;
  document["classifier_version"] = data.classifierVersion;

  File file = SD.open(kDatasetPath, FILE_APPEND);
  if (!file) {
    _ready = false;
    return false;
  }
  const size_t written = serializeJson(document, file);
  const size_t newline = file.write(static_cast<uint8_t>('\n'));
  file.flush();
  file.close();
  if (written == 0U || newline != 1U) {
    _ready = false;
    return false;
  }
  return true;
}

bool AcousticDatasetLogger::isReady() const {
  return _ready;
}

bool AcousticDatasetLogger::isValidLabel(const char* label) {
  if (label == nullptr) {
    return false;
  }
  for (const char* candidate : kLabels) {
    if (strcmp(label, candidate) == 0) {
      return true;
    }
  }
  return false;
}
