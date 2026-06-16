#pragma once
#include <Arduino.h>

class AsyncWebServer;

class AlexaManager {
public:
    static void init(AsyncWebServer* server);
    static void loop();
    static void updateDeviceState(uint8_t brightness);
};
