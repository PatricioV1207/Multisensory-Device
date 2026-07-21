#include "telemetry/TelemetryBuilder.h"

#include <ArduinoJson.h>
#include <cmath>
#include <cstring>
#include "telemetry_schema.h"

namespace {
bool inRange(float value, float minimum, float maximum) {
  return std::isfinite(value) && value >= minimum && value <= maximum;
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
  doc["bh1750_valid"] = data.light.valid;
  doc["sd_valid"] = data.storage.mounted;
  doc["sim_valid"] = data.cellular.modemPresent && data.cellular.simReady;
  doc["gprs_connected"] = data.cellular.gprsConnected;

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

  if (data.light.valid && std::isfinite(data.light.lux)) {
    doc["light_lux"] = data.light.lux;
  }

  if (data.cellular.signalQuality >= 0 &&
      data.cellular.signalQuality <= 31) {
    doc["gsm_csq"] = data.cellular.signalQuality;
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

bool TelemetryBuilder::buildV3(const TelemetryData& data, char* output,
                               size_t outputSize, size_t* written) {
  if (written != nullptr) {
    *written = 0;
  }
  if (output == nullptr || outputSize == 0 || data.deviceId == nullptr ||
      data.deviceId[0] == '\0' || data.vehicleId == nullptr ||
      data.vehicleId[0] == '\0' || data.sampleId == nullptr ||
      data.sampleId[0] == '\0' || data.bootId == 0U || data.sequence == 0U) {
    return false;
  }

  const bool timeValid = data.timeValid && data.measuredAt != nullptr &&
                         data.measuredAt[0] != '\0';
  const bool dhtValid = data.dht.valid &&
                        std::isfinite(data.dht.temperatureC) &&
                        std::isfinite(data.dht.humidityPercent);
  const bool gpsValid =
      data.gps.valid && std::isfinite(data.gps.latitude) &&
      std::isfinite(data.gps.longitude) && std::isfinite(data.gps.altitudeM) &&
      std::isfinite(data.gps.speedKmh) && std::isfinite(data.gps.hdop);
  const bool accelValid = data.gy801.accel.valid &&
                          std::isfinite(data.gy801.accel.x) &&
                          std::isfinite(data.gy801.accel.y) &&
                          std::isfinite(data.gy801.accel.z);
  const bool gyroValid = data.gy801.gyro.valid &&
                         std::isfinite(data.gy801.gyro.x) &&
                         std::isfinite(data.gy801.gyro.y) &&
                         std::isfinite(data.gy801.gyro.z);
  const bool magValid = data.gy801.mag.valid &&
                        std::isfinite(data.gy801.mag.x) &&
                        std::isfinite(data.gy801.mag.y) &&
                        std::isfinite(data.gy801.mag.z);
  const bool baroValid =
      data.gy801.barometer.valid &&
      std::isfinite(data.gy801.barometer.pressureHpa) &&
      std::isfinite(data.gy801.barometer.seaLevelPressureHpa) &&
      std::isfinite(data.gy801.barometer.temperatureC) &&
      std::isfinite(data.gy801.barometer.altitudeM);
  const bool lightValid = data.light.valid && std::isfinite(data.light.lux);
  const bool acousticValid =
      data.acoustic.microphoneValid && data.acoustic.analysisValid &&
      inRange(data.acoustic.relativeLevelDbfs, -160.0F, 0.0F) &&
      inRange(data.acoustic.peakDbfs, -160.0F, 0.0F) &&
      inRange(data.acoustic.confidence, 0.0F, 1.0F) &&
      knownAcousticCategory(data.acoustic.category) &&
      data.acoustic.classifierVersion != nullptr &&
      data.acoustic.classifierVersion[0] != '\0' &&
      std::strlen(data.acoustic.classifierVersion) <= 40U;

  JsonDocument doc;
  doc["schema_version"] = TelemetrySchema::VEHICLESENSE_VERSION;
  doc["message_type"] = "telemetry";
  doc[TelemetrySchema::DEVICE_ID] = data.deviceId;
  doc[TelemetrySchema::VEHICLE_ID] = data.vehicleId;
  doc["boot_id"] = data.bootId;
  doc["sequence"] = data.sequence;
  doc["sample_id"] = data.sampleId;
  doc[TelemetrySchema::UPTIME_MS] = data.uptimeMs;
  doc["time_valid"] = timeValid;
  if (timeValid) {
    doc["measured_at"] = data.measuredAt;
  }
  doc["replayed"] = data.replayed;
  doc["simulated"] = data.simulated;

  doc["dht_valid"] = dhtValid;
  doc["gps_valid"] = gpsValid;
  doc["accel_valid"] = accelValid;
  doc["gyro_valid"] = gyroValid;
  doc["mag_valid"] = magValid;
  doc["baro_valid"] = baroValid;
  doc["imu_valid"] = accelValid && gyroValid && magValid;
  doc["bh1750_valid"] = lightValid;
  doc["sd_valid"] = data.storage.mounted;
  doc["wifi_connected"] = data.wifiConnected;
  doc["mqtt_connected"] = data.mqttConnected;
  doc["mic_valid"] = data.acoustic.microphoneValid;
  doc["acoustic_valid"] = acousticValid;

  if (dhtValid) {
    doc["temperature_c"] = data.dht.temperatureC;
    doc["humidity_percent"] = data.dht.humidityPercent;
  }
  if (gpsValid) {
    doc["latitude"] = data.gps.latitude;
    doc["longitude"] = data.gps.longitude;
    doc["altitude_m"] = data.gps.altitudeM;
    doc["speed_kmh"] = data.gps.speedKmh;
    doc["satellites"] = data.gps.satellites;
    doc["gps_hdop"] = data.gps.hdop;
  }
  if (accelValid) {
    doc["accel_x"] = data.gy801.accel.x;
    doc["accel_y"] = data.gy801.accel.y;
    doc["accel_z"] = data.gy801.accel.z;
  }
  if (gyroValid) {
    doc["gyro_x"] = data.gy801.gyro.x;
    doc["gyro_y"] = data.gy801.gyro.y;
    doc["gyro_z"] = data.gy801.gyro.z;
  }
  if (magValid) {
    doc["mag_x"] = data.gy801.mag.x;
    doc["mag_y"] = data.gy801.mag.y;
    doc["mag_z"] = data.gy801.mag.z;
  }
  if (baroValid) {
    doc["pressure_hpa"] = data.gy801.barometer.pressureHpa;
    doc[TelemetrySchema::SEA_LEVEL_PRESSURE_HPA] =
        data.gy801.barometer.seaLevelPressureHpa;
    doc["baro_temperature_c"] = data.gy801.barometer.temperatureC;
    doc["baro_altitude_m"] = data.gy801.barometer.altitudeM;
  }
  if (lightValid) {
    doc["light_lux"] = data.light.lux;
  }
  if (data.wifiConnected && data.wifiRssiDbm >= -127 &&
      data.wifiRssiDbm <= 0) {
    doc["wifi_rssi_dbm"] = data.wifiRssiDbm;
  }
  doc["offline_queued"] = data.offline.queued;
  doc["offline_replayed"] = data.offline.replayed;
  doc["offline_dropped"] = data.offline.dropped;
  doc["offline_oldest_age_s"] = data.offline.oldestAgeSeconds;
  if (acousticValid) {
    doc["acoustic_relative_level_dbfs"] = data.acoustic.relativeLevelDbfs;
    doc["acoustic_peak_dbfs"] = data.acoustic.peakDbfs;
    doc["acoustic_category"] = data.acoustic.category;
    doc["acoustic_confidence"] = data.acoustic.confidence;
    doc["acoustic_clipping"] = data.acoustic.clipping;
    doc["acoustic_classifier_version"] =
        data.acoustic.classifierVersion;
  }

  const size_t required = measureJson(doc) + 1U;
  if (required > outputSize) {
    output[0] = '\0';
    return false;
  }
  const size_t count = serializeJson(doc, output, outputSize);
  if (count == 0U || count + 1U > outputSize) {
    output[0] = '\0';
    return false;
  }
  if (written != nullptr) {
    *written = count;
  }
  return true;
}
