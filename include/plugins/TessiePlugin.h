// --- START OF FILE plugins/TessiePlugin.h ---

#pragma once

#include "PluginManager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h> // We now need this here
#include <ArduinoJson.h>
#include "secrets.h" 

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class TessiePlugin : public Plugin
{
private:
    int chargePercentage = -1; 
    String chargingState = "";
    int animationStep = 0;
    int pulseCounter = 0;
    bool pulseUp = true;
    unsigned long lastApiCallTime = 0;
    unsigned long apiUpdateInterval = 300000; 
    volatile bool isLoading = true; 
    TaskHandle_t networkTaskHandle = NULL;

    // --- NEW: Persistent Client Objects ---
    // These are created once when the plugin loads and are reused for every
    // network request. This is the key to preventing resource leaks.
    HTTPClient httpClient;
    WiFiClientSecure wifiClient;

    void fetchChargeState();
    void drawError();
    void drawCharge();
    void drawStatusIcon();
    
    static void networkTaskFunction(void *pvParameters);

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};