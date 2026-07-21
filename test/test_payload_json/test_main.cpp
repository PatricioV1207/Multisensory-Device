#include <ArduinoJson.h>
#include <unity.h>
#include <cstring>
#include "telemetry/TelemetryBuilder.h"

namespace {
TelemetryData completeSample() {
  TelemetryData data;
  data.deviceId = "bus_iot_prototype_01";
  data.uptimeMs = 123456;
  data.dht = {25.4F, 60.0F, true, 1000};
  data.gps = {-2.123456, -79.123456, 10.5F, 15.2F, 6, true, true, 1000, 1000};
  data.gy801.accel = {0.01F, 0.02F, 9.81F, true, 1000};
  data.gy801.gyro = {0.1F, 0.2F, 0.3F, true, 1000};
  data.gy801.mag = {12.5F, 10.8F, 8.4F, true, 1000};
  data.gy801.barometer = {1012.8F, 28.1F, 4.2F, true, 1000};
  data.gy801.barometer.rawPressureHpa = 1012.8F;
  data.gy801.barometer.seaLevelPressureHpa = 1013.3F;
  data.gy801.imuValid = true;
  data.light = {250.0F, true, 1000};
  data.storage.mounted = true;
  data.cellular.modemPresent = true;
  data.cellular.simReady = true;
  data.cellular.networkRegistered = true;
  data.cellular.gprsConnected = true;
  data.cellular.signalQuality = 18;
  return data;
}

TelemetryData vehicleSenseSample() {
  TelemetryData data = completeSample();
  data.vehicleId = "vehicle_01";
  data.bootId = 7;
  data.sequence = 42;
  data.sampleId = "bus_iot_prototype_01:7:42";
  data.timeValid = true;
  data.measuredAt = "2026-07-20T23:18:51Z";
  data.replayed = false;
  data.simulated = false;
  data.gps.hdop = 0.9F;
  data.wifiConnected = true;
  data.wifiRssiDbm = -58;
  data.mqttConnected = true;
  data.offline.replayed = 4;
  return data;
}

void test_complete_payload_contains_expected_fields() {
  const TelemetryData data = completeSample();
  char output[1536];
  size_t written = 0;
  TEST_ASSERT_TRUE(TelemetryBuilder::build(data, output, sizeof(output), &written));
  TEST_ASSERT_GREATER_THAN(0U, written);

  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_EQUAL(2, doc["schema_version"].as<int>());
  TEST_ASSERT_EQUAL_STRING("bus_iot_prototype_01", doc["device_id"]);
  TEST_ASSERT_TRUE(doc["imu_valid"].as<bool>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 25.4F, doc["temperature_c"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 9.81F, doc["accel_z"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1012.8F, doc["pressure_hpa"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1013.3F,
                           doc["sea_level_pressure_hpa"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 250.0F, doc["light_lux"].as<float>());
  TEST_ASSERT_TRUE(doc["bh1750_valid"].as<bool>());
  TEST_ASSERT_TRUE(doc["sd_valid"].as<bool>());
  TEST_ASSERT_TRUE(doc["sim_valid"].as<bool>());
  TEST_ASSERT_TRUE(doc["gprs_connected"].as<bool>());
  TEST_ASSERT_EQUAL(18, doc["gsm_csq"].as<int>());
  TEST_ASSERT_FALSE(doc["pressure_raw_hpa"].is<float>());
}

void test_invalid_measurements_are_omitted_but_flags_remain() {
  TelemetryData data = completeSample();
  data.dht.valid = false;
  data.gps.valid = false;
  data.gy801.mag.valid = false;
  data.light.valid = false;
  data.gy801.imuValid = false;
  char output[1536];
  TEST_ASSERT_TRUE(TelemetryBuilder::build(data, output, sizeof(output)));

  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_FALSE(doc["dht_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["gps_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["mag_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["imu_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["bh1750_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["temperature_c"].is<float>());
  TEST_ASSERT_FALSE(doc["latitude"].is<double>());
  TEST_ASSERT_FALSE(doc["mag_x"].is<float>());
  TEST_ASSERT_FALSE(doc["light_lux"].is<float>());
  TEST_ASSERT_TRUE(doc["accel_x"].is<float>());
}

void test_small_buffer_and_missing_device_are_rejected() {
  TelemetryData data = completeSample();
  char tiny[16];
  TEST_ASSERT_FALSE(TelemetryBuilder::build(data, tiny, sizeof(tiny)));
  TEST_ASSERT_EQUAL_CHAR('\0', tiny[0]);

  char output[1536];
  data.deviceId = "";
  TEST_ASSERT_FALSE(TelemetryBuilder::build(data, output, sizeof(output)));
}

void test_v3_payload_has_identity_time_and_transport_state() {
  const TelemetryData data = vehicleSenseSample();
  char output[2048];
  size_t written = 0;
  TEST_ASSERT_TRUE(
      TelemetryBuilder::buildV3(data, output, sizeof(output), &written));
  TEST_ASSERT_GREATER_THAN(0U, written);

  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_EQUAL(3, doc["schema_version"].as<int>());
  TEST_ASSERT_EQUAL_STRING("telemetry", doc["message_type"]);
  TEST_ASSERT_EQUAL_STRING("vehicle_01", doc["vehicle_id"]);
  TEST_ASSERT_EQUAL(7U, doc["boot_id"].as<unsigned>());
  TEST_ASSERT_EQUAL(42U, doc["sequence"].as<unsigned>());
  TEST_ASSERT_EQUAL_STRING("bus_iot_prototype_01:7:42", doc["sample_id"]);
  TEST_ASSERT_TRUE(doc["time_valid"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("2026-07-20T23:18:51Z", doc["measured_at"]);
  TEST_ASSERT_TRUE(doc["wifi_connected"].as<bool>());
  TEST_ASSERT_EQUAL(-58, doc["wifi_rssi_dbm"].as<int>());
  TEST_ASSERT_TRUE(doc["mqtt_connected"].as<bool>());
  TEST_ASSERT_FALSE(doc["mic_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["acoustic_valid"].as<bool>());
  TEST_ASSERT_EQUAL(4U, doc["offline_replayed"].as<unsigned>());
}

void test_v3_omits_untrusted_time_and_invalid_values() {
  TelemetryData data = vehicleSenseSample();
  data.timeValid = false;
  data.measuredAt = "";
  data.dht.valid = false;
  data.gps.hdop = NAN;
  char output[2048];
  TEST_ASSERT_TRUE(TelemetryBuilder::buildV3(data, output, sizeof(output)));

  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_FALSE(doc["time_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["measured_at"].is<const char*>());
  TEST_ASSERT_FALSE(doc["dht_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["temperature_c"].is<float>());
  TEST_ASSERT_FALSE(doc["gps_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["latitude"].is<double>());
}

void test_v3_rejects_missing_vehicle_or_persistent_identity() {
  TelemetryData data = vehicleSenseSample();
  char output[2048];
  data.vehicleId = "";
  TEST_ASSERT_FALSE(TelemetryBuilder::buildV3(data, output, sizeof(output)));
  data.vehicleId = "vehicle_01";
  data.bootId = 0;
  TEST_ASSERT_FALSE(TelemetryBuilder::buildV3(data, output, sizeof(output)));
}

void test_v3_acoustic_summary_is_bounded_and_omitted_when_invalid() {
  TelemetryData data = vehicleSenseSample();
  data.acoustic.microphoneValid = true;
  data.acoustic.analysisValid = true;
  data.acoustic.relativeLevelDbfs = -31.8F;
  data.acoustic.peakDbfs = -8.2F;
  data.acoustic.category = "traffic";
  data.acoustic.confidence = 0.78F;
  data.acoustic.classifierVersion = "heuristic-1";
  char output[2048];
  TEST_ASSERT_TRUE(TelemetryBuilder::buildV3(data, output, sizeof(output)));
  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_TRUE(doc["mic_valid"].as<bool>());
  TEST_ASSERT_TRUE(doc["acoustic_valid"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("traffic", doc["acoustic_category"]);
  TEST_ASSERT_FLOAT_WITHIN(
      0.01F, -31.8F, doc["acoustic_relative_level_dbfs"].as<float>());

  data.acoustic.category = "invented";
  TEST_ASSERT_TRUE(TelemetryBuilder::buildV3(data, output, sizeof(output)));
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_TRUE(doc["mic_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["acoustic_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["acoustic_category"].is<const char*>());
  TEST_ASSERT_FALSE(doc["acoustic_confidence"].is<float>());
}
}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_complete_payload_contains_expected_fields);
  RUN_TEST(test_invalid_measurements_are_omitted_but_flags_remain);
  RUN_TEST(test_small_buffer_and_missing_device_are_rejected);
  RUN_TEST(test_v3_payload_has_identity_time_and_transport_state);
  RUN_TEST(test_v3_omits_untrusted_time_and_invalid_values);
  RUN_TEST(test_v3_rejects_missing_vehicle_or_persistent_identity);
  RUN_TEST(test_v3_acoustic_summary_is_bounded_and_omitted_when_invalid);
  return UNITY_END();
}
