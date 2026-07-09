#include "utils/Logger.h"

#include "config.h"

void Logger::begin(unsigned long baud) {
  Serial.begin(baud);
}

void Logger::debug(const char* tag, const String& message) {
  if (LOG_LEVEL <= LOG_LEVEL_DEBUG) {
    write("DEBUG", tag, message);
  }
}

void Logger::info(const char* tag, const String& message) {
  if (LOG_LEVEL <= LOG_LEVEL_INFO) {
    write("INFO", tag, message);
  }
}

void Logger::warn(const char* tag, const String& message) {
  if (LOG_LEVEL <= LOG_LEVEL_WARN) {
    write("WARN", tag, message);
  }
}

void Logger::error(const char* tag, const String& message) {
  if (LOG_LEVEL <= LOG_LEVEL_ERROR) {
    write("ERROR", tag, message);
  }
}

void Logger::write(const char* level, const char* tag, const String& message) {
  Serial.printf("[%10lu] [%-5s] [%-8s] %s\n", millis(), level, tag,
                message.c_str());
}
