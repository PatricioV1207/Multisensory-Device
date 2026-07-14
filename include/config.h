#pragma once

#include <stdint.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

// Build modes selected by platformio.ini.
#define APP_MODE_FULL_PROTOTYPE 0
#define APP_MODE_TEST_DHT11 1
#define APP_MODE_TEST_GPS 2
#define APP_MODE_TEST_I2C_SCANNER 3
#define APP_MODE_TEST_ADXL345 4
#define APP_MODE_TEST_L3G4200D 5
#define APP_MODE_TEST_HMC5883L 6
#define APP_MODE_TEST_BMP180 7
#define APP_MODE_TEST_GY801 8
#define APP_MODE_TEST_WIFI 9
#define APP_MODE_TEST_MQTT_WIFI 10

#ifndef APP_MODE
#define APP_MODE APP_MODE_FULL_PROTOTYPE
#endif

#ifndef DEVICE_ID
#define DEVICE_ID "bus_iot_prototype_01"
#endif

#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#ifndef GPS_BAUD
#define GPS_BAUD 9600
#endif

#ifndef DHT_READ_INTERVAL_MS
#define DHT_READ_INTERVAL_MS 2000UL
#endif

#ifndef IMU_READ_INTERVAL_MS
#define IMU_READ_INTERVAL_MS 100UL
#endif

#ifndef TELEMETRY_INTERVAL_MS
#define TELEMETRY_INTERVAL_MS 10000UL
#endif

#ifndef TEST_OUTPUT_INTERVAL_MS
#define TEST_OUTPUT_INTERVAL_MS 2000UL
#endif

#ifndef I2C_SCAN_INTERVAL_MS
#define I2C_SCAN_INTERVAL_MS 10000UL
#endif

#ifndef SENSOR_RETRY_INTERVAL_MS
#define SENSOR_RETRY_INTERVAL_MS 30000UL
#endif

#ifndef GPS_FIX_MAX_AGE_MS
#define GPS_FIX_MAX_AGE_MS 5000UL
#endif

#ifndef GPS_NO_DATA_WARNING_MS
#define GPS_NO_DATA_WARNING_MS 5000UL
#endif

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 12000UL
#endif

#ifndef RECONNECT_BACKOFF_MIN_MS
#define RECONNECT_BACKOFF_MIN_MS 5000UL
#endif

#ifndef RECONNECT_BACKOFF_MAX_MS
#define RECONNECT_BACKOFF_MAX_MS 30000UL
#endif

#ifndef I2C_TIMEOUT_MS
#define I2C_TIMEOUT_MS 50U
#endif

#ifndef MQTT_BUFFER_SIZE
#define MQTT_BUFFER_SIZE 1536U
#endif

#ifndef MQTT_KEEPALIVE_SECONDS
#define MQTT_KEEPALIVE_SECONDS 30U
#endif

#ifndef MQTT_SOCKET_TIMEOUT_SECONDS
#define MQTT_SOCKET_TIMEOUT_SECONDS 3U
#endif

#ifndef TELEMETRY_PAYLOAD_BUFFER_SIZE
#define TELEMETRY_PAYLOAD_BUFFER_SIZE 1536U
#endif

#ifndef SEA_LEVEL_PRESSURE_HPA
#define SEA_LEVEL_PRESSURE_HPA 1013.25F
#endif

// ADXL345 six-position calibration in m/s^2.
// Raw readings are corrected as: calibrated = (raw - offset) * scale.
#ifndef ADXL_OFFSET_X
#define ADXL_OFFSET_X -0.3607F
#endif

#ifndef ADXL_OFFSET_Y
#define ADXL_OFFSET_Y -0.1363F
#endif

#ifndef ADXL_OFFSET_Z
#define ADXL_OFFSET_Z 1.2093F
#endif

#ifndef ADXL_SCALE_X
#define ADXL_SCALE_X 0.95937F
#endif

#ifndef ADXL_SCALE_Y
#define ADXL_SCALE_Y 0.96348F
#endif

#ifndef ADXL_SCALE_Z
#define ADXL_SCALE_Z 0.99721F
#endif

// Expected 7-bit addresses. Alternate addresses are probed where supported.
#define ADXL345_ADDRESS_PRIMARY 0x53
#define ADXL345_ADDRESS_ALTERNATE 0x1D
#define L3G4200D_ADDRESS_PRIMARY 0x69
#define L3G4200D_ADDRESS_ALTERNATE 0x68
#define HMC5883L_ADDRESS 0x1E
#define QMC5883L_POSSIBLE_ADDRESS 0x0D
#define BMP180_ADDRESS 0x77

// Safe defaults allow sensor-only environments to compile without secrets.h.
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef MQTT_HOST
#define MQTT_HOST ""
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

#ifndef MQTT_TOPIC
#define MQTT_TOPIC "v1/devices/me/telemetry"
#endif
