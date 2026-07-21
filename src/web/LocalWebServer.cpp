#include "web/LocalWebServer.h"

#include <WiFi.h>
#include <cstring>
#include "config.h"
#include "utils/Logger.h"
#include "web/LocalWebJsonBuilder.h"
#include "web/WebAssets.h"

namespace {
bool hasRealValue(const char* value) {
  return value != nullptr && value[0] != '\0' &&
         strncmp(value, "YOUR_", 5) != 0;
}
}

bool LocalWebServer::begin(bool preserveStation) {
  if (!hasRealValue(LOCAL_AP_SSID) || !hasRealValue(LOCAL_AP_PASSWORD) ||
      strlen(LOCAL_AP_PASSWORD) < 8U) {
    Logger::error("WEB", "AP credentials missing or password shorter than 8 chars");
    return false;
  }
  WiFi.persistent(false);
  WiFi.mode(preserveStation ? WIFI_AP_STA : WIFI_AP);
  if (!WiFi.softAP(LOCAL_AP_SSID, LOCAL_AP_PASSWORD)) {
    Logger::error("WEB", "Could not start local access point");
    return false;
  }
  _ota.begin(_server, LOCAL_ADMIN_USERNAME, LOCAL_ADMIN_PASSWORD);
  registerRoutes();
  _server.begin();
  _running = true;
  Logger::info("WEB", "Local monitor ready at http://" + localIp());
  return true;
}

void LocalWebServer::registerRoutes() {
  _server.on("/", HTTP_GET, [this]() {
    _server.send_P(200, "text/html; charset=utf-8",
                   WebAssets::DASHBOARD_HTML);
  });
  _server.on("/api/status", HTTP_GET, [this]() { sendStatusJson(); });
  _server.on("/api/telemetry/basic", HTTP_GET,
             [this]() { sendBasicTelemetryJson(); });
  _server.on("/admin", HTTP_GET, [this]() {
    if (!_ota.authenticate()) {
      return;
    }
    _server.send_P(200, "text/html; charset=utf-8", WebAssets::ADMIN_HTML);
  });
  _server.onNotFound([this]() {
    _server.send(404, "application/json", "{\"error\":\"not_found\"}");
  });
}

void LocalWebServer::update(uint32_t nowMs) {
  if (!_running) {
    return;
  }
  _server.handleClient();
  _ota.update(nowMs);
}

void LocalWebServer::setData(const LocalWebData& data) {
  _data = data;
  _data.ota = _ota.getStatus();
}

void LocalWebServer::sendStatusJson() {
  _data.ota = _ota.getStatus();
  if (!LocalWebJsonBuilder::buildStatus(_data, _jsonBuffer,
                                        sizeof(_jsonBuffer))) {
    _server.send(500, "application/json",
                 "{\"error\":\"serialization_failed\"}");
    return;
  }
  _server.send(200, "application/json", _jsonBuffer);
}

void LocalWebServer::sendBasicTelemetryJson() {
  if (!LocalWebJsonBuilder::buildBasicTelemetry(
          _data, _jsonBuffer, sizeof(_jsonBuffer))) {
    _server.send(500, "application/json",
                 "{\"error\":\"serialization_failed\"}");
    return;
  }
  _server.send(200, "application/json", _jsonBuffer);
}

bool LocalWebServer::isRunning() const {
  return _running;
}

bool LocalWebServer::otaInProgress() const {
  return _ota.getStatus().inProgress;
}

const OtaStatus& LocalWebServer::getOtaStatus() const {
  return _ota.getStatus();
}

String LocalWebServer::localIp() const {
  return _running ? WiFi.softAPIP().toString() : String("0.0.0.0");
}
