#pragma once

// Use GPIO numbers, not the sometimes inconsistent silk-screen labels on clones.
#define PIN_DHT11_DATA 27

#define PIN_GPS_RX 16  // ESP32 RX2 <- GPS TX
#define PIN_GPS_TX 17  // ESP32 TX2 -> GPS RX (optional)

#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

