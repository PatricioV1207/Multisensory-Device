#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include "telemetry/TelemetryData.h"

enum class TimeSource : uint8_t {
  None = 0,
  GPS = 1,
  NTP = 2,
};

class TimeService {
 public:
  void begin();
  void update(uint32_t nowMs, bool wifiConnected, const GPSData& gps);
  bool isValid() const;
  TimeSource source() const;
  const char* sourceName() const;
  bool formatIso8601(char* output, size_t outputSize) const;

 private:
  bool systemClockValid() const;
  bool synchronizeFromGps(const GPSData& gps);
  void startNtp(uint32_t nowMs);

  bool _ntpStarted = false;
  uint32_t _lastNtpStartMs = 0;
  TimeSource _source = TimeSource::None;
};
