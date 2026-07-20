#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "system/RuntimeStatus.h"

class LocalOTAUpdater {
 public:
  bool begin(WebServer& server, const char* username, const char* password);
  void update(uint32_t nowMs);
  bool authenticate(bool sendChallenge = true);
  bool isConfigured() const;
  const OtaStatus& getStatus() const;

 private:
  void handleUpload();
  void finishRequest();

  WebServer* _server = nullptr;
  const char* _username = "";
  const char* _password = "";
  OtaStatus _status;
  bool _configured = false;
  bool _uploadAuthorized = false;
  bool _uploadError = false;
  bool _uploadFinishedOk = false;
  bool _restartPending = false;
  uint32_t _restartAtMs = 0;
};
