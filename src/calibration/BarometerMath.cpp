#include "calibration/BarometerMath.h"

#include <cmath>

namespace {
constexpr float kAltitudeScaleM = 44330.0F;
constexpr float kAltitudeExponent = 0.1903F;
constexpr float kSeaLevelExponent = 5.255F;
constexpr float kMinimumSeaLevelPressureHpa = 800.0F;
constexpr float kMaximumSeaLevelPressureHpa = 1100.0F;
}  // namespace

namespace BarometerMath {

float applyPressureOffset(float rawPressureHpa, float pressureOffsetHpa) {
  if (!std::isfinite(rawPressureHpa) || rawPressureHpa <= 0.0F ||
      !std::isfinite(pressureOffsetHpa)) {
    return NAN;
  }
  const float corrected = rawPressureHpa + pressureOffsetHpa;
  return corrected > 0.0F && std::isfinite(corrected) ? corrected : NAN;
}

float altitudeFromPressure(float pressureHpa, float seaLevelPressureHpa) {
  if (!std::isfinite(pressureHpa) || pressureHpa <= 0.0F ||
      !isValidSeaLevelPressure(seaLevelPressureHpa)) {
    return NAN;
  }
  const float ratio = pressureHpa / seaLevelPressureHpa;
  const float altitude =
      kAltitudeScaleM * (1.0F - std::pow(ratio, kAltitudeExponent));
  return std::isfinite(altitude) ? altitude : NAN;
}

float seaLevelPressureFromAltitude(float altitudeM, float pressureHpa) {
  if (!std::isfinite(altitudeM) || altitudeM >= kAltitudeScaleM ||
      !std::isfinite(pressureHpa) || pressureHpa <= 0.0F) {
    return NAN;
  }
  const float base = 1.0F - (altitudeM / kAltitudeScaleM);
  if (base <= 0.0F) {
    return NAN;
  }
  const float seaLevelPressure = pressureHpa / std::pow(base, kSeaLevelExponent);
  return isValidSeaLevelPressure(seaLevelPressure) ? seaLevelPressure : NAN;
}

bool isValidSeaLevelPressure(float pressureHpa) {
  return std::isfinite(pressureHpa) &&
         pressureHpa >= kMinimumSeaLevelPressureHpa &&
         pressureHpa <= kMaximumSeaLevelPressureHpa;
}

}  // namespace BarometerMath
