#pragma once

#include "PluginManager.h"
#include "screen.h"

class OffPlugin : public Plugin
{
public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};