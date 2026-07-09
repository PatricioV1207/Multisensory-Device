#pragma once

#include <Adafruit_ADXL345_U.h>
#include <Arduino.h>
#include "telemetry/TelemetryData.h"

class ADXL345Accel {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  uint8_t address() const;
  const AccelData& getData() const;

 private:
  bool probe(uint8_t address) const;

  Adafruit_ADXL345_Unified _sensor{34501};
  AccelData _data;
  bool _started = false;
  uint8_t _address = 0;
  uint32_t _lastReadMs = 0;
  uint32_t _lastBeginAttemptMs = 0;
};

