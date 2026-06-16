#pragma once
#include <Arduino.h>

class AsyncWebServer;

class AlexaManager {
public:
    static void init(AsyncWebServer* server);
    static void loop();
};
