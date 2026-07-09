#pragma once

#include <Arduino.h>

class Logger {
 public:
  static void begin(unsigned long baud);
  static void debug(const char* tag, const String& message);
  static void info(const char* tag, const String& message);
  static void warn(const char* tag, const String& message);
  static void error(const char* tag, const String& message);

 private:
  static void write(const char* level, const char* tag, const String& message);
};

