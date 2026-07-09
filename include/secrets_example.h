#pragma once

// Copy this file as include/secrets.h and replace the example values.
// Never commit include/secrets.h.
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ThingsBoard: use the device access token as MQTT_USERNAME and leave
// MQTT_PASSWORD empty. For another broker, configure all fields as required.
#define MQTT_HOST "demo.thingsboard.io"
#define MQTT_PORT 1883
#define MQTT_USERNAME "YOUR_DEVICE_ACCESS_TOKEN"
#define MQTT_PASSWORD ""
#define MQTT_TOPIC "v1/devices/me/telemetry"

