#include <unity.h>

#include <ArduinoJson.h>
#include <cmath>
#include <cstring>
#include "acoustic/AcousticAlertEvaluator.h"
#include "acoustic/AcousticClassifier.h"
#include "acoustic/AcousticMessageBuilder.h"
#include "acoustic/AcousticFeatureAccumulator.h"
#include "acoustic/AudioFeatureExtractor.h"

namespace {
constexpr size_t kSamples = 1024U;
constexpr uint32_t kSampleRate = 16000U;
constexpr float kPi = 3.14159265358979323846F;
float samples[kSamples];
AudioFeatureExtractor extractor;

void fillSine(float frequencyHz, float amplitude) {
  for (size_t index = 0; index < kSamples; ++index) {
    samples[index] = amplitude *
                     std::sin(2.0F * kPi * frequencyHz * index /
                              static_cast<float>(kSampleRate));
  }
}

AudioFeatures featureTemplate() {
  AudioFeatures features;
  features.valid = true;
  features.signalResponsive = true;
  features.sampleRateHz = kSampleRate;
  features.windowDurationMs = 1000U;
  features.relativeLevelDbfs = -30.0F;
  features.peakDbfs = -10.0F;
  features.clippingRatio = 0.0F;
  features.crestFactor = 3.0F;
  features.zeroCrossingRate = 0.1F;
  features.spectralCentroidHz = 1200.0F;
  features.spectralFlatness = 0.4F;
  features.spectralRolloffHz = 3200.0F;
  features.bands = {0.2F, 0.25F, 0.25F, 0.2F, 0.1F};
  return features;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_fft_finds_one_kilohertz_tone() {
  fillSine(1000.0F, 0.5F);
  AudioFeatures features;
  TEST_ASSERT_TRUE(
      extractor.analyze(samples, kSamples, kSampleRate, features));
  TEST_ASSERT_TRUE(features.signalResponsive);
  TEST_ASSERT_FLOAT_WITHIN(80.0F, 1000.0F, features.spectralCentroidHz);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.90F, features.bands.hz800To2000);
  TEST_ASSERT_LESS_THAN_FLOAT(0.10F, features.spectralFlatness);
  TEST_ASSERT_FLOAT_WITHIN(1.5F, -9.0F, features.relativeLevelDbfs);
}

void test_clipping_and_zero_signal_are_explicit() {
  for (size_t index = 0; index < kSamples; ++index) {
    samples[index] = index % 2U == 0U ? 1.0F : -1.0F;
  }
  AudioFeatures clipping;
  TEST_ASSERT_TRUE(
      extractor.analyze(samples, kSamples, kSampleRate, clipping));
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1.0F, clipping.clippingRatio);

  for (float& sample : samples) {
    sample = 0.0F;
  }
  AudioFeatures disconnected;
  TEST_ASSERT_TRUE(
      extractor.analyze(samples, kSamples, kSampleRate, disconnected));
  TEST_ASSERT_FALSE(disconnected.signalResponsive);
}

void test_classifier_is_conservative_for_quiet_tone_and_ambiguity() {
  fillSine(1000.0F, 0.0005F);
  AudioFeatures quiet;
  TEST_ASSERT_TRUE(extractor.analyze(samples, kSamples, kSampleRate, quiet));
  AcousticClassification quietResult = AcousticClassifier::classify(quiet);
  TEST_ASSERT_EQUAL_STRING("quiet", quietResult.category);

  fillSine(1000.0F, 0.5F);
  AudioFeatures tone;
  TEST_ASSERT_TRUE(extractor.analyze(samples, kSamples, kSampleRate, tone));
  AcousticClassification toneResult = AcousticClassifier::classify(tone);
  TEST_ASSERT_EQUAL_STRING("horn", toneResult.category);

  AudioFeatures ambiguous = featureTemplate();
  ambiguous.spectralCentroidHz = 5000.0F;
  ambiguous.spectralFlatness = 0.90F;
  ambiguous.bands = {0.02F, 0.03F, 0.05F, 0.35F, 0.55F};
  AcousticClassification unknown = AcousticClassifier::classify(ambiguous);
  TEST_ASSERT_EQUAL_STRING("unknown", unknown.category);
  TEST_ASSERT_LESS_THAN_FLOAT(0.30F, unknown.confidence);
}

void test_accumulator_averages_linear_energy() {
  AcousticFeatureAccumulator accumulator(2U);
  AudioFeatures first = featureTemplate();
  first.relativeLevelDbfs = -20.0F;
  AudioFeatures second = first;
  second.relativeLevelDbfs = -40.0F;
  AudioFeatures aggregate;
  TEST_ASSERT_FALSE(accumulator.add(first, aggregate));
  TEST_ASSERT_TRUE(accumulator.add(second, aggregate));
  TEST_ASSERT_TRUE(aggregate.valid);
  TEST_ASSERT_FLOAT_WITHIN(0.2F, -22.97F, aggregate.relativeLevelDbfs);
  TEST_ASSERT_EQUAL_UINT32(2000U, aggregate.windowDurationMs);
}

void test_alert_requires_confidence_duration_and_no_clipping() {
  AcousticAlertEvaluator evaluator;
  AcousticData data;
  data.microphoneValid = true;
  data.analysisValid = true;
  data.clipping = false;
  data.relativeLevelDbfs = -30.0F;
  data.confidence = 0.75F;
  data.category = "traffic";

  AcousticAlertStatus status = evaluator.update(data, 1000U);
  TEST_ASSERT_TRUE(status.active);
  TEST_ASSERT_FALSE(status.triggered);
  status = evaluator.update(data, 8000U);
  TEST_ASSERT_FALSE(status.triggered);
  status = evaluator.update(data, 9000U);
  TEST_ASSERT_TRUE(status.triggered);

  data.clipping = true;
  status = evaluator.update(data, 10000U);
  TEST_ASSERT_FALSE(status.active);
  TEST_ASSERT_FALSE(status.triggered);
}

void test_acoustic_and_event_messages_match_public_contract() {
  AcousticData data;
  data.microphoneValid = true;
  data.analysisValid = true;
  data.sampleRateHz = 16000U;
  data.windowDurationMs = 1024U;
  data.relativeLevelDbfs = -31.8F;
  data.peakDbfs = -8.2F;
  data.clipping = false;
  data.clippingRatio = 0.0F;
  data.crestFactor = 4.2F;
  data.zeroCrossingRate = 0.13F;
  data.spectralCentroidHz = 1120.0F;
  data.spectralFlatness = 0.42F;
  data.spectralRolloffHz = 3250.0F;
  data.bands = {0.25F, 0.32F, 0.21F, 0.14F, 0.08F};
  data.category = "traffic";
  data.confidence = 0.78F;
  data.classifierVersion = "heuristic-1";

  AcousticMessageContext context;
  context.deviceId = "device-01";
  context.vehicleId = "vehicle-01";
  context.bootId = 7U;
  context.sequence = 42U;
  context.sampleId = "device-01:7:42:acoustic";
  context.uptimeMs = 420000U;
  context.timeValid = true;
  context.measuredAt = "2026-07-20T23:18:51Z";
  char output[2048];
  TEST_ASSERT_TRUE(AcousticMessageBuilder::buildAggregate(
      context, data, output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_EQUAL_STRING("acoustic", document["message_type"]);
  TEST_ASSERT_EQUAL_STRING("traffic", document["category"]);
  TEST_ASSERT_FLOAT_WITHIN(
      0.001F, 0.25F,
      document["band_energy_ratio"]["hz_80_250"].as<float>());
  TEST_ASSERT_FALSE(document["noise_db_spl"].is<float>());

  AcousticAlertStatus alert;
  alert.triggered = true;
  alert.active = true;
  alert.eventType = "acoustic_traffic";
  alert.severity = "medium";
  alert.durationMs = 8000U;
  alert.confidence = 0.78F;
  context.sequence = 43U;
  context.sampleId = "device-01:7:43";
  TEST_ASSERT_TRUE(AcousticMessageBuilder::buildEvent(
      context, data, alert, "device-01:7:43:acoustic_traffic", output,
      sizeof(output)));
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_EQUAL_STRING("event", document["message_type"]);
  TEST_ASSERT_EQUAL_STRING("acoustic_traffic", document["event_type"]);
}

void test_acoustic_builder_omits_out_of_contract_features() {
  AcousticData data;
  data.microphoneValid = true;
  data.analysisValid = true;
  data.sampleRateHz = 16000U;
  data.windowDurationMs = 1000U;
  data.relativeLevelDbfs = -30.0F;
  data.peakDbfs = -3.0F;
  data.clippingRatio = 0.0F;
  data.crestFactor = 3.0F;
  data.zeroCrossingRate = 0.1F;
  data.spectralCentroidHz = 1000.0F;
  data.spectralFlatness = 0.4F;
  data.spectralRolloffHz = 3000.0F;
  data.bands = {0.2F, 0.2F, 0.2F, 0.2F, 0.2F};
  data.category = "traffic";
  data.confidence = 0.75F;
  data.classifierVersion = "heuristic-1";
  AcousticMessageContext context;
  context.deviceId = "device-01";
  context.vehicleId = "vehicle-01";
  context.bootId = 1U;
  context.sequence = 1U;
  context.sampleId = "device-01:1:1:acoustic";
  char output[2048];
  TEST_ASSERT_TRUE(AcousticMessageBuilder::buildAggregate(
      context, data, output, sizeof(output)));

  data.confidence = 1.1F;
  TEST_ASSERT_TRUE(AcousticMessageBuilder::buildAggregate(
      context, data, output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FALSE(document["analysis_valid"].as<bool>());
  TEST_ASSERT_FALSE(document["confidence"].is<float>());
  data.confidence = 0.75F;
  data.category = "invented";
  TEST_ASSERT_TRUE(AcousticMessageBuilder::buildAggregate(
      context, data, output, sizeof(output)));
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FALSE(document["analysis_valid"].as<bool>());
  TEST_ASSERT_FALSE(document["category"].is<const char*>());
  data.category = "traffic";
  context.sampleId = "device-01:1:999:acoustic";
  TEST_ASSERT_FALSE(AcousticMessageBuilder::buildAggregate(
      context, data, output, sizeof(output)));
  context.sampleId = "device-01:1:1:acoustic";
  context.deviceId = "invalid/device";
  TEST_ASSERT_FALSE(AcousticMessageBuilder::buildAggregate(
      context, data, output, sizeof(output)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_fft_finds_one_kilohertz_tone);
  RUN_TEST(test_clipping_and_zero_signal_are_explicit);
  RUN_TEST(test_classifier_is_conservative_for_quiet_tone_and_ambiguity);
  RUN_TEST(test_accumulator_averages_linear_energy);
  RUN_TEST(test_alert_requires_confidence_duration_and_no_clipping);
  RUN_TEST(test_acoustic_and_event_messages_match_public_contract);
  RUN_TEST(test_acoustic_builder_omits_out_of_contract_features);
  return UNITY_END();
}
