#include <unity.h>
#include <cstring>
#include "communication/MqttTopics.h"

namespace {
void test_builds_all_versioned_topics() {
  MqttTopics topics;
  TEST_ASSERT_TRUE(topics.build("vehicle-01", "device_01"));
  TEST_ASSERT_TRUE(topics.isValid());
  TEST_ASSERT_EQUAL_STRING(
      "vehiclesense/v1/vehicles/vehicle-01/devices/device_01/telemetry",
      topics.telemetry());
  TEST_ASSERT_EQUAL_STRING(
      "vehiclesense/v1/vehicles/vehicle-01/devices/device_01/status",
      topics.status());
  TEST_ASSERT_EQUAL_STRING(
      "vehiclesense/v1/vehicles/vehicle-01/devices/device_01/events",
      topics.events());
  TEST_ASSERT_EQUAL_STRING(
      "vehiclesense/v1/vehicles/vehicle-01/devices/device_01/acoustic",
      topics.acoustic());
  TEST_ASSERT_EQUAL_STRING(
      "vehiclesense/v1/vehicles/vehicle-01/devices/device_01/commands",
      topics.commands());
  TEST_ASSERT_EQUAL_STRING(
      "vehiclesense/v1/vehicles/vehicle-01/devices/device_01/command-acks",
      topics.commandAcks());
}

void test_rejects_topic_metacharacters_and_spaces() {
  MqttTopics topics;
  TEST_ASSERT_FALSE(topics.build("vehicle/01", "device_01"));
  TEST_ASSERT_FALSE(topics.build("vehicle+", "device_01"));
  TEST_ASSERT_FALSE(topics.build("vehicle#", "device_01"));
  TEST_ASSERT_FALSE(topics.build("vehicle 01", "device_01"));
  TEST_ASSERT_FALSE(topics.build("vehicle_01", ""));
}

void test_rejects_identifiers_over_64_characters() {
  char tooLong[66];
  memset(tooLong, 'a', sizeof(tooLong));
  tooLong[65] = '\0';
  MqttTopics topics;
  TEST_ASSERT_FALSE(topics.build(tooLong, "device_01"));
  TEST_ASSERT_FALSE(topics.build("vehicle_01", tooLong));
}
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_builds_all_versioned_topics);
  RUN_TEST(test_rejects_topic_metacharacters_and_spaces);
  RUN_TEST(test_rejects_identifiers_over_64_characters);
  return UNITY_END();
}
