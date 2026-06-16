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
        // Convert Alexa's 254-based scale back to the intended percentage (1-100%)
        uint8_t percent = 100;
        if (brightness < 255) {
            uint8_t briL = brightness - 1;
            // Alexa's Hue emulation shifts the mapping so 1% = bri 4, 50% = bri 127
            // We cleanly invert this using integer rounding
            if (briL <= 1) {
                percent = 1;
            } else {
                percent = (uint8_t)(((briL - 1) * 100 + 126) / 253);
                if (percent == 0) percent = 1;
            }
        }

        // Convert that percentage perfectly into the 255-based scale the frontend expects
        uint8_t mappedBrightness = (uint8_t)((percent * 255 + 50) / 100);

        // This is a generic "Turn On" command (value=255) from a fully off state
        if (!Screen.isPowerOn() && brightness == 255)
        {
            Logger::println("Restoring last known brightness via Power On.");
            Screen.setPower(true);
        }
        else
        {
            // This is a specific dimming command
            Logger::print("Setting specific brightness to: ");
            Logger::println(String(mappedBrightness));
            Screen.setBrightness(mappedBrightness, true);
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
