// --- START OF FILE plugins/TessiePlugin.cpp ---

#include "plugins/TessiePlugin.h"
#include "screen.h"
#include "signs.h" 

// A simple structure to hold coordinates
struct Point {
    int x;
    int y;
};

// This is the wavy line the animation will follow.
const static Point animationPath[] = {
    {0, 14}, {1, 13}, {2, 13}, {3, 14}, 
    {4, 15}, {5, 15}, {6, 14}, {7, 13}, 
    {8, 13}, {9, 14}, {10, 15}, {11, 15}, 
    {12, 14}, {13, 13}, {14, 14}, {15, 14}
};
const int pathSize = sizeof(animationPath) / sizeof(animationPath[0]);

// Pixel art definition for a bold 'T'
const static Point boldT[] = {
    {4, 4}, {5, 4}, {6, 4}, {7, 4}, {8, 4}, {9, 4}, {10, 4},
    {4, 5}, {5, 5}, {6, 5}, {7, 5}, {8, 5}, {9, 5}, {10, 5},
    {6, 6}, {7, 6}, {8, 6}, {6, 7}, {7, 7}, {8, 7},
    {6, 8}, {7, 8}, {8, 8}, {6, 9}, {7, 9}, {8, 9},
    {6, 10}, {7, 10}, {8, 10}
};
const int boldTSize = sizeof(boldT) / sizeof(boldT[0]);


const char *TessiePlugin::getName() const { return "Tessie"; }

void TessiePlugin::setup() {
    Serial.println("\n[TessiePlugin] setup() called.");
    animationStep = 0;
    pulseCounter = 0;
    pulseUp = true;
    isLoading = true; 
    
    // Configure the persistent client once.
    wifiClient.setInsecure();

    fetchChargeState(); 
}

void TessiePlugin::loop() {
    if (isLoading) {
        // --- WATCHDOG TIMER ---
        if (millis() - lastApiCallTime > 25000) {
            Serial.println("\n[TessiePlugin] WATCHDOG TIMEOUT: Background task stuck. Rebooting...");
            Screen.clear();
            drawError();
            delay(2000);
            ESP.restart();
        }

        // --- Loading Animation ---
        if (pulseUp) { if ((pulseCounter += 20) >= 200) { pulseCounter = 200; pulseUp = false; } } else { if ((pulseCounter -= 20) <= 0) { pulseCounter = 0; pulseUp = true; } }
        Screen.clear();
        int brightness = 55 + pulseCounter; 
        for (int p = 0; p < boldTSize; ++p) { Screen.setPixel(boldT[p].x, boldT[p].y, 1, brightness); }
        delay(30);
    }
    else {
        if (chargePercentage < 0) {
            Serial.println("[TessiePlugin] Soft error detected. Retrying...");
            drawError();
            delay(3000); 
            fetchChargeState();
            return;
        }
        
        if (millis() - lastApiCallTime > apiUpdateInterval) {
            Serial.printf("[TessiePlugin] Update interval (%lu ms) reached. Fetching new data.\n", apiUpdateInterval);
            fetchChargeState(); 
            return; 
        }

        Screen.clear();
        drawCharge();
        drawStatusIcon();
        if (chargingState == "Charging") { delay(80); } else { if (pulseUp) { if ((pulseCounter += 15) >= 155) { pulseCounter = 155; pulseUp = false; } } else { if ((pulseCounter -= 15) <= 0) { pulseCounter = 0; pulseUp = true; } } delay(50); }
    }
}


void TessiePlugin::fetchChargeState() {
    Serial.println("\n[TessiePlugin] fetchChargeState() called.");
    isLoading = true; 
    lastApiCallTime = millis(); 

    if (networkTaskHandle != NULL) {
        Serial.println("[TessiePlugin] Warning: Stale task handle found. Deleting it.");
        vTaskDelete(networkTaskHandle);
        networkTaskHandle = NULL;
    }

    Serial.println("[TessiePlugin] Launching background network task...");
    xTaskCreatePinnedToCore( networkTaskFunction, "TessieNetworkTask", 8192, this, 1, &networkTaskHandle, 0);                     
}


// --- MODIFIED: This version uses the PERSISTENT client objects from the plugin class ---
void TessiePlugin::networkTaskFunction(void *pvParameters) {
    Serial.println("[TessiePlugin] >> networkTaskFunction started.");
    TessiePlugin* plugin = (TessiePlugin*)pvParameters;
    
    static DynamicJsonDocument doc(2048); 
    doc.clear();

    String url = "https://api.tessie.com/" + String(TESSIE_VIN) + "/battery?access_token=" + String(TESSIE_API_KEY);
    
    // --- Use the persistent clients from the plugin instance ---
    plugin->httpClient.begin(plugin->wifiClient, url);
    plugin->httpClient.setTimeout(15000); 
    int httpCode = plugin->httpClient.GET();
    Serial.printf("[TessiePlugin] >> HTTP Response Code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        DeserializationError error = deserializeJson(doc, plugin->httpClient.getStream());
        
        if (!error) {
            if (doc.containsKey("battery_level") && doc.containsKey("pack_current")) {
                plugin->chargePercentage = doc["battery_level"];
                float packCurrent = doc["pack_current"];
                plugin->chargingState = (packCurrent > 1.0) ? "Charging" : "Disconnected";
                plugin->apiUpdateInterval = (plugin->chargingState == "Charging") ? (1 * 60 * 1000) : (5 * 60 * 1000);
            } else {
                 plugin->chargePercentage = -1;
            }
        } else {
             plugin->chargePercentage = -1;
        }
    } else {
        plugin->chargePercentage = -1;
    }

    // "Hang up" the connection, but DO NOT destroy the client objects.
    plugin->httpClient.end();

    Serial.println("[TessiePlugin] >> networkTaskFunction finished. Signaling completion.");
    plugin->isLoading = false;
    plugin->networkTaskHandle = NULL;
    vTaskDelete(NULL);
}


void TessiePlugin::drawStatusIcon() {
    if (chargingState == "Charging") {
        for (int i = 0; i < pathSize; ++i) { Screen.setPixel(animationPath[i].x, animationPath[i].y, 1, 64); }
        Point head = animationPath[animationStep];
        Screen.setPixel(head.x, head.y, 1, 255);
        int bodyIndex = (animationStep - 1 + pathSize) % pathSize;
        Point body = animationPath[bodyIndex];
        Screen.setPixel(body.x, body.y, 1, 180);
        int tailIndex = (animationStep - 2 + pathSize) % pathSize;
        Point tail = animationPath[tailIndex];
        Screen.setPixel(tail.x, tail.y, 1, 100);
        animationStep = (animationStep + 1) % pathSize;
    } else {
        int startX = 2; int startY = 15;
        Screen.drawLine(startX, startY - 3, startX + 11, startY - 3, 1);
        Screen.drawLine(startX, startY, startX + 11, startY, 1);
        Screen.setPixel(startX, startY - 1, 1);
        Screen.setPixel(startX, startY - 2, 1);
        Screen.setPixel(startX + 11, startY - 1, 1);
        Screen.setPixel(startX + 12, startY - 1, 1);
        Screen.setPixel(startX + 11, startY - 2, 1);
        Screen.setPixel(startX + 12, startY - 2, 1);
        int columnsToFill = chargePercentage / 10;
        if (chargePercentage > 0 && columnsToFill == 0) { columnsToFill = 1; }
        int brightness = 100 + pulseCounter;
        for (int i = 0; i < columnsToFill; ++i) {
            int currentX = startX + 1 + i;
            Screen.setPixel(currentX, startY - 2, 1, brightness);
            Screen.setPixel(currentX, startY - 1, 1, brightness);
        }
    }
}

void TessiePlugin::drawCharge() {
    std::vector<int> digits;
    if (chargePercentage == 100) {
        digits.push_back(1); digits.push_back(0); digits.push_back(0);
        Screen.drawNumbers(1, 5, digits);
    } else {
        if (chargePercentage < 10) { digits.push_back(0); }
        else { digits.push_back(chargePercentage / 10); }
        digits.push_back(chargePercentage % 10); 
        Screen.drawBigNumbers(0, 4, digits);
    }
}

void TessiePlugin::drawError() {
    const font& sysFont = fonts[0];
    const int charWidth = sysFont.sizeX;
    const int charOffset = sysFont.offset;
    const int startY = 4;
    const std::vector<int>& e_data = sysFont.data['E' - charOffset];
    for (int y = 0; y < e_data.size(); ++y) {
        int rowData = e_data[y];
        for (int x = 0; x < charWidth; ++x) { if ((rowData >> (7 - x)) & 1) { Screen.setPixel(0 + x, startY + y, 1); } }
    }
    const std::vector<int>& r_data = sysFont.data['R' - charOffset];
    for (int y = 0; y < r_data.size(); ++y) {
        int rowData = r_data[y];
        for (int x = 0; x < charWidth; ++x) { if ((rowData >> (7 - x)) & 1) { Screen.setPixel(6 + x, startY + y, 1); } }
    }
    for (int y = 0; y < r_data.size(); ++y) {
        int rowData = r_data[y];
        for (int x = 0; x < charWidth; ++x) { if ((rowData >> (7 - x)) & 1) { Screen.setPixel(11 + x, startY + y, 1); } }
    }
}