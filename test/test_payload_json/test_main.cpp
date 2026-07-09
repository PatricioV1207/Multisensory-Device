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
  data.gy801.imuValid = true;
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
  TEST_ASSERT_EQUAL(1, doc["schema_version"].as<int>());
  TEST_ASSERT_EQUAL_STRING("bus_iot_prototype_01", doc["device_id"]);
  TEST_ASSERT_TRUE(doc["imu_valid"].as<bool>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 25.4F, doc["temperature_c"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 9.81F, doc["accel_z"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1012.8F, doc["pressure_hpa"].as<float>());
}

void test_invalid_measurements_are_omitted_but_flags_remain() {
  TelemetryData data = completeSample();
  data.dht.valid = false;
  data.gps.valid = false;
  data.gy801.mag.valid = false;
  data.gy801.imuValid = false;
  char output[1536];
  TEST_ASSERT_TRUE(TelemetryBuilder::build(data, output, sizeof(output)));

  JsonDocument doc;
  TEST_ASSERT_FALSE(deserializeJson(doc, output));
  TEST_ASSERT_FALSE(doc["dht_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["gps_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["mag_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["imu_valid"].as<bool>());
  TEST_ASSERT_FALSE(doc["temperature_c"].is<float>());
  TEST_ASSERT_FALSE(doc["latitude"].is<double>());
  TEST_ASSERT_FALSE(doc["mag_x"].is<float>());
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
}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_complete_payload_contains_expected_fields);
  RUN_TEST(test_invalid_measurements_are_omitted_but_flags_remain);
  RUN_TEST(test_small_buffer_and_missing_device_are_rejected);
  return UNITY_END();
}
