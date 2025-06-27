// --- START OF FILE plugins/TessiePlugin.h ---

#pragma once

#include "PluginManager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h" 

// --- ADDED: For managing the background network task ---
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
    
    // --- MODIFIED: Refresh interval is now a variable, not a constant ---
    unsigned long apiUpdateInterval = 300000; // Default to 5 minutes

    // --- ADDED: Variables for background loading ---
    // This flag tells the main loop we are waiting for data
    volatile bool isLoading = true; 
    // This handle lets us manage the background task
    TaskHandle_t networkTaskHandle = NULL;

    void fetchChargeState();
    void drawError();
    void drawCharge();
    void drawStatusIcon();
    
    // --- ADDED: The function that will run on the second core ---
    // It must be static, but we pass 'this' instance to it.
    static void networkTaskFunction(void *pvParameters);

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};