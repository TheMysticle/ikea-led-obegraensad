#pragma once

#include "PluginManager.h"
#include "timing.h"

class BubblesPlugin : public Plugin
{
private:
  struct Bubble
  {
    float x;
    float y;
    float radius;
    float speed;
    uint8_t brightness;
  };

  NonBlockingDelay timer;
  static constexpr uint8_t kBubbleCount = 6;
  Bubble bubbles[kBubbleCount];

  void resetBubble(Bubble &bubble);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

  bool hasSpeedControl() const override { return true; }
  void setSpeed(int speed) override { 
    currentSpeed = speed;
    delayTime = map(speed, 1, 100, 160, 10);
  }
  int getSpeed() const override { return currentSpeed; }
  int getDefaultSpeed() const override { return 50; }

private:
  int currentSpeed = 50;
  int delayTime = 80;
};
