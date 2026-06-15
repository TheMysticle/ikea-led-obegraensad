#pragma once

#include "PluginManager.h"
#include "timing.h"

class CometPlugin : public Plugin
{
private:
  NonBlockingDelay timer;
  float x = 0.0f;
  float y = 0.0f;
  float vx = 0.8f;
  float vy = 0.5f;
  uint8_t trail[16][16] = {};

  void resetComet();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

  bool hasSpeedControl() const override { return true; }
  void setSpeed(int speed) override { 
    currentSpeed = speed;
    delayTime = map(speed, 1, 100, 120, 10);
  }
  int getSpeed() const override { return currentSpeed; }
  int getDefaultSpeed() const override { return 50; }

private:
  int currentSpeed = 50;
  int delayTime = 40;
};
