#include "storage/MicroSDLogger.h"

#include <Preferences.h>
#include <SD.h>
#include "config.h"
#include "pins.h"
#include "utils/Logger.h"

namespace {
constexpr const char* kLogDirectory = "/telemetry";
constexpr const char* kPreferencesNamespace = "sd_log";
constexpr const char* kBootCounterKey = "boot";
}

bool MicroSDLogger::initializeSession() {
  if (_sessionInitialized) {
    return true;
  }
  Preferences preferences;
  if (!preferences.begin(kPreferencesNamespace, false)) {
    return false;
  }
  uint32_t bootSession = preferences.getUInt(kBootCounterKey, 0) + 1U;
  if (bootSession == 0) {
    bootSession = 1;
  }
  const bool stored =
      preferences.putUInt(kBootCounterKey, bootSession) == sizeof(uint32_t);
  preferences.end();
  if (!stored) {
    return false;
  }
  _status.bootSession = bootSession;
  _status.segment = 0;
  _sessionInitialized = true;
  return true;
}

bool MicroSDLogger::begin() {
  _lastBeginAttemptMs = millis();
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  if (!_spiStarted) {
    _spi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    _spiStarted = true;
  }
  if (!initializeSession()) {
    Logger::warn("SD", "Could not initialize boot session in NVS");
    markUnavailable(_lastBeginAttemptMs);
    return false;
  }
  if (!SD.begin(PIN_SD_CS, _spi, SD_SPI_FREQUENCY_HZ) ||
      SD.cardType() == CARD_NONE) {
    Logger::warn("SD", "Card mount failed or no card present");
    markUnavailable(_lastBeginAttemptMs);
    return false;
  }
  if (!SD.exists(kLogDirectory) && !SD.mkdir(kLogDirectory)) {
    Logger::warn("SD", "Could not create /telemetry directory");
    markUnavailable(_lastBeginAttemptMs);
    return false;
  }
  _status.initialized = true;
  _status.mounted = true;
  _status.lastWriteOk = false;
  if (!openSegment()) {
    markUnavailable(_lastBeginAttemptMs);
    return false;
  }
  refreshCapacity();
  Logger::info("SD", "Logging to " + String(_path));
  return true;
}

void MicroSDLogger::update(uint32_t nowMs) {
  if (!_status.mounted &&
      static_cast<uint32_t>(nowMs - _lastBeginAttemptMs) >=
          SD_RETRY_INTERVAL_MS) {
    begin();
  }
}

bool MicroSDLogger::openSegment() {
  if (_file) {
    _file.close();
  }
  snprintf(_path, sizeof(_path), "%s/boot_%06lu_%03u.jsonl", kLogDirectory,
           static_cast<unsigned long>(_status.bootSession),
           static_cast<unsigned>(_status.segment));
  _file = SD.open(_path, FILE_APPEND);
  return static_cast<bool>(_file);
}

bool MicroSDLogger::rotateIfNeeded(size_t nextRecordBytes) {
  if (!_file) {
    return openSegment();
  }
  if (_file.size() + nextRecordBytes <= SD_LOG_ROTATE_BYTES) {
    return true;
  }
  if (_status.segment == UINT16_MAX) {
    return false;
  }
  ++_status.segment;
  return openSegment();
}

bool MicroSDLogger::appendJsonLine(const char* json, size_t length,
                                   uint32_t nowMs) {
  if (!isReady() || json == nullptr || length == 0 || json[0] == '\0') {
    return false;
  }
  if (!rotateIfNeeded(length + 1U)) {
    Logger::warn("SD", "Log rotation failed");
    markUnavailable(nowMs);
    return false;
  }
  const size_t written = _file.write(
      reinterpret_cast<const uint8_t*>(json), length);
  const size_t newlineWritten = _file.write(static_cast<uint8_t>('\n'));
  _file.flush();
  _status.lastWriteOk = written == length && newlineWritten == 1U;
  _status.lastWriteMs = nowMs;
  if (!_status.lastWriteOk) {
    Logger::warn("SD", "JSONL write failed; remount scheduled");
    markUnavailable(nowMs);
    return false;
  }
  refreshCapacity();
  return true;
}

void MicroSDLogger::markUnavailable(uint32_t nowMs) {
  if (_file) {
    _file.close();
  }
  SD.end();
  _status.initialized = false;
  _status.mounted = false;
  _status.lastWriteOk = false;
  _lastBeginAttemptMs = nowMs;
}

void MicroSDLogger::refreshCapacity() {
  _status.totalBytes = SD.totalBytes();
  _status.usedBytes = SD.usedBytes();
  _status.freeBytes = _status.totalBytes >= _status.usedBytes
                          ? _status.totalBytes - _status.usedBytes
                          : 0;
}

bool MicroSDLogger::isReady() const {
  return _status.mounted && static_cast<bool>(_file);
}

const StorageStatus& MicroSDLogger::getStatus() const {
  return _status;
}
