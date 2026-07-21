#include "storage/OfflineTelemetryQueue.h"

#include <FS.h>
#include <SD.h>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "storage/OfflineQueuePolicy.h"
#include "utils/Logger.h"

namespace {
constexpr const char* kQueueDirectory = "/spool";
constexpr const char* kTemporaryPath = "/spool/tmp.msg";
constexpr uint32_t kRecordMagic = 0x56535131UL;  // VSQ1
constexpr uint16_t kRecordVersion = 1U;
constexpr uint16_t kFlagDeferred = 1U << 0;
constexpr uint16_t kFlagLastPublishReplayed = 1U << 1;

#pragma pack(push, 1)
struct QueueRecordHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t flags;
  uint32_t recordId;
  uint32_t bootId;
  uint32_t attempts;
  uint32_t enqueuedUptimeMs;
  uint64_t enqueuedEpochSeconds;
  uint32_t payloadLength;
  uint32_t checksum;
};
#pragma pack(pop)

static_assert(sizeof(QueueRecordHeader) == 40U,
              "Queue record header layout changed");

uint32_t checksum(const char* data, size_t length) {
  uint32_t value = 2166136261UL;
  for (size_t index = 0; index < length; ++index) {
    value ^= static_cast<uint8_t>(data[index]);
    value *= 16777619UL;
  }
  return value;
}

void recordPath(uint32_t recordId, char* output, size_t outputSize) {
  snprintf(output, outputSize, "%s/q_%010lu.msg", kQueueDirectory,
           static_cast<unsigned long>(recordId));
}

bool parseRecordId(const char* path, uint32_t& recordId) {
  if (path == nullptr) {
    return false;
  }
  const char* name = strrchr(path, '/');
  name = name == nullptr ? path : name + 1;
  unsigned long parsed = 0;
  char trailing = '\0';
  if (sscanf(name, "q_%lu.msg%c", &parsed, &trailing) != 1 || parsed == 0U ||
      parsed > OfflineQueuePolicy::RECORD_ID_MASK) {
    return false;
  }
  recordId = static_cast<uint32_t>(parsed);
  return true;
}

}  // namespace

bool OfflineTelemetryQueue::begin(uint32_t currentBootId) {
  _lastBeginAttemptMs = millis();
  _currentBootId = currentBootId;
  _status.ready = false;
  if (currentBootId == 0U || SD.cardType() == CARD_NONE) {
    return false;
  }
  if (!SD.exists(kQueueDirectory) && !SD.mkdir(kQueueDirectory)) {
    Logger::warn("QUEUE", "Could not create /spool directory");
    return false;
  }
  if (SD.exists(kTemporaryPath)) {
    SD.remove(kTemporaryPath);
  }

  _nextRecordId = 1U;
  if (!scanRecords()) {
    return false;
  }
  _status.ready = true;
  Logger::info("QUEUE", "Offline spool ready queued=" +
                            String(_status.queued) + " bytes=" +
                            String(static_cast<unsigned long>(_status.bytes)));
  return true;
}

void OfflineTelemetryQueue::update(uint32_t nowMs,
                                   uint64_t nowEpochSeconds,
                                   bool storageReady) {
  if (!storageReady) {
    setUnavailable(nowMs);
    return;
  }
  if (!_status.ready) {
    if (static_cast<uint32_t>(nowMs - _lastBeginAttemptMs) >=
        SD_RETRY_INTERVAL_MS) {
      begin(_currentBootId);
    }
    return;
  }
  refreshOldestAge(nowEpochSeconds);
  if (static_cast<uint32_t>(nowMs - _lastMaintenanceMs) < 1000U) {
    return;
  }
  _lastMaintenanceMs = nowMs;
  while (_status.queued > 0U && _status.oldestAgeSeconds >
                                    OFFLINE_QUEUE_MAX_AGE_SECONDS) {
    Logger::warn("QUEUE", "Dropping expired offline record");
    if (!removeOldest(true)) {
      setUnavailable(nowMs);
      return;
    }
    refreshOldestAge(nowEpochSeconds);
  }
}

bool OfflineTelemetryQueue::scanRecords() {
  _status.queued = 0U;
  _status.bytes = 0U;
  _oldestRecordId = 0U;
  uint32_t largestRecordId = 0U;
  File directory = SD.open(kQueueDirectory);
  if (!directory || !directory.isDirectory()) {
    return false;
  }
  File file = directory.openNextFile();
  while (file) {
    const String path = file.name();
    const size_t fileSize = file.size();
    const bool isDirectory = file.isDirectory();
    file.close();
    uint32_t recordId = 0U;
    if (!isDirectory && parseRecordId(path.c_str(), recordId)) {
      QueueRecordHeader header = {};
      size_t verifiedSize = 0U;
      if (readHeader(recordId, &header, sizeof(header), &verifiedSize) &&
          header.magic == kRecordMagic && header.version == kRecordVersion &&
          header.recordId == recordId && header.payloadLength > 0U &&
          header.payloadLength < TELEMETRY_PAYLOAD_BUFFER_SIZE &&
          verifiedSize == sizeof(header) + header.payloadLength) {
        ++_status.queued;
        _status.bytes += fileSize;
        if (_oldestRecordId == 0U || recordId < _oldestRecordId) {
          _oldestRecordId = recordId;
        }
        if (recordId > largestRecordId) {
          largestRecordId = recordId;
        }
      } else {
        char invalidPath[48];
        recordPath(recordId, invalidPath, sizeof(invalidPath));
        SD.remove(invalidPath);
        ++_status.dropped;
      }
    }
    file = directory.openNextFile();
  }
  directory.close();
  _nextRecordId = largestRecordId == 0U ? 1U : largestRecordId + 1U;
  return _nextRecordId <= OfflineQueuePolicy::RECORD_ID_MASK;
}

bool OfflineTelemetryQueue::enqueue(const char* payload, size_t length,
                                    uint32_t nowMs,
                                    uint64_t nowEpochSeconds,
                                    bool deferred) {
  if (!_status.ready || payload == nullptr || length == 0U ||
      length >= TELEMETRY_PAYLOAD_BUFFER_SIZE || _nextRecordId == 0U ||
      _nextRecordId > OfflineQueuePolicy::RECORD_ID_MASK) {
    return false;
  }
  const size_t recordBytes = sizeof(QueueRecordHeader) + length;
  if (!ensureCapacity(recordBytes)) {
    ++_status.dropped;
    return false;
  }

  QueueRecordHeader header = {};
  header.magic = kRecordMagic;
  header.version = kRecordVersion;
  header.flags = deferred ? kFlagDeferred : 0U;
  header.recordId = _nextRecordId;
  header.bootId = _currentBootId;
  header.enqueuedUptimeMs = nowMs;
  header.enqueuedEpochSeconds = nowEpochSeconds;
  header.payloadLength = static_cast<uint32_t>(length);
  header.checksum = checksum(payload, length);

  SD.remove(kTemporaryPath);
  File file = SD.open(kTemporaryPath, FILE_WRITE);
  if (!file) {
    setUnavailable(nowMs);
    return false;
  }
  const size_t headerWritten = file.write(
      reinterpret_cast<const uint8_t*>(&header), sizeof(header));
  const size_t payloadWritten = file.write(
      reinterpret_cast<const uint8_t*>(payload), length);
  file.flush();
  file.close();
  if (headerWritten != sizeof(header) || payloadWritten != length) {
    SD.remove(kTemporaryPath);
    setUnavailable(nowMs);
    return false;
  }

  char path[48];
  recordPath(header.recordId, path, sizeof(path));
  if (!SD.rename(kTemporaryPath, path)) {
    SD.remove(kTemporaryPath);
    setUnavailable(nowMs);
    return false;
  }
  if (_status.queued == 0U) {
    _oldestRecordId = header.recordId;
  }
  ++_status.queued;
  _status.bytes += recordBytes;
  ++_nextRecordId;
  return true;
}

bool OfflineTelemetryQueue::peek(char* output, size_t outputSize,
                                 size_t* written, uint32_t* recordId,
                                 bool* replayed) {
  if (written != nullptr) {
    *written = 0U;
  }
  if (recordId != nullptr) {
    *recordId = 0U;
  }
  if (replayed != nullptr) {
    *replayed = false;
  }
  if (!_status.ready || _status.queued == 0U || output == nullptr ||
      outputSize == 0U || _oldestRecordId == 0U) {
    return false;
  }

  char path[48];
  recordPath(_oldestRecordId, path, sizeof(path));
  File file = SD.open(path, FILE_READ);
  QueueRecordHeader header = {};
  if (!file || file.read(reinterpret_cast<uint8_t*>(&header),
                         sizeof(header)) != sizeof(header) ||
      header.magic != kRecordMagic || header.version != kRecordVersion ||
      header.recordId != _oldestRecordId || header.payloadLength == 0U ||
      header.payloadLength + 1U > outputSize) {
    if (file) {
      file.close();
    }
    removeOldest(true);
    return false;
  }
  const size_t length = header.payloadLength;
  const size_t bytesRead =
      file.read(reinterpret_cast<uint8_t*>(output), length);
  file.close();
  output[length] = '\0';
  if (bytesRead != length || checksum(output, length) != header.checksum) {
    removeOldest(true);
    return false;
  }

  size_t outgoingLength = length;
  const bool shouldReplay = OfflineQueuePolicy::shouldMarkReplayed(
      (header.flags & kFlagDeferred) != 0U, header.attempts, header.bootId,
      _currentBootId);
  if (shouldReplay &&
      !OfflineQueuePolicy::markPayloadReplayed(output, outgoingLength)) {
    removeOldest(true);
    return false;
  }
  if (written != nullptr) {
    *written = outgoingLength;
  }
  if (recordId != nullptr) {
    *recordId = header.recordId;
  }
  if (replayed != nullptr) {
    *replayed = shouldReplay;
  }
  return true;
}

bool OfflineTelemetryQueue::markAttempted(uint32_t recordId, bool replayed) {
  if (!_status.ready || recordId == 0U) {
    return false;
  }
  char path[48];
  recordPath(recordId, path, sizeof(path));
  File file = SD.open(path, "r+");
  QueueRecordHeader header = {};
  if (!file || file.read(reinterpret_cast<uint8_t*>(&header),
                         sizeof(header)) != sizeof(header) ||
      header.magic != kRecordMagic || header.recordId != recordId) {
    if (file) {
      file.close();
    }
    return false;
  }
  ++header.attempts;
  if (replayed) {
    header.flags |= kFlagLastPublishReplayed;
  } else {
    header.flags &= static_cast<uint16_t>(~kFlagLastPublishReplayed);
  }
  const bool seekOk = file.seek(0U);
  const size_t count =
      seekOk ? file.write(reinterpret_cast<const uint8_t*>(&header),
                          sizeof(header))
             : 0U;
  file.flush();
  file.close();
  return count == sizeof(header);
}

bool OfflineTelemetryQueue::acknowledge(uint32_t recordId) {
  if (!_status.ready || recordId == 0U || recordId != _oldestRecordId) {
    return false;
  }
  QueueRecordHeader header = {};
  if (!readHeader(recordId, &header, sizeof(header))) {
    return false;
  }
  const bool wasReplay =
      (header.flags & kFlagLastPublishReplayed) != 0U;
  if (!removeOldest(false)) {
    return false;
  }
  if (wasReplay) {
    ++_status.replayed;
  }
  return true;
}

bool OfflineTelemetryQueue::ensureCapacity(size_t nextRecordBytes) {
  while (_status.queued > 0U && OfflineQueuePolicy::exceedsLimits(
                                    _status.queued, _status.bytes,
                                    nextRecordBytes,
                                    OFFLINE_QUEUE_MAX_RECORDS,
                                    OFFLINE_QUEUE_MAX_BYTES)) {
    Logger::warn("QUEUE", "Spool limit reached; dropping oldest record");
    if (!removeOldest(true)) {
      return false;
    }
  }
  return !OfflineQueuePolicy::exceedsLimits(
      _status.queued, _status.bytes, nextRecordBytes,
      OFFLINE_QUEUE_MAX_RECORDS, OFFLINE_QUEUE_MAX_BYTES);
}

bool OfflineTelemetryQueue::removeOldest(bool countAsDropped) {
  if (_oldestRecordId == 0U || _status.queued == 0U) {
    return false;
  }
  char path[48];
  recordPath(_oldestRecordId, path, sizeof(path));
  File file = SD.open(path, FILE_READ);
  const size_t fileSize = file ? file.size() : 0U;
  if (file) {
    file.close();
  }
  if (!SD.remove(path)) {
    return false;
  }
  --_status.queued;
  _status.bytes = fileSize <= _status.bytes ? _status.bytes - fileSize : 0U;
  if (countAsDropped) {
    ++_status.dropped;
  }
  if (_status.queued == 0U) {
    _oldestRecordId = 0U;
    _nextRecordId = 1U;
    _status.oldestAgeSeconds = 0U;
  } else if (!findOldest()) {
    return false;
  }
  return true;
}

bool OfflineTelemetryQueue::findOldest() {
  _oldestRecordId = 0U;
  File directory = SD.open(kQueueDirectory);
  if (!directory || !directory.isDirectory()) {
    return false;
  }
  File file = directory.openNextFile();
  while (file) {
    uint32_t recordId = 0U;
    if (!file.isDirectory() && parseRecordId(file.name(), recordId) &&
        (_oldestRecordId == 0U || recordId < _oldestRecordId)) {
      _oldestRecordId = recordId;
    }
    file.close();
    file = directory.openNextFile();
  }
  directory.close();
  return _oldestRecordId != 0U;
}

bool OfflineTelemetryQueue::readHeader(uint32_t recordId, void* header,
                                       size_t headerSize,
                                       size_t* fileSize) const {
  char path[48];
  recordPath(recordId, path, sizeof(path));
  File file = SD.open(path, FILE_READ);
  if (!file || header == nullptr || headerSize != sizeof(QueueRecordHeader)) {
    if (file) {
      file.close();
    }
    return false;
  }
  if (fileSize != nullptr) {
    *fileSize = file.size();
  }
  const size_t count =
      file.read(reinterpret_cast<uint8_t*>(header), headerSize);
  file.close();
  return count == headerSize;
}

void OfflineTelemetryQueue::refreshOldestAge(uint64_t nowEpochSeconds) {
  if (_oldestRecordId == 0U) {
    _status.oldestAgeSeconds = 0U;
    return;
  }
  QueueRecordHeader header = {};
  if (!readHeader(_oldestRecordId, &header, sizeof(header))) {
    _status.oldestAgeSeconds = 0U;
    return;
  }
  _status.oldestAgeSeconds = OfflineQueuePolicy::ageSeconds(
      header.enqueuedEpochSeconds, nowEpochSeconds);
}

void OfflineTelemetryQueue::setUnavailable(uint32_t nowMs) {
  _status.ready = false;
  _lastBeginAttemptMs = nowMs;
}

bool OfflineTelemetryQueue::isReady() const {
  return _status.ready;
}

bool OfflineTelemetryQueue::hasPending() const {
  return _status.ready && _status.queued > 0U;
}

const OfflineDeliveryStatus& OfflineTelemetryQueue::getStatus() const {
  return _status;
}
