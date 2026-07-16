#pragma once

#include <Arduino.h>
#include "sensors/GPSNeo6M.h"
#include "sensors/gy801/BMP180Barometer.h"

class BarometerCalibrationRunner {
 public:
  void begin();
  void update(uint32_t nowMs);

 private:
  static constexpr size_t kMaxSamples = 64;

  static float median(const float* values, size_t count);
  bool gpsMeetsQualityRequirements(const GPSData& gps) const;
  void collectSample(uint32_t nowMs);
  void finishSampling();
  void handleSerialCommands();
  void saveCandidate(float candidateHpa,
                     BarometerCalibrationSource source);
  void printProgress(uint32_t nowMs);

  GPSNeo6M _gps;
  BMP180Barometer _barometer;
  float _pressureSamples[kMaxSamples] = {0.0F};
  float _gpsAltitudeSamples[kMaxSamples] = {0.0F};
  size_t _pressureSampleCount = 0;
  size_t _gpsSampleCount = 0;
  float _gpsAltitudeMinimumM = NAN;
  float _gpsAltitudeMaximumM = NAN;
  float _knownAltitudeCandidateHpa = NAN;
  float _gpsCandidateHpa = NAN;
  uint32_t _samplingStartedMs = 0;
  uint32_t _lastBarometerSampleMs = 0;
  uint32_t _lastProgressMs = 0;
  bool _resultsReady = false;
  bool _gpsCandidateValid = false;
};
