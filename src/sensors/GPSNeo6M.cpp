#include "sensors/GPSNeo6M.h"

#include "config.h"
#include "pins.h"
#include "utils/Logger.h"

bool GPSNeo6M::begin() {
  _serial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  _started = true;
  _startedAtMs = millis();
  _data.valid = false;
  Logger::info("GPS", "NEO-6M UART2 started at " + String(GPS_BAUD) + " baud");
  return true;
}

void GPSNeo6M::update(uint32_t nowMs) {
  if (!_started) {
    return;
  }

  while (_serial.available() > 0) {
    const char value = static_cast<char>(_serial.read());
    _gps.encode(value);
    _lastByteMs = nowMs;
    _data.streamSeen = true;
    ++_data.charsProcessed;
  }

  if (_gps.satellites.isValid()) {
    _data.satellites = _gps.satellites.value();
  }
  if (_gps.hdop.isValid()) {
    _data.hdop = _gps.hdop.hdop();
  }

  const bool freshFix = _gps.location.isValid() && _gps.altitude.isValid() &&
                        _gps.speed.isValid() &&
                        _gps.location.age() <= GPS_FIX_MAX_AGE_MS &&
                        _gps.altitude.age() <= GPS_FIX_MAX_AGE_MS &&
                        _gps.speed.age() <= GPS_FIX_MAX_AGE_MS;
  if (freshFix) {
    _data.latitude = _gps.location.lat();
    _data.longitude = _gps.location.lng();
    _data.altitudeM = _gps.altitude.meters();
    _data.speedKmh = _gps.speed.kmph();
    _data.updatedAtMs = nowMs - _gps.location.age();
    _data.valid = true;
  } else {
    _data.valid = false;
  }

  const uint32_t referenceMs = _lastByteMs == 0 ? _startedAtMs : _lastByteMs;
  if (static_cast<uint32_t>(nowMs - referenceMs) >= GPS_NO_DATA_WARNING_MS &&
      (_lastWarningMs == 0 ||
       static_cast<uint32_t>(nowMs - _lastWarningMs) >= GPS_NO_DATA_WARNING_MS)) {
    _lastWarningMs = nowMs;
    Logger::warn("GPS", "No UART data; verify power, TX->GPIO32 and 9600 baud");
  }
}

bool GPSNeo6M::isValid() const {
  return _data.valid;
}

const GPSData& GPSNeo6M::getData() const {
  return _data;
}
