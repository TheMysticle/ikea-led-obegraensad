// --- START OF FILE plugins/TessiePlugin.h ---

#pragma once

#include "PluginManager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h" 

class TessiePlugin : public Plugin
{
private:
    int chargePercentage = -1; 
    String chargingState = "";
    int animationStep = 0; // --- ADD THIS LINE ---
    unsigned long lastApiCallTime = 0;
    const unsigned long apiUpdateInterval = 600000; 

    void fetchChargeState();
    void drawError();
    void drawCharge();
    void drawStatusIcon();

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};