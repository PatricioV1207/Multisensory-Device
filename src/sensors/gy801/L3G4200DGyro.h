#pragma once

#include <Arduino.h>
#include <L3G.h>
#include "telemetry/TelemetryData.h"

class L3G4200DGyro {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  uint8_t address() const;
  const GyroData& getData() const;

 private:
  bool probe(uint8_t address) const;

  L3G _sensor;
  GyroData _data;
  bool _started = false;
  uint8_t _address = 0;
  uint32_t _lastReadMs = 0;
  uint32_t _lastBeginAttemptMs = 0;
};

