#pragma once

#include <Arduino.h>

class CryptoUtils {
public:
    static void begin();
    static String encryptString(const String& input);
    static String decryptString(const String& input);

private:
    static uint8_t encryptionKey[16];
    static bool isInitialized;
    static void generateDeviceKey();
};

extern CryptoUtils crypto;
