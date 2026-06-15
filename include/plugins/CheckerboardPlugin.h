#pragma once

#include "PluginManager.h"
#include "timing.h"

class CheckerboardPlugin : public Plugin
{
private:
  NonBlockingDelay timer;
  static constexpr uint8_t WIDTH = 16;
  static constexpr uint8_t HEIGHT = 16;
  static constexpr uint8_t SQUARE_SIZE = 2;
  
  uint8_t offset;
  bool inverted;
  uint8_t phase;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

  bool hasSpeedControl() const override { return true; }
  void setSpeed(int speed) override { 
    currentSpeed = speed;
    delayTime = map(speed, 1, 100, 200, 10);
  }
  int getSpeed() const override { return currentSpeed; }
  int getDefaultSpeed() const override { return 50; }

private:
  int currentSpeed = 50;
  int delayTime = 100;
};
