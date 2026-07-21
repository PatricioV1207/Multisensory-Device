#include "acoustic/AcousticMessageBuilder.h"

#include <ArduinoJson.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
bool inRange(float value, float minimum, float maximum) {
  return std::isfinite(value) && value >= minimum && value <= maximum;
}

bool validIdentifier(const char* value, size_t maximumLength) {
  if (value == nullptr || value[0] == '\0') {
    return false;
  }
  const size_t length = std::strlen(value);
  if (length > maximumLength) {
    return false;
  }
  for (size_t index = 0U; index < length; ++index) {
    const char character = value[index];
    const bool allowed =
        (character >= 'A' && character <= 'Z') ||
        (character >= 'a' && character <= 'z') ||
        (character >= '0' && character <= '9') || character == '.' ||
        character == '_' || character == '-';
    if (!allowed) {
      return false;
    }
  }
  return true;
}

bool validCategory(const char* category) {
  constexpr const char* categories[] = {
      "traffic", "music", "speech", "engine", "horn",
      "siren",   "wind",  "quiet",  "unknown"};
  if (category == nullptr) {
    return false;
  }
  for (const char* candidate : categories) {
    if (std::strcmp(category, candidate) == 0) {
      return true;
    }
  }
  return false;
}

bool validBaseContext(const AcousticMessageContext& context) {
  return validIdentifier(context.deviceId, 64U) &&
         validIdentifier(context.vehicleId, 64U) &&
         context.bootId != 0U && context.sequence != 0U &&
         (!context.timeValid || (context.measuredAt != nullptr &&
                                 context.measuredAt[0] != '\0'));
}

bool validAggregateContext(const AcousticMessageContext& context) {
  if (!validBaseContext(context) || context.sampleId == nullptr) {
    return false;
  }
  char expected[171];
  const int count = std::snprintf(
      expected, sizeof(expected), "%s:%lu:%lu:acoustic", context.deviceId,
      static_cast<unsigned long>(context.bootId),
      static_cast<unsigned long>(context.sequence));
  return count > 0 && static_cast<size_t>(count) < sizeof(expected) &&
         std::strcmp(context.sampleId, expected) == 0;
}

bool validEventId(const char* eventId) {
  if (eventId == nullptr) {
    return false;
  }
  const size_t length = std::strlen(eventId);
  if (length < 5U || length > 170U) {
    return false;
  }
  for (size_t index = 0U; index < length; ++index) {
    const char character = eventId[index];
    const bool allowed =
        (character >= 'A' && character <= 'Z') ||
        (character >= 'a' && character <= 'z') ||
        (character >= '0' && character <= '9') || character == '.' ||
        character == '_' || character == '-' || character == ':';
    if (!allowed) {
      return false;
    }
  }
  return true;
}

bool validAnalysis(const AcousticData& data) {
  return data.microphoneValid && data.analysisValid &&
         data.sampleRateHz >= 8000U && data.sampleRateHz <= 48000U &&
         data.windowDurationMs >= 100U && data.windowDurationMs <= 10000U &&
         inRange(data.relativeLevelDbfs, -160.0F, 0.0F) &&
         inRange(data.peakDbfs, -160.0F, 0.0F) &&
         inRange(data.clippingRatio, 0.0F, 1.0F) &&
         inRange(data.crestFactor, 0.0F, 100.0F) &&
         inRange(data.zeroCrossingRate, 0.0F, 1.0F) &&
         inRange(data.spectralCentroidHz, 0.0F, 24000.0F) &&
         inRange(data.spectralFlatness, 0.0F, 1.0F) &&
         inRange(data.spectralRolloffHz, 0.0F, 24000.0F) &&
         inRange(data.bands.hz80To250, 0.0F, 1.0F) &&
         inRange(data.bands.hz250To800, 0.0F, 1.0F) &&
         inRange(data.bands.hz800To2000, 0.0F, 1.0F) &&
         inRange(data.bands.hz2000To4000, 0.0F, 1.0F) &&
         inRange(data.bands.hz4000To8000, 0.0F, 1.0F) &&
         inRange(data.confidence, 0.0F, 1.0F) &&
         validCategory(data.category) && data.classifierVersion != nullptr &&
         data.classifierVersion[0] != '\0' &&
         std::strlen(data.classifierVersion) <= 40U;
}

bool serialize(JsonDocument& document, char* output, size_t outputSize,
               size_t* written) {
  if (written != nullptr) {
    *written = 0U;
  }
  if (output == nullptr || outputSize == 0U ||
      measureJson(document) + 1U > outputSize) {
    if (output != nullptr && outputSize > 0U) {
      output[0] = '\0';
    }
    return false;
  }
  const size_t count = serializeJson(document, output, outputSize);
  if (count == 0U || count + 1U > outputSize) {
    output[0] = '\0';
    return false;
  }
  if (written != nullptr) {
    *written = count;
  }
  return true;
}

void addContext(JsonDocument& document, const AcousticMessageContext& context) {
  document["device_id"] = context.deviceId;
  document["vehicle_id"] = context.vehicleId;
  document["boot_id"] = context.bootId;
  document["sequence"] = context.sequence;
  document["sample_id"] = context.sampleId;
  document["uptime_ms"] = context.uptimeMs;
  document["time_valid"] = context.timeValid;
  if (context.timeValid) {
    document["measured_at"] = context.measuredAt;
  }
  document["simulated"] = context.simulated;
}
}  // namespace

bool AcousticMessageBuilder::buildAggregate(
    const AcousticMessageContext& context, const AcousticData& data,
    char* output, size_t outputSize, size_t* written) {
  if (!validAggregateContext(context)) {
    return false;
  }
  const bool analysisValid = validAnalysis(data);
  JsonDocument document;
  document["schema_version"] = 1;
  document["message_type"] = "acoustic";
  addContext(document, context);
  document["replayed"] = false;
  document["mic_valid"] = data.microphoneValid;
  document["analysis_valid"] = analysisValid;
  if (analysisValid) {
    document["window_duration_ms"] = data.windowDurationMs;
    document["sample_rate_hz"] = data.sampleRateHz;
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
    document["category"] = data.category;
    document["confidence"] = data.confidence;
    document["classifier_version"] = data.classifierVersion;
  }
  return serialize(document, output, outputSize, written);
}

bool AcousticMessageBuilder::buildEvent(
    const AcousticMessageContext& context, const AcousticData& data,
    const AcousticAlertStatus& alert, const char* eventId, char* output,
    size_t outputSize, size_t* written) {
  if (!validBaseContext(context) || !alert.triggered ||
      !validEventId(eventId) ||
      alert.eventType == nullptr ||
      (std::strcmp(alert.eventType, "acoustic_traffic") != 0 &&
       std::strcmp(alert.eventType, "acoustic_horn") != 0 &&
       std::strcmp(alert.eventType, "acoustic_siren") != 0) ||
      alert.severity == nullptr ||
      (std::strcmp(alert.severity, "medium") != 0 &&
       std::strcmp(alert.severity, "high") != 0) ||
      !inRange(alert.confidence, 0.0F, 1.0F) || !validAnalysis(data)) {
    return false;
  }
  JsonDocument document;
  document["schema_version"] = 1;
  document["message_type"] = "event";
  document["event_id"] = eventId;
  document["device_id"] = context.deviceId;
  document["vehicle_id"] = context.vehicleId;
  document["event_type"] = alert.eventType;
  document["severity"] = alert.severity;
  document["uptime_ms"] = context.uptimeMs;
  document["time_valid"] = context.timeValid;
  if (context.timeValid) {
    document["occurred_at"] = context.measuredAt;
  }
  document["duration_ms"] = alert.durationMs;
  document["confidence"] = alert.confidence;
  JsonObject details = document["details"].to<JsonObject>();
  details["relative_level_dbfs"] = data.relativeLevelDbfs;
  details["clipping"] = data.clipping;
  details["classifier_version"] = data.classifierVersion;
  document["simulated"] = context.simulated;
  return serialize(document, output, outputSize, written);
}
