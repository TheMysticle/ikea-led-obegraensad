#include "AlexaManager.h"
#define ESPALEXA_ASYNC // Important: Define this before including Espalexa.h!
#include <Espalexa.h>
#include "screen.h"
#include "scheduler.h"
#include "Logger.h"

Espalexa espalexa;

void setLedWallPower(uint8_t brightness)
{
    Logger::print("Alexa command received. New brightness: ");
    Logger::println(String(brightness));

    // Fix Alexa's 1% mapping: Echo sometimes sends 4 for 1%, which Web UI rounds to 2%.
    // Forcing it to 3 ensures Web UI mathematically rounds to exactly 1%.
    if (brightness > 0 && brightness <= 4) {
        brightness = 3;
    }

    // This is the "Turn Off" command
    if (brightness == 0)
    {
        Screen.setPower(false);
        if (Scheduler.isActive) {
            Scheduler.isBrightnessOverridden = true;
        }
    }
    // This is the "Turn On" or "Dim" command
    else
    {
        // This is a generic "Turn On" command (value=255) from a fully off state
        if (!Screen.isPowerOn() && brightness == 255)
        {
            Logger::println("Restoring last known brightness via Power On.");
            Screen.setPower(true);
        }
        else
        {
            // This is a specific dimming command (e.g., "set to 30%").
            Logger::print("Setting specific brightness to: ");
            Logger::println(String(brightness));
            Screen.setBrightness(brightness, true);
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
