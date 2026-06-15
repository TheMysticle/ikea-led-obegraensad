#pragma once

#include "PluginManager.h"
#include "timing.h"

class FirefliesPlugin : public Plugin
{
private:
  struct Firefly
  {
    float x;
    float y;
    float vx;
    float vy;
  };

  NonBlockingDelay timer;
  static constexpr uint8_t kFireflyCount = 10;
  Firefly fireflies[kFireflyCount];

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
  int delayTime = 60;
};
