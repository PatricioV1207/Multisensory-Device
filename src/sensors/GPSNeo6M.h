#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "telemetry/TelemetryData.h"

class GPSNeo6M {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  const GPSData& getData() const;

 private:
  HardwareSerial _serial{2};
  TinyGPSPlus _gps;
  GPSData _data;
  bool _started = false;
  uint32_t _startedAtMs = 0;
  uint32_t _lastByteMs = 0;
  uint32_t _lastWarningMs = 0;
};

