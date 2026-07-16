#include "telemetry/TelemetryValidator.h"

#include <cmath>
#include "config.h"

namespace {
bool finite3(float x, float y, float z) {
  return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
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
}
