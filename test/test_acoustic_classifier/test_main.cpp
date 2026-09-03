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

void test_classifier_routes_valid_audio_without_promoting_alerts() {
  fillSine(1000.0F, 0.00005F);
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
  AcousticClassification traffic = AcousticClassifier::classify(ambiguous);
  TEST_ASSERT_EQUAL_STRING("traffic", traffic.category);
  TEST_ASSERT_LESS_THAN_FLOAT(0.68F, traffic.confidence);

  AudioFeatures speech = featureTemplate();
  speech.relativeLevelDbfs = -72.0F;
  speech.zeroCrossingRate = 0.18F;
  speech.spectralCentroidHz = 1450.0F;
  speech.spectralFlatness = 0.24F;
  speech.bands = {0.08F, 0.30F, 0.34F, 0.20F, 0.08F};
  AcousticClassification speechResult = AcousticClassifier::classify(speech);
  TEST_ASSERT_EQUAL_STRING("speech", speechResult.category);
  TEST_ASSERT_LESS_THAN_FLOAT(0.68F, speechResult.confidence);

  AudioFeatures invalid = featureTemplate();
  invalid.signalResponsive = false;
  TEST_ASSERT_EQUAL_STRING(
      "unknown", AcousticClassifier::classify(invalid).category);
}

void test_physical_cabin_vectors_keep_horn_confidence_below_alert_threshold() {
  AudioFeatures silence = featureTemplate();
  silence.relativeLevelDbfs = -89.57F;
  silence.peakDbfs = -77.26F;
  silence.crestFactor = 3.600F;
  silence.zeroCrossingRate = 0.3996F;
  silence.spectralCentroidHz = 3713.0F;
  silence.spectralFlatness = 0.5105F;
  silence.spectralRolloffHz = 6638.7F;
  silence.bands = {0.0619F, 0.0910F, 0.1540F, 0.2271F, 0.4660F};
  TEST_ASSERT_EQUAL_STRING("quiet",
                           AcousticClassifier::classify(silence).category);

  AudioFeatures externalHorn = featureTemplate();
  externalHorn.relativeLevelDbfs = -85.65F;
  externalHorn.peakDbfs = -73.97F;
  externalHorn.crestFactor = 3.283F;
  externalHorn.zeroCrossingRate = 0.3708F;
  externalHorn.spectralCentroidHz = 3122.7F;
  externalHorn.spectralFlatness = 0.2161F;
  externalHorn.spectralRolloffHz = 4546.9F;
  externalHorn.bands = {0.0235F, 0.1841F, 0.0478F, 0.5545F, 0.1900F};
  const AcousticClassification externalResult =
      AcousticClassifier::classify(externalHorn);
  TEST_ASSERT_EQUAL_STRING("horn", externalResult.category);
  TEST_ASSERT_LESS_THAN_FLOAT(0.68F, externalResult.confidence);

  AudioFeatures ownHorn = featureTemplate();
  ownHorn.relativeLevelDbfs = -69.64F;
  ownHorn.peakDbfs = -49.19F;
  ownHorn.crestFactor = 3.895F;
  ownHorn.zeroCrossingRate = 0.2187F;
  ownHorn.spectralCentroidHz = 1558.0F;
  ownHorn.spectralFlatness = 0.2244F;
  ownHorn.spectralRolloffHz = 3658.2F;
  ownHorn.bands = {0.2077F, 0.4276F, 0.1000F, 0.1151F, 0.1495F};
  const AcousticClassification ownResult =
      AcousticClassifier::classify(ownHorn);
  TEST_ASSERT_EQUAL_STRING("horn", ownResult.category);
  TEST_ASSERT_LESS_THAN_FLOAT(0.68F, ownResult.confidence);

  AudioFeatures clap = featureTemplate();
  clap.relativeLevelDbfs = -84.20F;
  clap.peakDbfs = -50.96F;
  clap.crestFactor = 4.227F;
  clap.zeroCrossingRate = 0.3970F;
  clap.spectralCentroidHz = 3591.9F;
  clap.spectralFlatness = 0.4874F;
  clap.spectralRolloffHz = 6478.5F;
  clap.bands = {0.0639F, 0.1010F, 0.1627F, 0.2312F, 0.4411F};
  const AcousticClassification clapResult =
      AcousticClassifier::classify(clap);
  TEST_ASSERT_EQUAL_STRING("noise", clapResult.category);
  TEST_ASSERT_LESS_THAN_FLOAT(0.68F, clapResult.confidence);
}

void test_room_printer_background_is_neutral_noise() {
  AudioFeatures printer = featureTemplate();
  printer.relativeLevelDbfs = -84.17F;
  printer.peakDbfs = -67.37F;
  printer.crestFactor = 3.297F;
  printer.zeroCrossingRate = 0.2886F;
  printer.spectralCentroidHz = 3445.6F;
  printer.spectralFlatness = 0.4839F;
  printer.spectralRolloffHz = 6477.5F;
  printer.bands = {0.1244F, 0.1067F, 0.1318F, 0.2004F, 0.4368F};
  TEST_ASSERT_EQUAL_STRING(
      "noise", AcousticClassifier::classify(printer).category);

  AudioFeatures printerWithRoomSpeech = featureTemplate();
  printerWithRoomSpeech.relativeLevelDbfs = -65.79F;
  printerWithRoomSpeech.peakDbfs = -42.66F;
  printerWithRoomSpeech.crestFactor = 2.997F;
  printerWithRoomSpeech.zeroCrossingRate = 0.1667F;
  printerWithRoomSpeech.spectralCentroidHz = 2629.1F;
  printerWithRoomSpeech.spectralFlatness = 0.3709F;
  printerWithRoomSpeech.spectralRolloffHz = 5145.5F;
  printerWithRoomSpeech.bands = {0.3202F, 0.0900F, 0.1099F, 0.1495F,
                                 0.3304F};
  TEST_ASSERT_EQUAL_STRING(
      "noise", AcousticClassifier::classify(printerWithRoomSpeech).category);
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
  RUN_TEST(test_classifier_routes_valid_audio_without_promoting_alerts);
  RUN_TEST(
      test_physical_cabin_vectors_keep_horn_confidence_below_alert_threshold);
  RUN_TEST(test_room_printer_background_is_neutral_noise);
  RUN_TEST(test_accumulator_averages_linear_energy);
  RUN_TEST(test_alert_requires_confidence_duration_and_no_clipping);
  RUN_TEST(test_acoustic_and_event_messages_match_public_contract);
  RUN_TEST(test_acoustic_builder_omits_out_of_contract_features);
  return UNITY_END();
}
