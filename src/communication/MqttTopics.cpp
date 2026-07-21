#include "communication/MqttTopics.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace {
constexpr const char* kPrefix = "vehiclesense/v1/vehicles";
constexpr size_t kMaxIdLength = 64U;
}

bool MqttTopics::isValidId(const char* value) {
  if (value == nullptr) {
    return false;
  }
  const size_t length = strlen(value);
  if (length == 0U || length > kMaxIdLength) {
    return false;
  }
  for (size_t index = 0; index < length; ++index) {
    const unsigned char character =
        static_cast<unsigned char>(value[index]);
    if (!(std::isalnum(character) || character == '.' || character == '_' ||
          character == '-')) {
      return false;
    }
  }
  return true;
}

bool MqttTopics::buildOne(char* output, size_t outputSize,
                          const char* vehicleId, const char* deviceId,
                          const char* suffix) {
  const int count = snprintf(output, outputSize, "%s/%s/devices/%s/%s", kPrefix,
                             vehicleId, deviceId, suffix);
  return count > 0 && static_cast<size_t>(count) < outputSize;
}

bool MqttTopics::build(const char* vehicleId, const char* deviceId) {
  _valid = false;
  if (!isValidId(vehicleId) || !isValidId(deviceId)) {
    return false;
  }
  _valid = buildOne(_telemetry, sizeof(_telemetry), vehicleId, deviceId,
                    "telemetry") &&
           buildOne(_status, sizeof(_status), vehicleId, deviceId, "status") &&
           buildOne(_events, sizeof(_events), vehicleId, deviceId, "events") &&
           buildOne(_acoustic, sizeof(_acoustic), vehicleId, deviceId,
                    "acoustic") &&
           buildOne(_commands, sizeof(_commands), vehicleId, deviceId,
                    "commands") &&
           buildOne(_commandAcks, sizeof(_commandAcks), vehicleId, deviceId,
                    "command-acks");
  return _valid;
}

bool MqttTopics::isValid() const {
  return _valid;
}

const char* MqttTopics::telemetry() const {
  return _telemetry;
}

const char* MqttTopics::status() const {
  return _status;
}

const char* MqttTopics::events() const {
  return _events;
}

const char* MqttTopics::acoustic() const {
  return _acoustic;
}

const char* MqttTopics::commands() const {
  return _commands;
}

const char* MqttTopics::commandAcks() const {
  return _commandAcks;
}
