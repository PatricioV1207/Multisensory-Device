#include <ArduinoJson.h>
#include <unity.h>
#include <cstring>
#include "web/LocalWebJsonBuilder.h"

namespace {
LocalWebData sample() {
  LocalWebData data;
  data.deviceId = "bus_iot_prototype_01";
  data.firmwareVersion = "0.2.0";
  data.uptimeMs = 123456;
  data.dht = {25.4F, 60.0F, true, 1000};
  data.light = {320.0F, true, 1000};
  data.accel = {0.0F, 0.0F, 9.81F, true, 1000};
  data.gyro = {0.1F, 0.2F, 0.3F, true, 1000};
  data.mag = {10.0F, 20.0F, 30.0F, true, 1000};
  data.barometer = {1006.6F, 25.0F, 10.0F, true, 1000};
  data.imuValid = true;
  data.storage.mounted = true;
  data.storage.lastWriteOk = true;
  data.storage.freeBytes = 1024;
  data.cellular.modemPresent = true;
  data.cellular.simReady = true;
  data.cellular.networkRegistered = true;
  data.cellular.gprsConnected = true;
  data.cellular.signalQuality = 20;
  data.cellular.signalDbm = -73;
  data.mqtt.configured = true;
  data.mqtt.connected = true;
  data.mqtt.lastPublishSuccessMs = 120000;
  data.ota.enabled = true;
  return data;
}

void test_basic_endpoint_contains_only_allowed_telemetry() {
  char output[1536];
  TEST_ASSERT_TRUE(LocalWebJsonBuilder::buildBasicTelemetry(
      sample(), output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 25.4F,
                           document["temperature_c"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 320.0F,
                           document["light_lux"].as<float>());
  TEST_ASSERT_FALSE(document["latitude"].is<double>());
  TEST_ASSERT_FALSE(document["longitude"].is<double>());
  TEST_ASSERT_FALSE(document["satellites"].is<int>());
  TEST_ASSERT_FALSE(document["speed_kmh"].is<float>());
  TEST_ASSERT_FALSE(document["altitude_m"].is<float>());
}

void test_status_endpoint_contains_operational_state_without_gps() {
  char output[1536];
  TEST_ASSERT_TRUE(
      LocalWebJsonBuilder::buildStatus(sample(), output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_TRUE(document["sd"]["mounted"].as<bool>());
  TEST_ASSERT_TRUE(document["sim800l"]["gprs_connected"].as<bool>());
  TEST_ASSERT_TRUE(document["mqtt"]["connected"].as<bool>());
  TEST_ASSERT_FALSE(document["gps"].is<JsonObject>());
  TEST_ASSERT_NULL(strstr(output, "latitude"));
  TEST_ASSERT_NULL(strstr(output, "longitude"));
}

void test_invalid_values_are_omitted_and_small_buffer_rejected() {
  LocalWebData data = sample();
  data.dht.valid = false;
  data.light.valid = false;
  char output[1536];
  TEST_ASSERT_TRUE(LocalWebJsonBuilder::buildBasicTelemetry(
      data, output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FALSE(document["temperature_c"].is<float>());
  TEST_ASSERT_FALSE(document["light_lux"].is<float>());
  char tiny[8];
  TEST_ASSERT_FALSE(
      LocalWebJsonBuilder::buildStatus(data, tiny, sizeof(tiny)));
}
}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_basic_endpoint_contains_only_allowed_telemetry);
  RUN_TEST(test_status_endpoint_contains_operational_state_without_gps);
  RUN_TEST(test_invalid_values_are_omitted_and_small_buffer_rejected);
  return UNITY_END();
}
