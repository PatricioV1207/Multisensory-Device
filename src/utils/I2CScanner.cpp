#include "utils/I2CScanner.h"

#include "config.h"
#include "utils/Logger.h"

bool I2CScanResult::contains(uint8_t address) const {
  for (size_t i = 0; i < count; ++i) {
    if (addresses[i] == address) {
      return true;
    }
  }
  return false;
}

I2CScanResult I2CScanner::scan(TwoWire& wire) {
  I2CScanResult result;
  Logger::info("I2C", "Scanning 7-bit addresses...");
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    wire.beginTransmission(address);
    const uint8_t status = wire.endTransmission();
    if (status == 0) {
      if (result.count < sizeof(result.addresses)) {
        result.addresses[result.count++] = address;
      }
      char message[32];
      snprintf(message, sizeof(message), "Found device at 0x%02X", address);
      Logger::info("I2C", message);
    }
  }
  if (result.count == 0) {
    Logger::warn("I2C", "No devices detected; check power, GND, SDA and SCL");
  }
  reportExpected(result);
  return result;
}

void I2CScanner::reportExpected(const I2CScanResult& result) {
  const bool accel = result.contains(ADXL345_ADDRESS_PRIMARY) ||
                     result.contains(ADXL345_ADDRESS_ALTERNATE);
  const bool gyro = result.contains(L3G4200D_ADDRESS_PRIMARY) ||
                    result.contains(L3G4200D_ADDRESS_ALTERNATE);
  const bool mag = result.contains(HMC5883L_ADDRESS);
  const bool baro = result.contains(BMP180_ADDRESS);
  const bool light = result.contains(BH1750_ADDRESS_PRIMARY) ||
                     result.contains(BH1750_ADDRESS_ALTERNATE);

  Logger::info("I2C", String("ADXL345: ") + (accel ? "address present" : "MISSING"));
  Logger::info("I2C", String("L3G4200D: ") + (gyro ? "address present" : "MISSING"));
  Logger::info("I2C", String("HMC5883L: ") + (mag ? "address present" : "MISSING"));
  Logger::info("I2C", String("BMP180/BMP085: ") + (baro ? "address present" : "MISSING"));
  Logger::info("I2C", String("BH1750: ") + (light ? "address present" : "MISSING"));

  if (!mag && result.contains(QMC5883L_POSSIBLE_ADDRESS)) {
    Logger::warn("I2C", "0x0D detected: possible QMC5883L clone; HMC driver disabled");
  }
}
