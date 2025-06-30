#pragma once

#include "PluginManager.h"
#include <WiFiUdp.h>

class BreakoutPlugin : public Plugin
{
private:
  // --- Game Constants ---
  static const uint8_t DEBOUNCE_TIME = 100;
  static const uint8_t X_MAX = 16;
  static const uint8_t Y_MAX = 16;
  static const uint8_t BRICK_AMOUNT = X_MAX * 4;
  static const uint8_t BALL_DELAY_MAX = 200;
  static const uint8_t BALL_DELAY_MIN = 100;
  static const uint8_t BALL_DELAY_STEP = 5;
  static const uint8_t PADDLE_WIDTH = 5;
  static const uint8_t DIRECTION_NONE = 0;
  static const uint8_t DIRECTION_LEFT = 1;
  static const uint8_t DIRECTION_RIGHT = 2;
  static const uint8_t LED_TYPE_OFF = 0;
  static const uint8_t LED_TYPE_ON = 1;
  static const uint8_t GAME_STATE_RUNNING = 1;
  static const uint8_t GAME_STATE_END = 2;
  static const uint8_t GAME_STATE_LEVEL = 3;

  // --- Network/Control Members ---
  enum ControlMode { CONTROL_AUTO, CONTROL_MANUAL };
  ControlMode controlMode;
  WiFiUDP udp;
  static const unsigned int UDP_PORT = 12345;
  static const unsigned long UDP_TIMEOUT = 5000;
  unsigned long lastUdpPacketTime;
  char packetBuffer[255];

  // --- Non-Blocking Timers ---
  unsigned long lastGameTick;
  static const unsigned int GAME_TICK_DELAY = 50; // Ball speed
  unsigned long lastAutoPaddleMove;
  static const unsigned int AUTO_PADDLE_DELAY = 150; // Auto-paddle speed

  struct Coords
  {
    unsigned char x;
    unsigned char y;
  };

  unsigned char gameState;
  unsigned char level;
  unsigned char destroyedBricks;
  Coords paddle[BreakoutPlugin::PADDLE_WIDTH];
  Coords bricks[BreakoutPlugin::BRICK_AMOUNT];
  Coords ball;

  int ballMovement[2];
  uint8_t ballDelay;
  uint8_t score;
  // 'lastBallUpdate' is now replaced by the more flexible lastGameTick
  // unsigned long lastBallUpdate = 0; 

  void resetLEDs();
  void initGame();
  void initBricks();
  void newLevel();
  void updateBall();
  void hitBrick(unsigned char i);
  void checkPaddleCollision();
  void autoUpdatePaddle();
  void movePaddle(int direction);
  void checkForUdp();
  void end();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};