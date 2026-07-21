#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "web/LocalOTAUpdater.h"
#include "web/LocalWebData.h"

class LocalWebServer {
 public:
  bool begin(bool preserveStation = false);
  void update(uint32_t nowMs);
  void setData(const LocalWebData& data);
  bool isRunning() const;
  bool otaInProgress() const;
  const OtaStatus& getOtaStatus() const;
  String localIp() const;

 private:
  void registerRoutes();
  void sendStatusJson();
  void sendBasicTelemetryJson();

  WebServer _server{80};
  LocalOTAUpdater _ota;
  LocalWebData _data;
  bool _running = false;
  char _jsonBuffer[LOCAL_WEB_JSON_BUFFER_SIZE] = {0};
};
