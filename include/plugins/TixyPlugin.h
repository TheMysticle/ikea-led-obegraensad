#pragma once

#include "PluginManager.h"

class TixyPlugin : public Plugin
{

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
  float code(double t, double i, double x, double y);
};