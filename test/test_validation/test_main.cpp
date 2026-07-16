#include <unity.h>
#include "telemetry/TelemetryValidator.h"

namespace {
TelemetryData validSample() {
  TelemetryData data;
  data.deviceId = "test";
  data.dht = {25.0F, 50.0F, true, 9900};
  data.gps = {-2.1, -79.9, 5.0F, 10.0F, 7, true, true, 100, 9900};
  data.gy801.accel = {0.0F, 0.0F, 9.81F, true, 9900};
  data.gy801.gyro = {0.0F, 0.0F, 0.0F, true, 9900};
  data.gy801.mag = {10.0F, 20.0F, 30.0F, true, 9900};
  data.gy801.barometer = {1013.25F, 25.0F, 0.0F, true, 9900};
  data.gy801.barometer.rawPressureHpa = 1013.25F;
  data.gy801.barometer.seaLevelPressureHpa = 1013.25F;
  return data;
}

void test_valid_data_remains_valid() {
  TelemetryData data = validSample();
  TelemetryValidator::validate(data, 10000);
  TEST_ASSERT_TRUE(data.dht.valid);
  TEST_ASSERT_TRUE(data.gps.valid);
  TEST_ASSERT_TRUE(data.gy801.imuValid);
  TEST_ASSERT_TRUE(data.gy801.barometer.valid);
}

void test_partial_imu_failure_does_not_invalidate_other_components() {
  TelemetryData data = validSample();
  data.gy801.mag.valid = false;
  TelemetryValidator::validate(data, 10000);
  TEST_ASSERT_TRUE(data.gy801.accel.valid);
  TEST_ASSERT_TRUE(data.gy801.gyro.valid);
  TEST_ASSERT_FALSE(data.gy801.mag.valid);
  TEST_ASSERT_FALSE(data.gy801.imuValid);
}

void test_stale_and_out_of_range_values_are_invalidated() {
  TelemetryData data = validSample();
  data.dht.updatedAtMs = 1;
  data.gps.latitude = 120.0;
  data.gy801.barometer.pressureHpa = 1500.0F;
  TelemetryValidator::validate(data, 10000);
  TEST_ASSERT_FALSE(data.dht.valid);
  TEST_ASSERT_FALSE(data.gps.valid);
  TEST_ASSERT_FALSE(data.gy801.barometer.valid);
}
}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_valid_data_remains_valid);
  RUN_TEST(test_partial_imu_failure_does_not_invalidate_other_components);
  RUN_TEST(test_stale_and_out_of_range_values_are_invalidated);
  return UNITY_END();
}
