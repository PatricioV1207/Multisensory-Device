#pragma once

#include "telemetry/TelemetryData.h"

struct StoredBarometerCalibration {
  float seaLevelPressureHpa;
  BarometerCalibrationSource source;
  bool persisted;
};

class BarometerCalibrationStore {
 public:
  static StoredBarometerCalibration load();
  static bool save(float seaLevelPressureHpa,
                   BarometerCalibrationSource source);
  static bool clear();
  static const char* sourceName(BarometerCalibrationSource source);
};
