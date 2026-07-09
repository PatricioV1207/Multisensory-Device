#pragma once

#include <Adafruit_BMP085_U.h>
#include <Arduino.h>
#include "telemetry/TelemetryData.h"

class BMP180Barometer {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  const BarometerData& getData() const;

 private:
  bool probe() const;

  Adafruit_BMP085_Unified _sensor{18001};
  BarometerData _data;
  bool _started = false;
  uint32_t _lastReadMs = 0;
  uint32_t _lastBeginAttemptMs = 0;
};

