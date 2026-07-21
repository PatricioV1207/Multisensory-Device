#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

class DeviceIdentity {
 public:
  bool begin();
  bool isValid() const;
  uint32_t bootId() const;
  uint32_t nextSequence();
  bool formatSampleId(uint32_t sequence, char* output, size_t outputSize) const;

 private:
  uint32_t _bootId = 0;
  uint32_t _sequence = 0;
  bool _valid = false;
};
