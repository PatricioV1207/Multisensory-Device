#pragma once

// Use GPIO numbers, not the sometimes inconsistent silk-screen labels on clones.
#define PIN_DHT11_DATA 27

#define PIN_SIM800_RX 16  // ESP32 RX1 <- SIM800L TX
#define PIN_SIM800_TX 17  // ESP32 TX1 -> SIM800L RX

#define PIN_GPS_RX 32  // ESP32 RX2 <- GPS TX
#define PIN_GPS_TX 33  // ESP32 TX2 -> GPS RX (optional)

#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

#define PIN_SD_SCK 18
#define PIN_SD_MISO 19
#define PIN_SD_MOSI 23
#define PIN_SD_CS 5

#define PIN_INMP441_BCLK 26
#define PIN_INMP441_WS 25
#define PIN_INMP441_DATA 34
