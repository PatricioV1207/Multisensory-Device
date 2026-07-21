#include "app/AppController.h"

#include <Wire.h>
#include <ArduinoJson.h>
#include <esp_timer.h>
#include <cstring>
#include "calibration/BarometerCalibrationStore.h"
#include "config.h"
#include "pins.h"
#include "telemetry/TelemetryBuilder.h"
#include "telemetry/TelemetryValidator.h"
#include "utils/Logger.h"

void AppController::begin() {
  Logger::begin(SERIAL_BAUD);
  Logger::info("BOOT", "Starting " + String(DEVICE_ID) +
                           " app_mode=" + String(APP_MODE));

#if APP_MODE != APP_MODE_FULL_PROTOTYPE && \
    APP_MODE != APP_MODE_FULL_PROTOTYPE_CELLULAR && \
    APP_MODE != APP_MODE_VEHICLESENSE_WIFI
  _testRunner.begin();
  return;
#endif

#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  _identity.begin();
  _time.begin();
#endif

  beginSharedSensors();

#if APP_MODE == APP_MODE_FULL_PROTOTYPE
  _wifi.begin();
  _mqtt.begin(_wifiClient);
#elif APP_MODE == APP_MODE_FULL_PROTOTYPE_CELLULAR
  _light.begin();
  _storage.begin();
  _web.begin();
  _modem.begin(SIM800LMode::Gprs);

  constexpr bool useTls = CELLULAR_MQTT_USE_TLS != 0;
  constexpr bool transportAllowed =
      useTls || (ALLOW_INSECURE_CELLULAR_MQTT != 0);
  if (transportAllowed) {
    const MqttConnectionConfig cellularMqtt{
        CELLULAR_MQTT_HOST, CELLULAR_MQTT_PORT, CELLULAR_MQTT_USERNAME,
        CELLULAR_MQTT_PASSWORD, CELLULAR_MQTT_TOPIC};
    _mqtt.begin(_modem.networkClient(useTls), cellularMqtt);
  } else {
    Logger::error("CONFIG", "Cellular MQTT blocked: enable TLS or explicit "
                            "lab-only insecure override");
  }
#elif APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  _wifi.begin();
  _light.begin();
  _storage.begin();
  _web.begin(true);
  if (_identity.isValid()) {
    _secureMqtt.begin(_identity.bootId());
  }
#endif
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  _mqttStatus.configured = _secureMqtt.isConfigured();
#else
  _mqttStatus.configured = _mqtt.isConfigured();
#endif
  Logger::info("BOOT", "Initialization complete; failures are non-fatal");
}

void AppController::beginSharedSensors() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  _dht.begin();
  _gps.begin();
  const StoredBarometerCalibration barometerCalibration =
      BarometerCalibrationStore::load();
  _gy801.setBarometerCalibration(
      barometerCalibration.seaLevelPressureHpa, BARO_PRESSURE_OFFSET_HPA,
      barometerCalibration.source);
  _gy801.begin();
  Logger::info(
      "BARO",
      "Reference p0=" + String(barometerCalibration.seaLevelPressureHpa, 3) +
          " hPa source=" +
          BarometerCalibrationStore::sourceName(barometerCalibration.source));
}

void AppController::update() {
  const uint32_t nowMs = millis();

#if APP_MODE != APP_MODE_FULL_PROTOTYPE && \
    APP_MODE != APP_MODE_FULL_PROTOTYPE_CELLULAR && \
    APP_MODE != APP_MODE_VEHICLESENSE_WIFI
  _testRunner.update(nowMs);
  delay(1);
  return;
#endif

  _gps.update(nowMs);
  _dht.update(nowMs);
  _gy801.update(nowMs);

#if APP_MODE == APP_MODE_FULL_PROTOTYPE
  _wifi.update(nowMs);
  _mqtt.update(nowMs, _wifi.isConnected());
#elif APP_MODE == APP_MODE_FULL_PROTOTYPE_CELLULAR
  _light.update(nowMs);
  _storage.update(nowMs);
  _modem.update(nowMs);
  _mqtt.update(nowMs, _modem.isGprsConnected());
  _mqttStatus.configured = _mqtt.isConfigured();
  _mqttStatus.connected = _mqtt.isConnected();
  _mqttStatus.clientState = static_cast<int16_t>(_mqtt.state());
  _web.update(nowMs);
  if (static_cast<uint32_t>(nowMs - _lastWebSnapshotMs) >=
      LOCAL_WEB_SNAPSHOT_INTERVAL_MS) {
    _lastWebSnapshotMs = nowMs;
    refreshLocalWebData(nowMs);
  }
#elif APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  _light.update(nowMs);
  _storage.update(nowMs);
  _wifi.update(nowMs);
  _time.update(nowMs, _wifi.isConnected(), _gps.getData());
  _secureMqtt.update(nowMs, _wifi.isConnected() && _time.isValid());
  processVehicleSenseMqtt(nowMs);
  _web.update(nowMs);
  if (static_cast<uint32_t>(nowMs - _lastWebSnapshotMs) >=
      LOCAL_WEB_SNAPSHOT_INTERVAL_MS) {
    _lastWebSnapshotMs = nowMs;
    refreshLocalWebData(nowMs);
  }
#endif

  if (static_cast<uint32_t>(nowMs - _lastTelemetryMs) >= TELEMETRY_INTERVAL_MS) {
    _lastTelemetryMs = nowMs;
    publishTelemetry(nowMs);
  }
  delay(1);
}

void AppController::buildSnapshot(uint32_t nowMs) {
  _telemetry.deviceId = DEVICE_ID;
  _telemetry.uptimeMs = static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
  _telemetry.dht = _dht.getData();
  _telemetry.gps = _gps.getData();
  _telemetry.gy801 = _gy801.getData();
  _telemetry.light = _light.getData();
  _telemetry.storage = _storage.getStatus();
  _telemetry.cellular = _modem.getStatus();
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  _telemetry.vehicleId = VEHICLE_ID;
  _telemetry.bootId = _identity.bootId();
  _telemetry.sampleId = _sampleId;
  _telemetry.measuredAt = _measuredAt;
  _telemetry.replayed = false;
  _telemetry.simulated = false;
  _telemetry.wifiConnected = _wifi.isConnected();
  _telemetry.wifiRssiDbm = _wifi.rssi();
  _telemetry.mqttConnected = _secureMqtt.isConnected();
#endif
  TelemetryValidator::validate(_telemetry, nowMs);
}

void AppController::publishTelemetry(uint32_t nowMs) {
  buildSnapshot(nowMs);
  size_t written = 0;
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  if (!_identity.isValid()) {
    Logger::error("IDENTITY", "Telemetry v3 skipped; persistent identity unavailable");
    return;
  }
  _telemetry.sequence = _identity.nextSequence();
  if (!_identity.formatSampleId(_telemetry.sequence, _sampleId,
                                sizeof(_sampleId))) {
    Logger::error("IDENTITY", "Telemetry v3 skipped; sample_id overflow");
    return;
  }
  _telemetry.timeValid =
      _time.formatIso8601(_measuredAt, sizeof(_measuredAt));
  const bool built = TelemetryBuilder::buildV3(
      _telemetry, _payload, sizeof(_payload), &written);
#else
  const bool built = TelemetryBuilder::build(
      _telemetry, _payload, sizeof(_payload), &written);
#endif
  if (!built) {
    Logger::error("JSON", "Serialization failed or payload buffer is too small");
    return;
  }

  Logger::info("JSON", String(_payload));
  Logger::debug("JSON", "Payload bytes=" + String(written));

#if APP_MODE == APP_MODE_FULL_PROTOTYPE_CELLULAR || \
    APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  if (_web.otaInProgress()) {
    Logger::warn("OTA", "Storage and MQTT paused during firmware upload");
    return;
  }
  if (!_storage.appendJsonLine(_payload, written, nowMs)) {
    Logger::warn("SD", "Telemetry sample was not persisted");
  }
#endif

#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  if (!_secureMqtt.isConnected()) {
    Logger::warn("MQTT", "v3 sample not published; HiveMQ offline");
    return;
  }
  _mqttStatus.lastPublishAttemptMs = nowMs;
  if (_secureMqtt.publishTelemetry(_payload, written, _telemetry.sequence)) {
    _mqttStatus.lastPublishOk = false;
    Logger::info("MQTT", "Telemetry queued with QoS 1; awaiting PUBACK");
  } else {
    Logger::warn("MQTT", "Telemetry queue busy or publish rejected");
  }
  return;
#endif

  if (!_mqtt.isConnected()) {
    Logger::warn("MQTT", "Telemetry not published; broker offline (no local queue)");
    return;
  }
  _mqttStatus.lastPublishAttemptMs = nowMs;
  if (_mqtt.publish(_payload)) {
    _mqttStatus.lastPublishOk = true;
    _mqttStatus.lastPublishSuccessMs = nowMs;
    Logger::info("MQTT", "Telemetry published");
  } else {
    _mqttStatus.lastPublishOk = false;
  }
}

void AppController::processVehicleSenseMqtt(uint32_t nowMs) {
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  _mqttStatus.configured = _secureMqtt.isConfigured();
  _mqttStatus.connected = _secureMqtt.isConnected();
  _mqttStatus.reconnecting = _secureMqtt.isReconnecting();
  _mqttStatus.clientState =
      static_cast<int16_t>(_secureMqtt.lastError() & 0x7FFF);

  uint32_t token = 0;
  uint32_t acknowledgedAtMs = 0;
  if (_secureMqtt.takeTelemetryAck(token, acknowledgedAtMs)) {
    _mqttStatus.lastPublishOk = true;
    _mqttStatus.lastPublishSuccessMs = acknowledgedAtMs;
    _mqttStatus.lastPublishAckMs = acknowledgedAtMs;
    _mqttStatus.lastAcknowledgedToken = token;
    Logger::info("MQTT", "Telemetry PUBACK token=" + String(token));
  }

  size_t commandLength = 0;
  if (!_secureMqtt.takeCommand(_commandBuffer, sizeof(_commandBuffer),
                               &commandLength)) {
    return;
  }
  JsonDocument command;
  const DeserializationError error =
      deserializeJson(command, _commandBuffer, commandLength);
  if (error || command["schema_version"].as<int>() != 1 ||
      strcmp(command["message_type"] | "", "command") != 0) {
    Logger::warn("MQTT", "Malformed command rejected without execution");
    return;
  }
  const char* commandId = command["command_id"] | "";
  const char* targetDevice = command["device_id"] | "";
  const char* targetVehicle = command["vehicle_id"] | "";
  if (commandId[0] == '\0' || strcmp(targetDevice, DEVICE_ID) != 0 ||
      strcmp(targetVehicle, VEHICLE_ID) != 0) {
    Logger::warn("MQTT", "Command target mismatch or missing command_id");
    return;
  }
  acknowledgeUnsupportedCommand(commandId, nowMs);
#else
  (void)nowMs;
#endif
}

void AppController::acknowledgeUnsupportedCommand(const char* commandId,
                                                  uint32_t nowMs) {
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  (void)nowMs;
  JsonDocument ack;
  ack["schema_version"] = 1;
  ack["message_type"] = "command_ack";
  ack["command_id"] = commandId;
  ack["device_id"] = DEVICE_ID;
  ack["vehicle_id"] = VEHICLE_ID;
  ack["state"] = "unsupported";
  ack["uptime_ms"] = static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
  const bool timeValid =
      _time.formatIso8601(_measuredAt, sizeof(_measuredAt));
  ack["time_valid"] = timeValid;
  if (timeValid) {
    ack["acknowledged_at"] = _measuredAt;
  }
  ack["error_code"] = "COMMAND_HANDLER_DEFERRED";
  ack["message"] = "Command execution is not enabled in this firmware phase";
  ack["simulated"] = false;
  const size_t length =
      serializeJson(ack, _commandAckPayload, sizeof(_commandAckPayload));
  if (length == 0U || length >= sizeof(_commandAckPayload) ||
      !_secureMqtt.publishCommandAck(_commandAckPayload, length)) {
    Logger::warn("MQTT", "Command acknowledgement could not be queued");
    return;
  }
  Logger::warn("MQTT", "Command acknowledged as unsupported; id=" +
                           String(commandId));
#else
  (void)commandId;
  (void)nowMs;
#endif
}

void AppController::refreshLocalWebData(uint32_t nowMs) {
#if APP_MODE == APP_MODE_FULL_PROTOTYPE_CELLULAR || \
    APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  buildSnapshot(nowMs);
  LocalWebData local;
  local.deviceId = DEVICE_ID;
  local.vehicleId = VEHICLE_ID;
  local.firmwareVersion = FIRMWARE_VERSION;
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  local.timeSource = _time.sourceName();
#endif
  local.uptimeMs = _telemetry.uptimeMs;
  local.dht = _telemetry.dht;
  local.light = _telemetry.light;
  local.gps = _telemetry.gps;
  local.accel = _telemetry.gy801.accel;
  local.gyro = _telemetry.gy801.gyro;
  local.mag = _telemetry.gy801.mag;
  local.barometer = _telemetry.gy801.barometer;
  local.imuValid = _telemetry.gy801.imuValid;
  local.storage = _storage.getStatus();
  local.cellular = _modem.getStatus();
#if APP_MODE == APP_MODE_VEHICLESENSE_WIFI
  local.wifi.configured = _wifi.isConfigured();
  local.wifi.connected = _wifi.isConnected();
  local.wifi.accessPointRunning = _web.isRunning();
  local.wifi.rssiDbm = _wifi.rssi();
  const String stationIp = _wifi.localIp();
  const String accessPointIp = _web.localIp();
  if (_wifi.isConnected()) {
    stationIp.toCharArray(local.wifi.stationIp,
                          sizeof(local.wifi.stationIp));
  }
  if (_web.isRunning()) {
    accessPointIp.toCharArray(local.wifi.accessPointIp,
                              sizeof(local.wifi.accessPointIp));
  }
#else
  local.wifi.accessPointRunning = _web.isRunning();
  const String accessPointIp = _web.localIp();
  if (_web.isRunning()) {
    accessPointIp.toCharArray(local.wifi.accessPointIp,
                              sizeof(local.wifi.accessPointIp));
  }
#endif
  local.mqtt = _mqttStatus;
  local.ota = _web.getOtaStatus();
  local.offline = _telemetry.offline;
  local.acoustic = _telemetry.acoustic;
  local.alertsAvailable = false;
  _web.setData(local);
#else
  (void)nowMs;
#endif
}
