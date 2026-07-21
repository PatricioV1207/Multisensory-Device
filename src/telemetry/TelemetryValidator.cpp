#include "telemetry/TelemetryValidator.h"

#include <cmath>
#include <cstring>
#include "config.h"

namespace {
bool finite3(float x, float y, float z) {
  return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

bool knownAcousticCategory(const char* category) {
  constexpr const char* categories[] = {
      "traffic", "music", "speech", "engine", "horn",
      "siren",   "wind",  "quiet",  "unknown"};
  if (category == nullptr) {
    return false;
  }
  for (const char* candidate : categories) {
    if (std::strcmp(category, candidate) == 0) {
      return true;
    }
  }
  return false;
}
}  // namespace

bool TelemetryValidator::isFresh(uint32_t nowMs, uint32_t updatedAtMs,
                                 uint32_t maxAgeMs) {
  return updatedAtMs != 0 && static_cast<uint32_t>(nowMs - updatedAtMs) <= maxAgeMs;
}

void TelemetryValidator::validate(TelemetryData& data, uint32_t nowMs) {
  data.dht.valid = data.dht.valid &&
                   isFresh(nowMs, data.dht.updatedAtMs, DHT_READ_INTERVAL_MS * 2UL) &&
                   std::isfinite(data.dht.temperatureC) &&
                   std::isfinite(data.dht.humidityPercent) &&
                   data.dht.temperatureC >= 0.0F && data.dht.temperatureC <= 50.0F &&
                   data.dht.humidityPercent >= 20.0F && data.dht.humidityPercent <= 90.0F;

  data.gps.valid = data.gps.valid &&
                   isFresh(nowMs, data.gps.updatedAtMs, GPS_FIX_MAX_AGE_MS) &&
                   std::isfinite(data.gps.latitude) && std::isfinite(data.gps.longitude) &&
                   std::isfinite(data.gps.altitudeM) && std::isfinite(data.gps.speedKmh) &&
                   data.gps.latitude >= -90.0 && data.gps.latitude <= 90.0 &&
                   data.gps.longitude >= -180.0 && data.gps.longitude <= 180.0;

  data.light.valid = data.light.valid &&
                     isFresh(nowMs, data.light.updatedAtMs,
                             LIGHT_READ_INTERVAL_MS * 3UL) &&
                     std::isfinite(data.light.lux) && data.light.lux >= 0.0F &&
                     data.light.lux <= BH1750_MAX_LUX;

  data.gy801.accel.valid = data.gy801.accel.valid &&
                           isFresh(nowMs, data.gy801.accel.updatedAtMs,
                                   IMU_READ_INTERVAL_MS * 5UL) &&
                           finite3(data.gy801.accel.x, data.gy801.accel.y,
                                   data.gy801.accel.z);

  data.gy801.gyro.valid = data.gy801.gyro.valid &&
                          isFresh(nowMs, data.gy801.gyro.updatedAtMs,
                                  IMU_READ_INTERVAL_MS * 5UL) &&
                          finite3(data.gy801.gyro.x, data.gy801.gyro.y,
                                  data.gy801.gyro.z);

  data.gy801.mag.valid = data.gy801.mag.valid &&
                         isFresh(nowMs, data.gy801.mag.updatedAtMs,
                                 IMU_READ_INTERVAL_MS * 5UL) &&
                         finite3(data.gy801.mag.x, data.gy801.mag.y,
                                 data.gy801.mag.z);

  data.gy801.barometer.valid = data.gy801.barometer.valid &&
                               isFresh(nowMs, data.gy801.barometer.updatedAtMs,
                                       BARO_READ_INTERVAL_MS * 2UL) &&
                               std::isfinite(
                                   data.gy801.barometer.rawPressureHpa) &&
                               std::isfinite(data.gy801.barometer.pressureHpa) &&
                               std::isfinite(
                                   data.gy801.barometer.seaLevelPressureHpa) &&
                               std::isfinite(data.gy801.barometer.temperatureC) &&
                               std::isfinite(data.gy801.barometer.altitudeM) &&
                               data.gy801.barometer.rawPressureHpa >= 300.0F &&
                               data.gy801.barometer.rawPressureHpa <= 1100.0F &&
                               data.gy801.barometer.pressureHpa >= 300.0F &&
                               data.gy801.barometer.pressureHpa <= 1100.0F &&
                               data.gy801.barometer.seaLevelPressureHpa >=
                                   800.0F &&
                               data.gy801.barometer.seaLevelPressureHpa <=
                                   1100.0F;

  data.gy801.imuValid = data.gy801.accel.valid && data.gy801.gyro.valid &&
                        data.gy801.mag.valid;

  data.acoustic.microphoneValid =
      data.acoustic.microphoneValid &&
      isFresh(nowMs, data.acoustic.updatedAtMs, ACOUSTIC_STALE_MS);
  data.acoustic.analysisValid =
      data.acoustic.microphoneValid && data.acoustic.analysisValid &&
      std::isfinite(data.acoustic.relativeLevelDbfs) &&
      std::isfinite(data.acoustic.peakDbfs) &&
      std::isfinite(data.acoustic.clippingRatio) &&
      std::isfinite(data.acoustic.confidence) &&
      data.acoustic.relativeLevelDbfs >= -160.0F &&
      data.acoustic.relativeLevelDbfs <= 0.0F &&
      data.acoustic.peakDbfs >= -160.0F && data.acoustic.peakDbfs <= 0.0F &&
      data.acoustic.clippingRatio >= 0.0F &&
      data.acoustic.clippingRatio <= 1.0F && data.acoustic.confidence >= 0.0F &&
      data.acoustic.confidence <= 1.0F &&
      knownAcousticCategory(data.acoustic.category) &&
      data.acoustic.classifierVersion != nullptr &&
      data.acoustic.classifierVersion[0] != '\0' &&
      std::strlen(data.acoustic.classifierVersion) <= 40U;
}
