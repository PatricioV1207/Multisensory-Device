#pragma once

#include <Arduino.h>
#include <BH1750.h>
#include "telemetry/TelemetryData.h"

class BH1750Sensor {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  const LightData& getData() const;

 private:
  bool probe(uint8_t address) const;

  BH1750 _sensor;
  LightData _data;
  uint8_t _address = 0;
  bool _started = false;
  uint32_t _lastReadMs = 0;
  uint32_t _lastBeginAttemptMs = 0;
};
