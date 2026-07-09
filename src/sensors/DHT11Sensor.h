#pragma once

#include <Arduino.h>
#include <DHT.h>
#include "pins.h"
#include "telemetry/TelemetryData.h"

class DHT11Sensor {
 public:
  bool begin();
  void update(uint32_t nowMs);
  bool isValid() const;
  const DHT11Data& getData() const;

 private:
  DHT _sensor{PIN_DHT11_DATA, DHT11};
  DHT11Data _data;
  bool _started = false;
  uint32_t _lastReadMs = 0;
  uint32_t _lastWarningMs = 0;
};
