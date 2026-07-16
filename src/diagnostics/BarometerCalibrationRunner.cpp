#include "diagnostics/BarometerCalibrationRunner.h"

#include <cmath>
#include "calibration/BarometerCalibrationStore.h"
#include "calibration/BarometerMath.h"
#include "config.h"
#include "utils/Logger.h"

static_assert(BARO_CALIBRATION_MIN_SAMPLES <= 64U,
              "BARO_CALIBRATION_MIN_SAMPLES exceeds calibration buffer");

void BarometerCalibrationRunner::begin() {
  const StoredBarometerCalibration calibration =
      BarometerCalibrationStore::load();
  _barometer.setCalibration(calibration.seaLevelPressureHpa,
                            BARO_PRESSURE_OFFSET_HPA, calibration.source);
  const bool barometerStarted = _barometer.begin();
  const bool gpsStarted = _gps.begin();

  Logger::info("BARO_CAL", "Calibration environment started");
  Serial.printf("[BARO_CAL] barometer=%d gps=%d current_p0_hpa=%.2f "
                "source=%s\n",
                barometerStarted, gpsStarted,
                calibration.seaLevelPressureHpa,
                BarometerCalibrationStore::sourceName(calibration.source));
  Serial.printf("[BARO_CAL] known_altitude_m=%.2f duration_s=%lu "
                "minimum_samples=%u\n",
                BARO_CALIBRATION_KNOWN_ALTITUDE_M,
                static_cast<unsigned long>(BARO_CALIBRATION_DURATION_MS / 1000UL),
                static_cast<unsigned>(BARO_CALIBRATION_MIN_SAMPLES));
  Serial.println("[BARO_CAL] Keep the device stationary. No value is saved "
                 "automatically.");
  Serial.println("[BARO_CAL] Commands after results: k=known altitude, "
                 "g=GPS, r=reset NVS");
}

void BarometerCalibrationRunner::update(uint32_t nowMs) {
  _gps.update(nowMs);
  _barometer.update(nowMs);
  handleSerialCommands();

  if (!_resultsReady) {
    const BarometerData& barometer = _barometer.getData();
    if (barometer.valid && barometer.updatedAtMs != 0 &&
        barometer.updatedAtMs != _lastBarometerSampleMs) {
      _lastBarometerSampleMs = barometer.updatedAtMs;
      collectSample(nowMs);
    }

    if (_samplingStartedMs != 0 &&
        static_cast<uint32_t>(nowMs - _samplingStartedMs) >=
            BARO_CALIBRATION_DURATION_MS &&
        _pressureSampleCount >= BARO_CALIBRATION_MIN_SAMPLES) {
      finishSampling();
    }
  }

  printProgress(nowMs);
}

float BarometerCalibrationRunner::median(const float* values, size_t count) {
  if (values == nullptr || count == 0 || count > kMaxSamples) {
    return NAN;
  }
  float sorted[kMaxSamples];
  for (size_t i = 0; i < count; ++i) {
    sorted[i] = values[i];
  }
  for (size_t i = 1; i < count; ++i) {
    const float value = sorted[i];
    size_t position = i;
    while (position > 0 && sorted[position - 1] > value) {
      sorted[position] = sorted[position - 1];
      --position;
    }
    sorted[position] = value;
  }
  if ((count % 2U) == 0U) {
    return (sorted[(count / 2U) - 1U] + sorted[count / 2U]) * 0.5F;
  }
  return sorted[count / 2U];
}

bool BarometerCalibrationRunner::gpsMeetsQualityRequirements(
    const GPSData& gps) const {
  return gps.valid && gps.satellites >= BARO_CALIBRATION_MIN_SATELLITES &&
         std::isfinite(gps.hdop) && gps.hdop > 0.0F &&
         gps.hdop <= BARO_CALIBRATION_MAX_HDOP &&
         std::isfinite(gps.speedKmh) &&
         gps.speedKmh <= BARO_CALIBRATION_MAX_SPEED_KMH &&
         std::isfinite(gps.altitudeM);
}

void BarometerCalibrationRunner::collectSample(uint32_t nowMs) {
  const BarometerData& barometer = _barometer.getData();
  if (_pressureSampleCount >= kMaxSamples ||
      !std::isfinite(barometer.pressureHpa)) {
    return;
  }
  if (_samplingStartedMs == 0) {
    _samplingStartedMs = nowMs;
  }
  _pressureSamples[_pressureSampleCount++] = barometer.pressureHpa;

  const GPSData& gps = _gps.getData();
  if (!gpsMeetsQualityRequirements(gps) ||
      _gpsSampleCount >= kMaxSamples) {
    return;
  }
  _gpsAltitudeSamples[_gpsSampleCount++] = gps.altitudeM;
  if (!std::isfinite(_gpsAltitudeMinimumM) ||
      gps.altitudeM < _gpsAltitudeMinimumM) {
    _gpsAltitudeMinimumM = gps.altitudeM;
  }
  if (!std::isfinite(_gpsAltitudeMaximumM) ||
      gps.altitudeM > _gpsAltitudeMaximumM) {
    _gpsAltitudeMaximumM = gps.altitudeM;
  }
}

void BarometerCalibrationRunner::finishSampling() {
  const float medianPressure =
      median(_pressureSamples, _pressureSampleCount);
  _knownAltitudeCandidateHpa =
      BarometerMath::seaLevelPressureFromAltitude(
          BARO_CALIBRATION_KNOWN_ALTITUDE_M, medianPressure);

  const float gpsSpread = _gpsAltitudeMaximumM - _gpsAltitudeMinimumM;
  const float medianGpsAltitude =
      median(_gpsAltitudeSamples, _gpsSampleCount);
  _gpsCandidateValid =
      _gpsSampleCount >= BARO_CALIBRATION_MIN_SAMPLES &&
      std::isfinite(gpsSpread) &&
      gpsSpread <= BARO_CALIBRATION_MAX_ALTITUDE_SPREAD_M;
  if (_gpsCandidateValid) {
    _gpsCandidateHpa = BarometerMath::seaLevelPressureFromAltitude(
        medianGpsAltitude, medianPressure);
    _gpsCandidateValid =
        BarometerMath::isValidSeaLevelPressure(_gpsCandidateHpa);
  }

  _resultsReady = true;
  Serial.printf("[BARO_CAL] COMPLETE pressure_samples=%u gps_samples=%u "
                "median_pressure_local_hpa=%.3f\n",
                static_cast<unsigned>(_pressureSampleCount),
                static_cast<unsigned>(_gpsSampleCount), medianPressure);
  Serial.printf("[BARO_CAL] known_altitude_m=%.2f "
                "candidate_known_hpa=%.3f\n",
                BARO_CALIBRATION_KNOWN_ALTITUDE_M,
                _knownAltitudeCandidateHpa);
  if (_gpsCandidateValid) {
    Serial.printf("[BARO_CAL] gps_altitude_median_m=%.2f "
                  "gps_altitude_spread_m=%.2f candidate_gps_hpa=%.3f\n",
                  medianGpsAltitude, gpsSpread, _gpsCandidateHpa);
    Serial.printf("[BARO_CAL] candidate_difference_hpa=%.3f\n",
                  std::fabs(_knownAltitudeCandidateHpa - _gpsCandidateHpa));
  } else {
    Serial.printf("[BARO_CAL] GPS candidate unavailable: samples=%u/%u "
                  "altitude_spread_m=%.2f/%.2f\n",
                  static_cast<unsigned>(_gpsSampleCount),
                  static_cast<unsigned>(BARO_CALIBRATION_MIN_SAMPLES),
                  gpsSpread, BARO_CALIBRATION_MAX_ALTITUDE_SPREAD_M);
  }
  Serial.println("[BARO_CAL] Send k to save known-altitude candidate, "
                 "g to save GPS candidate, or r to clear NVS.");
}

void BarometerCalibrationRunner::handleSerialCommands() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == '\r' || command == '\n' || command == ' ' ||
        command == '\t') {
      continue;
    }
    if (command == 'k' || command == 'K') {
      if (!_resultsReady ||
          !BarometerMath::isValidSeaLevelPressure(
              _knownAltitudeCandidateHpa)) {
        Serial.println("[BARO_CAL] Known-altitude candidate is not ready.");
      } else {
        saveCandidate(_knownAltitudeCandidateHpa,
                      BarometerCalibrationSource::KnownAltitude);
      }
    } else if (command == 'g' || command == 'G') {
      if (!_resultsReady || !_gpsCandidateValid) {
        Serial.println("[BARO_CAL] GPS candidate is not ready or failed "
                       "quality checks.");
      } else {
        saveCandidate(_gpsCandidateHpa, BarometerCalibrationSource::GPS);
      }
    } else if (command == 'r' || command == 'R') {
      if (BarometerCalibrationStore::clear()) {
        _barometer.setCalibration(BARO_DEFAULT_SEA_LEVEL_PRESSURE_HPA,
                                  BARO_PRESSURE_OFFSET_HPA,
                                  BarometerCalibrationSource::Default);
        Serial.printf("[BARO_CAL] NVS cleared; fallback_p0_hpa=%.2f\n",
                      BARO_DEFAULT_SEA_LEVEL_PRESSURE_HPA);
      } else {
        Serial.println("[BARO_CAL] ERROR: could not clear NVS.");
      }
    } else {
      Serial.printf("[BARO_CAL] Unknown command '%c'. Use k, g or r.\n",
                    command);
    }
  }
}

void BarometerCalibrationRunner::saveCandidate(
    float candidateHpa, BarometerCalibrationSource source) {
  if (!BarometerCalibrationStore::save(candidateHpa, source)) {
    Serial.println("[BARO_CAL] ERROR: candidate was not saved to NVS.");
    return;
  }
  _barometer.setCalibration(candidateHpa, BARO_PRESSURE_OFFSET_HPA, source);
  Serial.printf("[BARO_CAL] SAVED p0_hpa=%.3f source=%s. "
                "Reflash test_bmp180 or full_prototype.\n",
                candidateHpa,
                BarometerCalibrationStore::sourceName(source));
}

void BarometerCalibrationRunner::printProgress(uint32_t nowMs) {
  if (_resultsReady ||
      static_cast<uint32_t>(nowMs - _lastProgressMs) < 5000UL) {
    return;
  }
  _lastProgressMs = nowMs;
  const GPSData& gps = _gps.getData();
  const uint32_t elapsedSeconds = _samplingStartedMs == 0
                                      ? 0
                                      : static_cast<uint32_t>(
                                            nowMs - _samplingStartedMs) /
                                            1000UL;
  Serial.printf("[BARO_CAL] progress_s=%lu pressure_samples=%u "
                "gps_samples=%u gps_valid=%d satellites=%lu hdop=%.2f "
                "speed_kmh=%.2f\n",
                static_cast<unsigned long>(elapsedSeconds),
                static_cast<unsigned>(_pressureSampleCount),
                static_cast<unsigned>(_gpsSampleCount), gps.valid,
                static_cast<unsigned long>(gps.satellites), gps.hdop,
                gps.speedKmh);
}
