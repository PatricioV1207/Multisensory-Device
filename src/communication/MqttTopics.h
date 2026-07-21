#pragma once

#include <cstddef>

class MqttTopics {
 public:
  static constexpr size_t TOPIC_SIZE = 256U;

  bool build(const char* vehicleId, const char* deviceId);
  bool isValid() const;

  const char* telemetry() const;
  const char* status() const;
  const char* events() const;
  const char* acoustic() const;
  const char* commands() const;
  const char* commandAcks() const;

  static bool isValidId(const char* value);

 private:
  bool buildOne(char* output, size_t outputSize, const char* vehicleId,
                const char* deviceId, const char* suffix);

  bool _valid = false;
  char _telemetry[TOPIC_SIZE] = {0};
  char _status[TOPIC_SIZE] = {0};
  char _events[TOPIC_SIZE] = {0};
  char _acoustic[TOPIC_SIZE] = {0};
  char _commands[TOPIC_SIZE] = {0};
  char _commandAcks[TOPIC_SIZE] = {0};
};
