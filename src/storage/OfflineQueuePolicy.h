#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace OfflineQueuePolicy {

constexpr uint32_t REPLAY_TOKEN_MASK = 0x80000000UL;
constexpr uint32_t RECORD_ID_MASK = 0x7FFFFFFFUL;

inline uint32_t replayToken(uint32_t recordId) {
  return REPLAY_TOKEN_MASK | (recordId & RECORD_ID_MASK);
}

inline uint32_t liveToken(uint32_t sequence) {
  return sequence & RECORD_ID_MASK;
}

inline bool isReplayToken(uint32_t token) {
  return (token & REPLAY_TOKEN_MASK) != 0U;
}

inline uint32_t recordIdFromToken(uint32_t token) {
  return token & RECORD_ID_MASK;
}

inline bool shouldMarkReplayed(bool deferred, uint32_t attempts,
                               uint32_t recordBootId,
                               uint32_t currentBootId) {
  return deferred || attempts > 0U || recordBootId != currentBootId;
}

inline bool exceedsLimits(uint32_t queued, uint64_t bytes,
                          size_t nextRecordBytes, uint32_t maxRecords,
                          uint64_t maxBytes) {
  return queued >= maxRecords || nextRecordBytes > maxBytes ||
         bytes > maxBytes - nextRecordBytes;
}

inline uint32_t ageSeconds(uint64_t enqueuedEpochSeconds,
                           uint64_t nowEpochSeconds) {
  if (enqueuedEpochSeconds == 0U || nowEpochSeconds == 0U ||
      nowEpochSeconds <= enqueuedEpochSeconds) {
    return 0U;
  }
  const uint64_t age = nowEpochSeconds - enqueuedEpochSeconds;
  return age > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(age);
}

inline bool markPayloadReplayed(char* payload, size_t& length) {
  constexpr const char* kFalse = "\"replayed\":false";
  constexpr const char* kTrue = "\"replayed\":true";
  if (payload == nullptr || length == 0U) {
    return false;
  }
  char* position = std::strstr(payload, kFalse);
  if (position == nullptr) {
    return false;
  }
  const size_t prefix = static_cast<size_t>(position - payload);
  const size_t falseLength = std::strlen(kFalse);
  const size_t trueLength = std::strlen(kTrue);
  std::memcpy(position, kTrue, trueLength);
  std::memmove(payload + prefix + trueLength,
               payload + prefix + falseLength,
               length - prefix - falseLength + 1U);
  length -= falseLength - trueLength;
  return true;
}

}  // namespace OfflineQueuePolicy
