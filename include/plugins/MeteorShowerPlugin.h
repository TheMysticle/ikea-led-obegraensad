#pragma once

#include "PluginManager.h"
#include "timing.h"

class MeteorShowerPlugin : public Plugin
{
private:
  struct Meteor
  {
    float x;
    float y;
    float vx;
    float vy;
  };

  NonBlockingDelay timer;
  static constexpr uint8_t kMeteorCount = 6;
  Meteor meteors[kMeteorCount];

  void resetMeteor(Meteor &meteor);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

  bool hasSpeedControl() const override { return true; }
  void setSpeed(int speed) override { 
    currentSpeed = speed;
    delayTime = map(speed, 1, 100, 100, 10);
  }
  int getSpeed() const override { return currentSpeed; }
  int getDefaultSpeed() const override { return 50; }

private:
  int currentSpeed = 50;
  int delayTime = 50;
};
