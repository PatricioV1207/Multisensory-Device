#pragma once

#include <stdint.h>

namespace TelemetrySchema {
static constexpr uint8_t VERSION = 1;
static constexpr const char* DEVICE_ID = "device_id";
static constexpr const char* UPTIME_MS = "uptime_ms";
static constexpr const char* SEA_LEVEL_PRESSURE_HPA =
    "sea_level_pressure_hpa";
}  // namespace TelemetrySchema
