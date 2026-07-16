#include "calibration/BarometerCalibrationStore.h"

#include <Preferences.h>
#include "calibration/BarometerMath.h"
#include "config.h"

namespace {
constexpr const char* kNamespace = "baro_cal";
constexpr const char* kVersionKey = "version";
constexpr const char* kPressureKey = "p0_hpa";
constexpr const char* kSourceKey = "source";
constexpr uint16_t kStorageVersion = 1;

bool isPersistentSource(BarometerCalibrationSource source) {
  return source == BarometerCalibrationSource::KnownAltitude ||
         source == BarometerCalibrationSource::GPS;
}
}  // namespace

StoredBarometerCalibration BarometerCalibrationStore::load() {
  StoredBarometerCalibration result{BARO_DEFAULT_SEA_LEVEL_PRESSURE_HPA,
                                    BarometerCalibrationSource::Default,
                                    false};
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return result;
  }

  const uint16_t version = preferences.getUShort(kVersionKey, 0);
  const float pressure = preferences.getFloat(kPressureKey, NAN);
  const auto source = static_cast<BarometerCalibrationSource>(
      preferences.getUChar(kSourceKey, 0));
  preferences.end();

  if (version == kStorageVersion &&
      BarometerMath::isValidSeaLevelPressure(pressure) &&
      isPersistentSource(source)) {
    result.seaLevelPressureHpa = pressure;
    result.source = source;
    result.persisted = true;
  }
  return result;
}

bool BarometerCalibrationStore::save(float seaLevelPressureHpa,
                                     BarometerCalibrationSource source) {
  if (!BarometerMath::isValidSeaLevelPressure(seaLevelPressureHpa) ||
      !isPersistentSource(source)) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return false;
  }
  const bool cleared = preferences.clear();
  const bool versionSaved =
      preferences.putUShort(kVersionKey, kStorageVersion) == sizeof(uint16_t);
  const bool pressureSaved =
      preferences.putFloat(kPressureKey, seaLevelPressureHpa) == sizeof(float);
  const bool sourceSaved =
      preferences.putUChar(kSourceKey, static_cast<uint8_t>(source)) ==
      sizeof(uint8_t);
  preferences.end();
  return cleared && versionSaved && pressureSaved && sourceSaved;
}

bool BarometerCalibrationStore::clear() {
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return false;
  }
  const bool cleared = preferences.clear();
  preferences.end();
  return cleared;
}

const char* BarometerCalibrationStore::sourceName(
    BarometerCalibrationSource source) {
  switch (source) {
    case BarometerCalibrationSource::KnownAltitude:
      return "known_altitude";
    case BarometerCalibrationSource::GPS:
      return "gps";
    case BarometerCalibrationSource::Default:
    default:
      return "default";
  }
}
