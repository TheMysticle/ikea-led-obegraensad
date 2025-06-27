#pragma once

#include "PluginManager.h"

class TixyPlugin : public Plugin
{
private:
  int currentPreset = 0; // To hold the state of the current preset

public:
  void setup() override;
  void loop() override;
  void nextPreset(); // New method to manually change the preset
  const char *getName() const override;
  float code(double t, double i, double x, double y);
};