#include "system/TimeService.h"

#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>
#include <cstdio>
#include "config.h"
#include "utils/Logger.h"

namespace {
constexpr time_t kMinimumValidEpoch = 1704067200;  // 2024-01-01 UTC.
constexpr time_t kMaximumValidEpoch = 4102444800;  // 2100-01-01 UTC.
}

void TimeService::begin() {
  setenv("TZ", "UTC0", 1);
  tzset();
  _source = systemClockValid() ? TimeSource::NTP : TimeSource::None;
}

void TimeService::update(uint32_t nowMs, bool wifiConnected,
                         const GPSData& gps) {
  if (wifiConnected && !systemClockValid() &&
      (!_ntpStarted || static_cast<uint32_t>(nowMs - _lastNtpStartMs) >=
                           NTP_SYNC_RETRY_INTERVAL_MS)) {
    startNtp(nowMs);
  }

  if (_ntpStarted && sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED &&
      systemClockValid()) {
    if (_source != TimeSource::NTP) {
      Logger::info("TIME", "UTC synchronized from NTP");
    }
    _source = TimeSource::NTP;
  }

  if (!systemClockValid() && gps.utcValid && synchronizeFromGps(gps)) {
    _source = TimeSource::GPS;
    Logger::info("TIME", "UTC synchronized from GPS fallback");
  }
}

void TimeService::startNtp(uint32_t nowMs) {
  configTime(0, 0, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);
  _ntpStarted = true;
  _lastNtpStartMs = nowMs;
  Logger::info("TIME", "NTP synchronization requested");
}

bool TimeService::synchronizeFromGps(const GPSData& gps) {
  struct tm utc = {};
  utc.tm_year = static_cast<int>(gps.utcYear) - 1900;
  utc.tm_mon = static_cast<int>(gps.utcMonth) - 1;
  utc.tm_mday = gps.utcDay;
  utc.tm_hour = gps.utcHour;
  utc.tm_min = gps.utcMinute;
  utc.tm_sec = gps.utcSecond;
  utc.tm_isdst = 0;
  const time_t epoch = mktime(&utc);
  if (epoch < kMinimumValidEpoch || epoch >= kMaximumValidEpoch) {
    return false;
  }
  const struct timeval value = {epoch, 0};
  return settimeofday(&value, nullptr) == 0;
}

bool TimeService::systemClockValid() const {
  const time_t now = time(nullptr);
  return now >= kMinimumValidEpoch && now < kMaximumValidEpoch;
}

bool TimeService::isValid() const {
  return systemClockValid();
}

TimeSource TimeService::source() const {
  return isValid() ? _source : TimeSource::None;
}

const char* TimeService::sourceName() const {
  switch (source()) {
    case TimeSource::GPS:
      return "gps";
    case TimeSource::NTP:
      return "ntp";
    case TimeSource::None:
    default:
      return "none";
  }
}

bool TimeService::formatIso8601(char* output, size_t outputSize) const {
  if (output == nullptr || outputSize < 21U || !isValid()) {
    if (output != nullptr && outputSize > 0U) {
      output[0] = '\0';
    }
    return false;
  }
  const time_t now = time(nullptr);
  struct tm utc = {};
  if (gmtime_r(&now, &utc) == nullptr) {
    output[0] = '\0';
    return false;
  }
  const size_t count =
      strftime(output, outputSize, "%Y-%m-%dT%H:%M:%SZ", &utc);
  return count > 0U;
}
