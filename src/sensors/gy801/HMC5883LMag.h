#pragma once

#include <Adafruit_HMC5883_U.h>
#include <Arduino.h>
#include "telemetry/TelemetryData.h"

class HMC5883LMag {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  const MagData& getData() const;

 private:
  bool probe() const;
  bool possibleQmcClone() const;

  Adafruit_HMC5883_Unified _sensor{588301};
  MagData _data;
  bool _started = false;
  uint32_t _lastReadMs = 0;
  uint32_t _lastBeginAttemptMs = 0;
};

