#pragma once

#include "ESPAsyncWebServer.h"

void handleGetData(AsyncWebServerRequest *request);
void handleMessage(AsyncWebServerRequest *request);
void handleMessageRemove(AsyncWebServerRequest *request);
void handleGetInfo(AsyncWebServerRequest *request);
void handleSetPlugin(AsyncWebServerRequest *request);
void handleSetBrightness(AsyncWebServerRequest *request);
void handleGetData(AsyncWebServerRequest *request);
void handleSetSchedule(AsyncWebServerRequest *request, const String& body);
void handleClearSchedule(AsyncWebServerRequest *request);
void handleStopSchedule(AsyncWebServerRequest *request);
void handleStartSchedule(AsyncWebServerRequest *request);
void handleClearStorage(AsyncWebServerRequest *request);