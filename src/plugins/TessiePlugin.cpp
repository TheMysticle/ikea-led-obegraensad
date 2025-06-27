// --- START OF FILE plugins/TessiePlugin.cpp ---

#include "plugins/TessiePlugin.h"
#include "screen.h"
#include <WiFiClientSecure.h> 

// A simple structure to hold coordinates
struct Point {
    int x;
    int y;
};

// --- ADD THIS: Define the animation path based on your image ---
// This is the wavy line the animation will follow.
const static Point animationPath[] = {
    {0, 14}, {1, 13}, {2, 13}, {3, 14}, 
    {4, 15}, {5, 15}, {6, 14}, {7, 13}, 
    {8, 13}, {9, 14}, {10, 15}, {11, 15}, 
    {12, 14}, {13, 13}, {14, 13}, {15, 14}
};
const int pathSize = sizeof(animationPath) / sizeof(animationPath[0]);


// Returns the name of the plugin
const char *TessiePlugin::getName() const
{
    return "Tessie";
}

// Called once when the plugin is activated
void TessiePlugin::setup()
{
    animationStep = 0; // Reset animation on setup
    fetchChargeState(); // Initial data fetch
    lastApiCallTime = millis();
}

// --- UPDATED loop() for animation timing ---
void TessiePlugin::loop()
{
    // Check if it's time to fetch new data from the API
    if (millis() - lastApiCallTime > apiUpdateInterval)
    {
        fetchChargeState();
        lastApiCallTime = millis();
    }

    Screen.clear();

    if (chargePercentage < 0)
    {
        drawError();
    }
    else
    {
        drawCharge();
        drawStatusIcon();
    }

    // Adjust delay for smooth animation vs static display
    if (chargingState == "Charging") {
        delay(80); // Fast delay for smooth animation
    } else {
        delay(2000); // Slow delay for static display
    }
}

// --- UPDATED drawStatusIcon() with animation logic ---
void TessiePlugin::drawStatusIcon() {
    if (chargingState == "Charging") {
        // Draw the static path first
        for (int i = 0; i < pathSize; ++i) {
            Screen.setPixel(animationPath[i].x, animationPath[i].y, 1, 64); // Dim path
        }

        // Draw a 3-pixel "comet" that moves along the path
        // Head (brightest)
        Point head = animationPath[animationStep];
        Screen.setPixel(head.x, head.y, 1, 255);

        // Body
        int bodyIndex = (animationStep - 1 + pathSize) % pathSize;
        Point body = animationPath[bodyIndex];
        Screen.setPixel(body.x, body.y, 1, 180);

        // Tail (dimmest)
        int tailIndex = (animationStep - 2 + pathSize) % pathSize;
        Point tail = animationPath[tailIndex];
        Screen.setPixel(tail.x, tail.y, 1, 100);

        // Advance the animation for the next frame
        animationStep = (animationStep + 1) % pathSize;
    } else {
        // Draw the static "Battery" icon for all other states
        int startX = 2;
        int startY = 13;
        Screen.drawLine(startX, startY - 2, startX + 10, startY - 2, 1);
        Screen.drawLine(startX, startY, startX + 10, startY, 1);
        Screen.setPixel(startX, startY - 1, 1);
        Screen.setPixel(startX + 10, startY - 1, 1);
        Screen.setPixel(startX + 11, startY - 1, 1);
    }
}


// Draws the charge percentage on the screen
void TessiePlugin::drawCharge()
{
    std::vector<int> digits;

    if (chargePercentage == 100)
    {
        digits.push_back(1); digits.push_back(0); digits.push_back(0);
        Screen.drawNumbers(1, 5, digits); // Shifted up to make room
    }
    else
    {
        if (chargePercentage < 10) { digits.push_back(0); }
        else { digits.push_back(chargePercentage / 10); }
        digits.push_back(chargePercentage % 10); 
        Screen.drawBigNumbers(0, 4, digits); // Shifted up to make room
    }
}

// Draws an error message (E R R) by manually placing pixels.
void TessiePlugin::drawError()
{
    const font& sysFont = fonts[0];
    const int charWidth = sysFont.sizeX;
    const int charOffset = sysFont.offset;
    const int startY = 4;

    const std::vector<int>& e_data = sysFont.data['E' - charOffset];
    for (int y = 0; y < e_data.size(); ++y) {
        int rowData = e_data[y];
        for (int x = 0; x < charWidth; ++x) {
            if ((rowData >> (7 - x)) & 1) { Screen.setPixel(0 + x, startY + y, 1); }
        }
    }

    const std::vector<int>& r_data = sysFont.data['R' - charOffset];
    for (int y = 0; y < r_data.size(); ++y) {
        int rowData = r_data[y];
        for (int x = 0; x < charWidth; ++x) {
            if ((rowData >> (7 - x)) & 1) { Screen.setPixel(6 + x, startY + y, 1); }
        }
    }

    for (int y = 0; y < r_data.size(); ++y) {
        int rowData = r_data[y];
        for (int x = 0; x < charWidth; ++x) {
            if ((rowData >> (7 - x)) & 1) { Screen.setPixel(11 + x, startY + y, 1); }
        }
    }
}


// Fetches vehicle data from the Tessie API and extracts the charge state.
void TessiePlugin::fetchChargeState()
{
    currentStatus = LOADING;
    Screen.clear();
    Screen.setPixel(4, 7, 1); Screen.setPixel(5, 7, 1);
    Screen.setPixel(7, 7, 1); Screen.setPixel(8, 7, 1);
    Screen.setPixel(10, 7, 1); Screen.setPixel(11, 7, 1);

    chargePercentage = -1;
    chargingState = ""; 

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
        
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
        }
        else
        {
            JsonArray results = doc["results"].as<JsonArray>();
            for (JsonObject vehicle : results) {
                const char* vin = vehicle["vin"];
                if (vin && strcmp(vin, TESSIE_VIN) == 0) {
                    if (vehicle.containsKey("last_state") && 
                        vehicle["last_state"].containsKey("charge_state") && 
                        vehicle["last_state"]["charge_state"].containsKey("battery_level")) {
                        
                        chargePercentage = vehicle["last_state"]["charge_state"]["battery_level"];
                        chargingState = vehicle["last_state"]["charge_state"]["charging_state"].as<String>();
                    }
                    break; 
                }
            }
        }
    }
    
    http.end();
    currentStatus = NONE;
}