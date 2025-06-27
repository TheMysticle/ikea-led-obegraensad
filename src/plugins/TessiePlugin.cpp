// --- START OF FILE plugins/TessiePlugin.cpp ---

#include "plugins/TessiePlugin.h"
#include "screen.h"
#include <WiFiClientSecure.h> 
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


// Returns the name of the plugin
const char *TessiePlugin::getName() const
{
    return "Tessie";
}

// Called once when the plugin is activated
void TessiePlugin::setup()
{
    // Initialize variables and kick off the first data fetch
    animationStep = 0;
    pulseCounter = 0;
    pulseUp = true;
    isLoading = true; // Start in loading state
    fetchChargeState(); // This will now launch a background task
}

// --- MODIFIED: loop() is now a state machine ---
void TessiePlugin::loop()
{
    if (isLoading)
    {
        // --- STATE 1: LOADING ---
        // We are waiting for the network task to finish.
        // Run the pulsing 'T' animation continuously.
        
        // Update pulse animation counter
        if (pulseUp) {
            pulseCounter += 20;
            if (pulseCounter >= 200) { pulseCounter = 200; pulseUp = false; }
        } else {
            pulseCounter -= 20;
            if (pulseCounter <= 0) { pulseCounter = 0; pulseUp = true; }
        }

        Screen.clear();
        int brightness = 55 + pulseCounter; // Brightness oscillates between 55 and 255

        // Draw the bold 'T' with the current brightness
        for (int p = 0; p < boldTSize; ++p) {
            Screen.setPixel(boldT[p].x, boldT[p].y, 1, brightness);
        }
        delay(30);
    }
    else
    {
        // --- STATE 2: DISPLAYING ---
        // Data is loaded, so we display it.
        
        // Check if it's time to fetch new data
        if (millis() - lastApiCallTime > apiUpdateInterval)
        {
            fetchChargeState(); // This will set isLoading=true and switch states
            return; // Exit loop to start showing animation immediately
        }

        // Run the normal display logic
        Screen.clear();

        if (chargePercentage < 0) {
            drawError();
        } else {
            drawCharge();
            drawStatusIcon();
        }

        // Adjust delay for animation vs static display
        if (chargingState == "Charging") {
            delay(80); 
        } else {
            // Update pulse counter for the battery fill
            if (pulseUp) {
                pulseCounter += 15;
                if (pulseCounter >= 155) { pulseCounter = 155; pulseUp = false; }
            } else {
                pulseCounter -= 15;
                if (pulseCounter <= 0) { pulseCounter = 0; pulseUp = true; }
            }
            delay(50);
        }
    }
}

// --- MODIFIED: This now just launches the background task ---
void TessiePlugin::fetchChargeState()
{
    isLoading = true; // Set the flag to start the loading animation
    lastApiCallTime = millis();

    // Create a task that will run 'networkTaskFunction' on Core 0
    // with a stack size of 10000 bytes and passing 'this' plugin instance.
    xTaskCreatePinnedToCore(
        networkTaskFunction,    // Function to implement the task
        "TessieNetworkTask",    // Name of the task
        10000,                  // Stack size in words
        this,                   // Task input parameter (passes the instance of our class)
        1,                      // Priority of the task
        &networkTaskHandle,     // Task handle to keep track of created task
        0);                     // Pin to core 0
}


// --- NEW: This function runs in the background to fetch data ---
void TessiePlugin::networkTaskFunction(void *pvParameters)
{
    // The parameter is the 'this' pointer from the class instance.
    TessiePlugin* plugin = (TessiePlugin*)pvParameters;

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();

    String url = "https://api.tessie.com/vehicles?access_token=" + String(TESSIE_API_KEY);
    
    http.begin(client, url);
    http.setTimeout(15000); 

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        DynamicJsonDocument doc(16384); 
        DeserializationError error = deserializeJson(doc, http.getStream());
        
        if (!error)
        {
            JsonArray results = doc["results"].as<JsonArray>();
            for (JsonObject vehicle : results) {
                const char* vin = vehicle["vin"];
                if (vin && strcmp(vin, TESSIE_VIN) == 0) {
                    if (vehicle.containsKey("last_state") && 
                        vehicle["last_state"].containsKey("charge_state") && 
                        vehicle["last_state"]["charge_state"].containsKey("battery_level")) {
                        
                        // Update the plugin's variables
                        plugin->chargePercentage = vehicle["last_state"]["charge_state"]["battery_level"];
                        plugin->chargingState = vehicle["last_state"]["charge_state"]["charging_state"].as<String>();

                        // --- DYNAMIC REFRESH LOGIC ---
                        // If car is disconnected, wait 5 mins. Otherwise, wait 1 min.
                        if (plugin->chargingState == "Disconnected") {
                            plugin->apiUpdateInterval = 5 * 60 * 1000; // 5 minutes
                        } else {
                            plugin->apiUpdateInterval = 1 * 60 * 1000; // 1 minute
                        }
                    }
                    break; 
                }
            }
        } else {
            plugin->chargePercentage = -1; // JSON error
        }
    } else {
        plugin->chargePercentage = -1; // HTTP error
    }
    
    http.end();

    // Signal that loading is complete
    plugin->isLoading = false;

    // The task is done, so it deletes itself
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