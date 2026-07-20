#include "web/LocalOTAUpdater.h"

#include <Update.h>
#include <cstring>
#include "utils/Logger.h"

namespace {
bool hasRealValue(const char* value) {
  return value != nullptr && value[0] != '\0' &&
         strncmp(value, "YOUR_", 5) != 0;
}
}

bool LocalOTAUpdater::begin(WebServer& server, const char* username,
                            const char* password) {
  _server = &server;
  _username = username;
  _password = password;
  _configured = hasRealValue(username) && hasRealValue(password);
  _status.enabled = _configured;
  if (!_configured) {
    Logger::warn("OTA", "Administrator credentials missing; OTA disabled");
    return false;
  }
  _server->on("/admin/update", HTTP_POST,
              [this]() { finishRequest(); },
              [this]() { handleUpload(); });
  return true;
}

bool LocalOTAUpdater::authenticate(bool sendChallenge) {
  if (!_configured || _server == nullptr) {
    if (_server != nullptr && sendChallenge) {
      _server->send(503, "application/json", "{\"error\":\"ota_disabled\"}");
    }
    return false;
  }
  if (_server->authenticate(_username, _password)) {
    return true;
  }
  if (sendChallenge) {
    _server->requestAuthentication(BASIC_AUTH, "Bus IoT Administrator");
  }
  return false;
}

void LocalOTAUpdater::handleUpload() {
  if (_server == nullptr) {
    return;
  }
  HTTPUpload& upload = _server->upload();
  if (upload.status == UPLOAD_FILE_START) {
    _uploadAuthorized = authenticate(false);
    _uploadError = !_uploadAuthorized || !upload.filename.endsWith(".bin");
    _uploadFinishedOk = false;
    _status.inProgress = _uploadAuthorized && !_uploadError;
    _status.lastUpdateOk = false;
    _status.lastActivityMs = millis();
    if (!_uploadError && !Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      _uploadError = true;
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    _status.lastActivityMs = millis();
    _uploadAuthorized = _uploadAuthorized && authenticate(false);
    if (!_uploadAuthorized || _uploadError) {
      _uploadError = true;
      _status.inProgress = false;
      if (Update.isRunning()) {
        Update.abort();
      }
      return;
    }
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      _uploadError = true;
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    _status.lastActivityMs = millis();
    _uploadAuthorized = _uploadAuthorized && authenticate(false);
    if (_uploadAuthorized && !_uploadError) {
      _uploadFinishedOk = Update.end(true);
      _uploadError = !_uploadFinishedOk;
      if (_uploadError) {
        Update.printError(Serial);
      }
    } else if (Update.isRunning()) {
      Update.abort();
    }
    _status.inProgress = false;
    _status.lastUpdateOk = _uploadFinishedOk;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (Update.isRunning()) {
      Update.abort();
    }
    _uploadError = true;
    _uploadFinishedOk = false;
    _status.inProgress = false;
    _status.lastUpdateOk = false;
    _status.lastActivityMs = millis();
  }
}

void LocalOTAUpdater::finishRequest() {
  if (_server == nullptr || !authenticate()) {
    return;
  }
  if (_uploadFinishedOk && !_uploadError) {
    _server->send(200, "application/json",
                  "{\"ok\":true,\"restarting\":true}");
    _restartPending = true;
    _restartAtMs = millis() + 750UL;
    Logger::info("OTA", "Firmware accepted; restart scheduled");
  } else {
    _server->send(400, "application/json",
                  "{\"ok\":false,\"error\":\"update_failed\"}");
    Logger::warn("OTA", "Firmware update rejected or incomplete");
  }
}

void LocalOTAUpdater::update(uint32_t nowMs) {
  if (_restartPending &&
      static_cast<int32_t>(nowMs - _restartAtMs) >= 0) {
    ESP.restart();
  }
}

bool LocalOTAUpdater::isConfigured() const {
  return _configured;
}

const OtaStatus& LocalOTAUpdater::getStatus() const {
  return _status;
}
