#include "system/DeviceIdentity.h"

#include <Preferences.h>
#include <cstdio>
#include "config.h"
#include "utils/Logger.h"

namespace {
constexpr const char* kNamespace = "vs_identity";
constexpr const char* kBootIdKey = "boot_id";
}

bool DeviceIdentity::begin() {
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    Logger::error("IDENTITY", "Could not open NVS namespace");
    return false;
  }

  uint32_t nextBootId = preferences.getUInt(kBootIdKey, 0U) + 1U;
  if (nextBootId == 0U) {
    nextBootId = 1U;
  }
  const bool stored =
      preferences.putUInt(kBootIdKey, nextBootId) == sizeof(uint32_t);
  preferences.end();
  if (!stored) {
    Logger::error("IDENTITY", "Could not persist boot_id; v3 disabled");
    return false;
  }

  _bootId = nextBootId;
  _sequence = 0U;
  _valid = true;
  Logger::info("IDENTITY", "device=" + String(DEVICE_ID) +
                               " vehicle=" + String(VEHICLE_ID) +
                               " boot_id=" + String(_bootId));
  return true;
}

bool DeviceIdentity::isValid() const {
  return _valid;
}

uint32_t DeviceIdentity::bootId() const {
  return _bootId;
}

uint32_t DeviceIdentity::nextSequence() {
  if (!_valid) {
    return 0U;
  }
  ++_sequence;
  if (_sequence == 0U) {
    _sequence = 1U;
  }
  return _sequence;
}

bool DeviceIdentity::formatSampleId(uint32_t sequence, char* output,
                                    size_t outputSize) const {
  if (!_valid || sequence == 0U || output == nullptr || outputSize == 0U) {
    return false;
  }
  const int count = snprintf(output, outputSize, "%s:%lu:%lu", DEVICE_ID,
                             static_cast<unsigned long>(_bootId),
                             static_cast<unsigned long>(sequence));
  return count > 0 && static_cast<size_t>(count) < outputSize;
}
