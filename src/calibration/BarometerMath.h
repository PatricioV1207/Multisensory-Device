#pragma once

namespace BarometerMath {

float applyPressureOffset(float rawPressureHpa, float pressureOffsetHpa);
float altitudeFromPressure(float pressureHpa, float seaLevelPressureHpa);
float seaLevelPressureFromAltitude(float altitudeM, float pressureHpa);
bool isValidSeaLevelPressure(float pressureHpa);

}  // namespace BarometerMath
