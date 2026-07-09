#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include "communication/MQTTClientCustom.h"
#include "communication/WiFiManagerCustom.h"
#include "sensors/DHT11Sensor.h"
#include "sensors/GPSNeo6M.h"
#include "sensors/gy801/ADXL345Accel.h"
#include "sensors/gy801/BMP180Barometer.h"
#include "sensors/gy801/GY801IMU.h"
#include "sensors/gy801/HMC5883LMag.h"
#include "sensors/gy801/L3G4200DGyro.h"

class ModuleTestRunner {
 public:
  void begin();
  void update(uint32_t nowMs);

 private:
  bool usesI2C() const;
  void printReadings(uint32_t nowMs);
  void publishMqttTest(uint32_t nowMs);

  DHT11Sensor _dht;
  GPSNeo6M _gps;
  ADXL345Accel _accel;
  L3G4200DGyro _gyro;
  HMC5883LMag _mag;
  BMP180Barometer _barometer;
  GY801IMU _gy801;
  WiFiManagerCustom _wifi;
  WiFiClient _wifiClient;
  MQTTClientCustom _mqtt;
  uint32_t _lastOutputMs = 0;
  uint32_t _lastScanMs = 0;
  uint32_t _lastPublishMs = 0;
};

