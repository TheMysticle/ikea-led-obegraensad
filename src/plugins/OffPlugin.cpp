#include "plugins/OffPlugin.h"

void OffPlugin::setup()
{
    // No specific setup needed for an "off" plugin, but we can clear the screen to ensure it's off from the start.
    Screen.clear();
}

void OffPlugin::loop()
{
    // Continuously clear the screen to keep all LEDs off.
    Screen.clear();
}

const char *OffPlugin::getName() const
{
    return "Off";
}