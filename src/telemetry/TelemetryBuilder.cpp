#include "telemetry/TelemetryBuilder.h"

#include <ArduinoJson.h>
#include <cmath>
#include <cstring>
#include "telemetry_schema.h"

bool TelemetryBuilder::build(const TelemetryData& data, char* output,
                             size_t outputSize, size_t* written) {
  if (written != nullptr) {
    *written = 0;
  }
  if (output == nullptr || outputSize == 0 || data.deviceId == nullptr ||
      data.deviceId[0] == '\0') {
    return false;
  }

  JsonDocument doc;
  doc["schema_version"] = TelemetrySchema::VERSION;
  doc[TelemetrySchema::DEVICE_ID] = data.deviceId;
  doc[TelemetrySchema::UPTIME_MS] = data.uptimeMs;

  doc["dht_valid"] = data.dht.valid;
  doc["gps_valid"] = data.gps.valid;
  doc["accel_valid"] = data.gy801.accel.valid;
  doc["gyro_valid"] = data.gy801.gyro.valid;
  doc["mag_valid"] = data.gy801.mag.valid;
  doc["baro_valid"] = data.gy801.barometer.valid;
  doc["imu_valid"] = data.gy801.imuValid;

  if (data.dht.valid && std::isfinite(data.dht.temperatureC) &&
      std::isfinite(data.dht.humidityPercent)) {
    doc["temperature_c"] = data.dht.temperatureC;
    doc["humidity_percent"] = data.dht.humidityPercent;
  }

  if (data.gps.valid && std::isfinite(data.gps.latitude) &&
      std::isfinite(data.gps.longitude) && std::isfinite(data.gps.altitudeM) &&
      std::isfinite(data.gps.speedKmh)) {
    doc["latitude"] = data.gps.latitude;
    doc["longitude"] = data.gps.longitude;
    doc["altitude_m"] = data.gps.altitudeM;
    doc["speed_kmh"] = data.gps.speedKmh;
    doc["satellites"] = data.gps.satellites;
  }

  if (data.gy801.accel.valid && std::isfinite(data.gy801.accel.x) &&
      std::isfinite(data.gy801.accel.y) && std::isfinite(data.gy801.accel.z)) {
    doc["accel_x"] = data.gy801.accel.x;
    doc["accel_y"] = data.gy801.accel.y;
    doc["accel_z"] = data.gy801.accel.z;
  }

  if (data.gy801.gyro.valid && std::isfinite(data.gy801.gyro.x) &&
      std::isfinite(data.gy801.gyro.y) && std::isfinite(data.gy801.gyro.z)) {
    doc["gyro_x"] = data.gy801.gyro.x;
    doc["gyro_y"] = data.gy801.gyro.y;
    doc["gyro_z"] = data.gy801.gyro.z;
  }

  if (data.gy801.mag.valid && std::isfinite(data.gy801.mag.x) &&
      std::isfinite(data.gy801.mag.y) && std::isfinite(data.gy801.mag.z)) {
    doc["mag_x"] = data.gy801.mag.x;
    doc["mag_y"] = data.gy801.mag.y;
    doc["mag_z"] = data.gy801.mag.z;
  }

  if (data.gy801.barometer.valid &&
      std::isfinite(data.gy801.barometer.pressureHpa) &&
      std::isfinite(data.gy801.barometer.seaLevelPressureHpa) &&
      std::isfinite(data.gy801.barometer.temperatureC) &&
      std::isfinite(data.gy801.barometer.altitudeM)) {
    doc["pressure_hpa"] = data.gy801.barometer.pressureHpa;
    doc[TelemetrySchema::SEA_LEVEL_PRESSURE_HPA] =
        data.gy801.barometer.seaLevelPressureHpa;
    doc["baro_temperature_c"] = data.gy801.barometer.temperatureC;
    doc["baro_altitude_m"] = data.gy801.barometer.altitudeM;
  }

  const size_t required = measureJson(doc) + 1U;
  if (required > outputSize) {
    output[0] = '\0';
    return false;
  }

  const size_t count = serializeJson(doc, output, outputSize);
  if (count == 0 || count + 1U > outputSize) {
    output[0] = '\0';
    return false;
  }
  if (written != nullptr) {
    *written = count;
  }
  return true;
}
