#pragma once

#include "PluginManager.h"
#include "timing.h"

class SpiralPlugin : public Plugin
{
private:
  NonBlockingDelay timer;
  static constexpr uint8_t WIDTH = 16;
  static constexpr uint8_t HEIGHT = 16;
  static constexpr uint8_t CENTER_X = 8;
  static constexpr uint8_t CENTER_Y = 8;
  
  float angle;
  float radius;
  bool expanding;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

  bool hasSpeedControl() const override { return true; }
  void setSpeed(int speed) override { 
    currentSpeed = speed;
    delayTime = map(speed, 1, 100, 100, 5);
  }
  int getSpeed() const override { return currentSpeed; }
  int getDefaultSpeed() const override { return 50; }

private:
  int currentSpeed = 50;
  int delayTime = 30;
};
