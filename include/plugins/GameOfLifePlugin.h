#pragma once

#include "PluginManager.h"
#include "timing.h"

class GameOfLifePlugin : public Plugin
{
private:
  static constexpr uint8_t STATE_RUNNING = 1;
  static constexpr uint8_t STATE_END = 2;
  static constexpr uint8_t STATE_INIT = 3;
  static constexpr uint8_t STATE_END_DELAY = 4;
  uint8_t state;
  uint8_t previous2[ROWS * COLS];
  uint8_t previous[ROWS * COLS];
  uint8_t buffer[ROWS * COLS];
  uint8_t updateCell(int row, int col);
  uint8_t countNeighbours(int row, int col);
  void next();
  void init();
  void show();
  uint16_t gol_delay = 150;
  int currentSpeed = 50;

  NonBlockingDelay updateTimer;
  NonBlockingDelay initTimer;
  uint8_t initStep = 0;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
  
  bool hasSpeedControl() const override { return true; }
  void setSpeed(int speed) override { 
    currentSpeed = speed;
    gol_delay = map(speed, 1, 100, 300, 10);
  }
  int getSpeed() const override { return currentSpeed; }
  int getDefaultSpeed() const override { return 50; }
};
