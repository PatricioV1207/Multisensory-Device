#pragma once

#include <Arduino.h>
#include <Wire.h>

struct I2CScanResult {
  uint8_t addresses[24] = {0};
  size_t count = 0;

  bool contains(uint8_t address) const;
};

class I2CScanner {
 public:
  static I2CScanResult scan(TwoWire& wire = Wire);
  static void reportExpected(const I2CScanResult& result);
};

