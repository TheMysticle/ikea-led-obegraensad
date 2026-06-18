#pragma once

#include <Arduino.h>

class CrashLogger {
public:
    static void init();
    static String getLastCrashReason();
    static String getLastBacktrace();
    static void clearLastCrashReason();
};
