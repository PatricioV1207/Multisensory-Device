#include <ArduinoJson.h>
#include <unity.h>
#include <cstring>
#include "web/LocalWebJsonBuilder.h"

namespace {
LocalWebData sample() {
  LocalWebData data;
  data.deviceId = "bus_iot_prototype_01";
  data.vehicleId = "vehicle_01";
  data.firmwareVersion = "0.2.0";
  data.timeSource = "ntp";
  data.simulated = true;
  data.uptimeMs = 123456;
  data.dht = {25.4F, 60.0F, true, 1000};
  data.light = {320.0F, true, 1000};
  data.accel = {0.0F, 0.0F, 9.81F, true, 1000};
  data.gyro = {0.1F, 0.2F, 0.3F, true, 1000};
  data.mag = {10.0F, 20.0F, 30.0F, true, 1000};
  data.barometer = {1006.6F, 25.0F, 10.0F, true, 1000};
  data.gps.latitude = -2.189412;
  data.gps.longitude = -79.889066;
  data.gps.altitudeM = 12.4F;
  data.gps.speedKmh = 36.2F;
  data.gps.satellites = 10;
  data.gps.valid = true;
  data.gps.streamSeen = true;
  data.gps.charsProcessed = 5000;
  data.gps.hdop = 0.9F;
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
  data.mqtt.lastPublishAckMs = 120000;
  data.mqtt.lastAcknowledgedToken = 12;
  data.wifi.configured = true;
  data.wifi.connected = true;
  data.wifi.accessPointRunning = true;
  data.wifi.rssiDbm = -58;
  strcpy(data.wifi.stationIp, "192.168.1.42");
  strcpy(data.wifi.accessPointIp, "192.168.4.1");
  data.offline.ready = true;
  data.offline.queued = 3;
  data.offline.replayed = 4;
  data.offline.dropped = 1;
  data.offline.oldestAgeSeconds = 25;
  data.offline.bytes = 4096;
  data.ota.enabled = true;
  return data;
}

void test_basic_endpoint_contains_text_gps_without_map_data() {
  char output[1536];
  TEST_ASSERT_TRUE(LocalWebJsonBuilder::buildBasicTelemetry(
      sample(), output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 25.4F,
                           document["temperature_c"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 320.0F,
                           document["light_lux"].as<float>());
  TEST_ASSERT_TRUE(document["gps_valid"].as<bool>());
  TEST_ASSERT_FLOAT_WITHIN(0.00001F, -2.189412F,
                           document["latitude"].as<float>());
  TEST_ASSERT_FLOAT_WITHIN(0.00001F, -79.889066F,
                           document["longitude"].as<float>());
  TEST_ASSERT_EQUAL(10, document["satellites"].as<int>());
  TEST_ASSERT_FALSE(document["map"].is<JsonObject>());
  TEST_ASSERT_FALSE(document["route"].is<JsonArray>());
}

void test_status_endpoint_contains_vehicle_wifi_and_delivery_state() {
  char output[1536];
  TEST_ASSERT_TRUE(
      LocalWebJsonBuilder::buildStatus(sample(), output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_TRUE(document["sd"]["mounted"].as<bool>());
  TEST_ASSERT_TRUE(document["sim800l"]["gprs_connected"].as<bool>());
  TEST_ASSERT_TRUE(document["mqtt"]["connected"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("vehicle_01", document["vehicle_id"]);
  TEST_ASSERT_TRUE(document["simulated"].as<bool>());
  TEST_ASSERT_TRUE(document["wifi"]["connected"].as<bool>());
  TEST_ASSERT_EQUAL(-58, document["wifi"]["rssi_dbm"].as<int>());
  TEST_ASSERT_EQUAL_STRING("192.168.4.1", document["wifi"]["ap_ip"]);
  TEST_ASSERT_EQUAL(12U,
                    document["mqtt"]["last_acknowledged_token"].as<unsigned>());
  TEST_ASSERT_TRUE(document["offline"]["ready"].as<bool>());
  TEST_ASSERT_EQUAL(3U, document["offline"]["queued"].as<unsigned>());
  TEST_ASSERT_EQUAL(4096U, document["offline"]["bytes"].as<unsigned>());
  TEST_ASSERT_FALSE(document["gps"].is<JsonObject>());
  TEST_ASSERT_NULL(strstr(output, "latitude"));
  TEST_ASSERT_NULL(strstr(output, "longitude"));
}

void test_invalid_values_are_omitted_and_small_buffer_rejected() {
  LocalWebData data = sample();
  data.dht.valid = false;
  data.light.valid = false;
  data.gps.valid = false;
  char output[1536];
  TEST_ASSERT_TRUE(LocalWebJsonBuilder::buildBasicTelemetry(
      data, output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FALSE(document["temperature_c"].is<float>());
  TEST_ASSERT_FALSE(document["light_lux"].is<float>());
  TEST_ASSERT_FALSE(document["latitude"].is<double>());
  TEST_ASSERT_FALSE(document["longitude"].is<double>());
  char tiny[8];
  TEST_ASSERT_FALSE(
      LocalWebJsonBuilder::buildStatus(data, tiny, sizeof(tiny)));
}

void test_acoustic_values_are_relative_and_omitted_when_invalid() {
  LocalWebData data = sample();
  data.acoustic.microphoneValid = true;
  data.acoustic.analysisValid = true;
  data.acoustic.relativeLevelDbfs = -31.8F;
  data.acoustic.peakDbfs = -8.2F;
  data.acoustic.confidence = 0.78F;
  data.acoustic.category = "traffic";
  data.acoustic.clipping = false;
  data.acousticAlert.active = true;
  data.acousticAlert.eventType = "acoustic_traffic";
  data.acousticAlert.severity = "medium";
  data.acousticAlert.confidence = 0.78F;
  data.acousticAlert.durationMs = 9000U;
  char output[1536];
  TEST_ASSERT_TRUE(LocalWebJsonBuilder::buildBasicTelemetry(
      data, output, sizeof(output)));
  JsonDocument document;
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_EQUAL_STRING("traffic", document["acoustic_category"]);
  TEST_ASSERT_FLOAT_WITHIN(
      0.01F, -31.8F, document["acoustic_relative_level_dbfs"].as<float>());
  TEST_ASSERT_FALSE(document["noise_db_spl"].is<float>());
  TEST_ASSERT_TRUE(document["acoustic_alert_active"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("acoustic_traffic",
                           document["acoustic_alert_type"]);
  TEST_ASSERT_EQUAL(9000U,
                    document["acoustic_alert_duration_ms"].as<unsigned>());

  data.acoustic.analysisValid = false;
  TEST_ASSERT_TRUE(LocalWebJsonBuilder::buildBasicTelemetry(
      data, output, sizeof(output)));
  TEST_ASSERT_FALSE(deserializeJson(document, output));
  TEST_ASSERT_FALSE(document["acoustic_relative_level_dbfs"].is<float>());
  TEST_ASSERT_FALSE(document["acoustic_category"].is<const char*>());
}
}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_basic_endpoint_contains_text_gps_without_map_data);
  RUN_TEST(test_status_endpoint_contains_vehicle_wifi_and_delivery_state);
  RUN_TEST(test_invalid_values_are_omitted_and_small_buffer_rejected);
  RUN_TEST(test_acoustic_values_are_relative_and_omitted_when_invalid);
  return UNITY_END();
}
