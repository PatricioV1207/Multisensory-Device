#include "web/LocalWebJsonBuilder.h"

#include <ArduinoJson.h>
#include <cmath>

namespace {
bool serializeDocument(JsonDocument& document, char* output,
                       size_t outputSize) {
  if (output == nullptr || outputSize == 0 ||
      measureJson(document) + 1U > outputSize) {
    if (output != nullptr && outputSize > 0) {
      output[0] = '\0';
    }
    return false;
  }
  const size_t count = serializeJson(document, output, outputSize);
  return count > 0 && count + 1U <= outputSize;
}

float magnitude3(float x, float y, float z) {
  return std::sqrt((x * x) + (y * y) + (z * z));
}
}  // namespace

bool LocalWebJsonBuilder::buildStatus(const LocalWebData& data, char* output,
                                      size_t outputSize) {
  JsonDocument document;
  document["api_version"] = 2;
  document["device_id"] = data.deviceId == nullptr ? "" : data.deviceId;
  document["vehicle_id"] = data.vehicleId == nullptr ? "" : data.vehicleId;
  document["firmware_version"] =
      data.firmwareVersion == nullptr ? "" : data.firmwareVersion;
  document["time_source"] =
      data.timeSource == nullptr ? "none" : data.timeSource;
  document["simulated"] = data.simulated;
  document["uptime_ms"] = data.uptimeMs;

  JsonObject wifi = document["wifi"].to<JsonObject>();
  wifi["configured"] = data.wifi.configured;
  wifi["connected"] = data.wifi.connected;
  wifi["ap_running"] = data.wifi.accessPointRunning;
  if (data.wifi.connected && data.wifi.rssiDbm >= -127 &&
      data.wifi.rssiDbm <= 0) {
    wifi["rssi_dbm"] = data.wifi.rssiDbm;
  }
  if (data.wifi.stationIp[0] != '\0') {
    wifi["station_ip"] = data.wifi.stationIp;
  }
  if (data.wifi.accessPointIp[0] != '\0') {
    wifi["ap_ip"] = data.wifi.accessPointIp;
  }

  JsonObject sd = document["sd"].to<JsonObject>();
  sd["mounted"] = data.storage.mounted;
  sd["last_write_ok"] = data.storage.lastWriteOk;
  sd["last_write_ms"] = data.storage.lastWriteMs;
  sd["free_bytes"] = data.storage.freeBytes;
  sd["used_bytes"] = data.storage.usedBytes;
  sd["total_bytes"] = data.storage.totalBytes;
  sd["boot_session"] = data.storage.bootSession;
  sd["segment"] = data.storage.segment;

  JsonObject sim = document["sim800l"].to<JsonObject>();
  sim["present"] = data.cellular.modemPresent;
  sim["sim_ready"] = data.cellular.simReady;
  sim["network_registered"] = data.cellular.networkRegistered;
  sim["gprs_connected"] = data.cellular.gprsConnected;
  if (data.cellular.modemName[0] != '\0') {
    sim["model"] = data.cellular.modemName;
  }
  if (data.cellular.operatorName[0] != '\0') {
    sim["operator"] = data.cellular.operatorName;
  }
  if (data.cellular.localIp[0] != '\0') {
    sim["local_ip"] = data.cellular.localIp;
  }
  if (data.cellular.signalQuality >= 0) {
    sim["csq"] = data.cellular.signalQuality;
    sim["signal_dbm"] = data.cellular.signalDbm;
  }

  JsonObject mqtt = document["mqtt"].to<JsonObject>();
  mqtt["configured"] = data.mqtt.configured;
  mqtt["connected"] = data.mqtt.connected;
  mqtt["reconnecting"] = data.mqtt.reconnecting;
  mqtt["last_publish_ok"] = data.mqtt.lastPublishOk;
  mqtt["last_publish_ms"] = data.mqtt.lastPublishSuccessMs;
  mqtt["last_puback_ms"] = data.mqtt.lastPublishAckMs;
  mqtt["last_acknowledged_token"] = data.mqtt.lastAcknowledgedToken;
  mqtt["state"] = data.mqtt.clientState;

  JsonObject offline = document["offline"].to<JsonObject>();
  offline["ready"] = data.offline.ready;
  offline["queued"] = data.offline.queued;
  offline["replayed"] = data.offline.replayed;
  offline["dropped"] = data.offline.dropped;
  offline["oldest_age_s"] = data.offline.oldestAgeSeconds;
  offline["bytes"] = data.offline.bytes;

  JsonObject ota = document["ota"].to<JsonObject>();
  ota["enabled"] = data.ota.enabled;
  ota["in_progress"] = data.ota.inProgress;
  ota["last_update_ok"] = data.ota.lastUpdateOk;
  document["alerts_available"] = data.alertsAvailable;
  return serializeDocument(document, output, outputSize);
}

bool LocalWebJsonBuilder::buildBasicTelemetry(const LocalWebData& data,
                                              char* output,
                                              size_t outputSize) {
  JsonDocument document;
  document["api_version"] = 2;
  document["uptime_ms"] = data.uptimeMs;
  document["dht_valid"] = data.dht.valid;
  document["bh1750_valid"] = data.light.valid;
  document["baro_valid"] = data.barometer.valid;
  document["accel_valid"] = data.accel.valid;
  document["gyro_valid"] = data.gyro.valid;
  document["mag_valid"] = data.mag.valid;
  document["imu_valid"] = data.imuValid;
  document["gps_valid"] = data.gps.valid;
  document["gps_stream_seen"] = data.gps.streamSeen;
  document["gps_chars_processed"] = data.gps.charsProcessed;
  document["mic_valid"] = data.acoustic.microphoneValid;
  document["acoustic_valid"] = data.acoustic.analysisValid;
  document["acoustic_alert_active"] = data.acousticAlert.active;
  if (data.acousticAlert.active) {
    document["acoustic_alert_type"] = data.acousticAlert.eventType;
    document["acoustic_alert_severity"] = data.acousticAlert.severity;
    document["acoustic_alert_confidence"] = data.acousticAlert.confidence;
    document["acoustic_alert_duration_ms"] = data.acousticAlert.durationMs;
  }

  if (data.dht.valid && std::isfinite(data.dht.temperatureC) &&
      std::isfinite(data.dht.humidityPercent)) {
    document["temperature_c"] = data.dht.temperatureC;
    document["humidity_percent"] = data.dht.humidityPercent;
  }
  if (data.light.valid && std::isfinite(data.light.lux)) {
    document["light_lux"] = data.light.lux;
  }
  if (data.barometer.valid &&
      std::isfinite(data.barometer.pressureHpa) &&
      std::isfinite(data.barometer.altitudeM)) {
    document["pressure_hpa"] = data.barometer.pressureHpa;
    document["baro_altitude_m"] = data.barometer.altitudeM;
  }
  if (data.accel.valid && std::isfinite(data.accel.x) &&
      std::isfinite(data.accel.y) && std::isfinite(data.accel.z)) {
    document["accel_magnitude_mps2"] =
        magnitude3(data.accel.x, data.accel.y, data.accel.z);
  }
  if (data.gyro.valid && std::isfinite(data.gyro.x) &&
      std::isfinite(data.gyro.y) && std::isfinite(data.gyro.z)) {
    document["gyro_magnitude_rad_s"] =
        magnitude3(data.gyro.x, data.gyro.y, data.gyro.z);
  }
  if (data.mag.valid && std::isfinite(data.mag.x) &&
      std::isfinite(data.mag.y) && std::isfinite(data.mag.z)) {
    document["mag_magnitude_ut"] =
        magnitude3(data.mag.x, data.mag.y, data.mag.z);
  }
  if (data.gps.streamSeen) {
    document["satellites"] = data.gps.satellites;
    if (std::isfinite(data.gps.hdop)) {
      document["gps_hdop"] = data.gps.hdop;
    }
  }
  if (data.gps.valid && std::isfinite(data.gps.latitude) &&
      std::isfinite(data.gps.longitude) &&
      std::isfinite(data.gps.altitudeM) &&
      std::isfinite(data.gps.speedKmh)) {
    document["latitude"] = data.gps.latitude;
    document["longitude"] = data.gps.longitude;
    document["gps_altitude_m"] = data.gps.altitudeM;
    document["speed_kmh"] = data.gps.speedKmh;
  }
  if (data.acoustic.microphoneValid && data.acoustic.analysisValid &&
      std::isfinite(data.acoustic.relativeLevelDbfs) &&
      std::isfinite(data.acoustic.peakDbfs) &&
      std::isfinite(data.acoustic.confidence) &&
      data.acoustic.category != nullptr && data.acoustic.category[0] != '\0') {
    document["acoustic_relative_level_dbfs"] =
        data.acoustic.relativeLevelDbfs;
    document["acoustic_peak_dbfs"] = data.acoustic.peakDbfs;
    document["acoustic_category"] = data.acoustic.category;
    document["acoustic_confidence"] = data.acoustic.confidence;
    document["acoustic_clipping"] = data.acoustic.clipping;
  }
  return serializeDocument(document, output, outputSize);
}
