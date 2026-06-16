#include "AlexaManager.h"
#define ESPALEXA_ASYNC // Important: Define this before including Espalexa.h!
#include <Espalexa.h>
#include "screen.h"
#include "scheduler.h"
#include "Logger.h"

extern uint8_t lastKnownBrightness;
Espalexa espalexa;

void setLedWallPower(uint8_t brightness)
{
    Logger::print("Alexa command received. New brightness: ");
    Logger::println(String(brightness));

    // This is the "Turn Off" command
    if (brightness == 0)
    {
        // Get the brightness from the screen class BEFORE we turn it off
        uint8_t currentBrightness = Screen.getCurrentBrightness();
        if (currentBrightness > 0)
        {
            // Save the current brightness level into our runtime variable
            lastKnownBrightness = currentBrightness;
            Logger::print("Saving last known brightness for this session: ");
            Logger::println(String(lastKnownBrightness));
        }
        
        // Set brightness to 0 and PERSIST this "off" state
        Screen.setBrightness(0, true);
        if (Scheduler.isActive) {
            Scheduler.isBrightnessOverridden = true;
        }
    }
    // This is the "Turn On" or "Dim" command
    else
    {
        // This is a generic "Turn On" command (value=255) from a fully off state
        if (Screen.getCurrentBrightness() == 0 && brightness == 255)
        {
            Logger::print("Restoring last known brightness: ");
            Logger::println(String(lastKnownBrightness));
            // Restore the saved brightness instead of using Alexa's 255, and PERSIST it
            Screen.setBrightness(lastKnownBrightness, true);
        }
        else
        {
            // This is a specific dimming command (e.g., "set to 30%").
            // Use the value from Alexa and PERSIST it.
            Logger::print("Setting specific brightness to: ");
            Logger::println(String(brightness));
            Screen.setBrightness(brightness, true);
            
            // Also update our runtime variable with this new specific value
            lastKnownBrightness = brightness;
        }
        if (Scheduler.isActive) {
            Scheduler.isBrightnessOverridden = true;
        }
    }
}

#include "config.h"

void AlexaManager::init(AsyncWebServer* server) {
    espalexa.addDevice(config.getAlexaDeviceName().c_str(), setLedWallPower); // Name Alexa will recognize from Config
    espalexa.begin(server); // Pass your existing AsyncWebServer instance to Espalexa
}

void AlexaManager::loop() {
    espalexa.loop();
}

void AlexaManager::updateDeviceState(uint8_t brightness) {
    // Only update if espalexa has devices
    EspalexaDevice* d = espalexa.getDevice(0);
    if (d != nullptr) {
        d->setValue(brightness);
    }
}
