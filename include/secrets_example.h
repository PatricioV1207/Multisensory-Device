#pragma once

// Copy this file as include/secrets.h and replace the example values.
// Never commit include/secrets.h.
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Public identities. They are not passwords, but each deployed device and
// vehicle must use stable unique values.
#define DEVICE_ID "vehiclesense_device_01"
#define VEHICLE_ID "vehicle_01"

// ThingsBoard: use the device access token as MQTT_USERNAME and leave
// MQTT_PASSWORD empty. For another broker, configure all fields as required.
#define MQTT_HOST "demo.thingsboard.io"
#define MQTT_PORT 1883
#define MQTT_USERNAME "YOUR_DEVICE_ACCESS_TOKEN"
#define MQTT_PASSWORD ""
#define MQTT_TOPIC "v1/devices/me/telemetry"

// VehicleSense production transport: HiveMQ Cloud MQTT over TLS. The ESP32
// uses the built-in CA bundle and still validates the broker hostname.
#define HIVEMQ_HOST "YOUR_CLUSTER.s1.eu.hivemq.cloud"
#define HIVEMQ_PORT 8883
#define HIVEMQ_USERNAME "YOUR_HIVEMQ_DEVICE_USERNAME"
#define HIVEMQ_PASSWORD "YOUR_HIVEMQ_DEVICE_PASSWORD"

// SIM800L / GPRS. Confirm the APN with the mobile operator.
#define CELLULAR_APN "YOUR_OPERATOR_APN"
#define CELLULAR_APN_USER ""
#define CELLULAR_APN_PASSWORD ""
#define CELLULAR_SIM_PIN ""

// Real cellular telemetry. Keep TLS enabled unless using a controlled lab
// broker and ALLOW_INSECURE_CELLULAR_MQTT is explicitly enabled in config.h.
#define CELLULAR_MQTT_HOST "YOUR_CLOUD_BROKER_HOST"
#define CELLULAR_MQTT_PORT 8883
#define CELLULAR_MQTT_USERNAME "YOUR_CLOUD_MQTT_USERNAME"
#define CELLULAR_MQTT_PASSWORD "YOUR_CLOUD_MQTT_PASSWORD"
#define CELLULAR_MQTT_TOPIC "buses/prototype/telemetry"

// Insecure MQTT diagnostics publish synthetic data only.
#define MQTT_TEST_HOST "YOUR_TEST_BROKER_HOST"
#define MQTT_TEST_PORT 1883
#define MQTT_TEST_USERNAME ""
#define MQTT_TEST_PASSWORD ""
#define MQTT_TEST_TOPIC "buses/prototype/diagnostics"

// Local WPA2 access point and separate HTTP Basic administrator credentials.
#define LOCAL_AP_SSID "VehicleSense-Local"
#define LOCAL_AP_PASSWORD "YOUR_AP_PASSWORD_MIN_8_CHARS"
#define LOCAL_ADMIN_USERNAME "admin"
#define LOCAL_ADMIN_PASSWORD "YOUR_ADMIN_PASSWORD"
