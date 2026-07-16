#include <cmath>
#include <unity.h>
#include "calibration/BarometerMath.h"

namespace {
void test_real_samples_match_known_altitudes_with_1008_hpa_reference() {
  const float homeAltitude =
      BarometerMath::altitudeFromPressure(1006.6525F, 1008.00F);
  const float universityAltitude =
      BarometerMath::altitudeFromPressure(998.015F, 1008.00F);

  TEST_ASSERT_FLOAT_WITHIN(0.10F, 11.28F, homeAltitude);
  TEST_ASSERT_FLOAT_WITHIN(0.10F, 83.90F, universityAltitude);
}

void test_inverse_formula_reconstructs_observed_sea_level_references() {
  const float homeReference =
      BarometerMath::seaLevelPressureFromAltitude(10.0F, 1006.6525F);
  const float universityReference =
      BarometerMath::seaLevelPressureFromAltitude(85.0F, 998.015F);

  TEST_ASSERT_FLOAT_WITHIN(0.01F, 1007.85F, homeReference);
  TEST_ASSERT_FLOAT_WITHIN(0.01F, 1008.13F, universityReference);
}

void test_pressure_offset_is_applied_before_altitude_conversion() {
  const float corrected =
      BarometerMath::applyPressureOffset(1006.50F, 0.15F);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 1006.65F, corrected);
  TEST_ASSERT_FLOAT_WITHIN(
      0.10F, 11.30F,
      BarometerMath::altitudeFromPressure(corrected, 1008.00F));
}

void test_invalid_values_are_rejected() {
  TEST_ASSERT_TRUE(std::isnan(
      BarometerMath::altitudeFromPressure(-1.0F, 1008.0F)));
  TEST_ASSERT_TRUE(std::isnan(
      BarometerMath::altitudeFromPressure(1000.0F, 1200.0F)));
  TEST_ASSERT_TRUE(std::isnan(
      BarometerMath::seaLevelPressureFromAltitude(44330.0F, 1000.0F)));
  TEST_ASSERT_FALSE(BarometerMath::isValidSeaLevelPressure(799.9F));
  TEST_ASSERT_FALSE(BarometerMath::isValidSeaLevelPressure(1100.1F));
}
}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_real_samples_match_known_altitudes_with_1008_hpa_reference);
  RUN_TEST(test_inverse_formula_reconstructs_observed_sea_level_references);
  RUN_TEST(test_pressure_offset_is_applied_before_altitude_conversion);
  RUN_TEST(test_invalid_values_are_rejected);
  return UNITY_END();
}
