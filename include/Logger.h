#pragma once
#include <Arduino.h>

class Logger {
public:
    static bool liveLoggingEnabled;
    static void println(const String& message);
    static void print(const String& message);
    static void printf(const char *format, ...);
};
