#pragma once

#include "PluginManager.h"
#include <WiFiUdp.h>

class BreakoutPlugin : public Plugin
{
private:
  // --- Game Constants ---
  static const uint8_t X_MAX = 16;
  static const uint8_t Y_MAX = 16;
  static const uint8_t BRICK_AMOUNT = X_MAX * 4;
  static const uint8_t BALL_DELAY_MAX = 200;
  static const uint8_t BALL_DELAY_MIN = 100;
  static const uint8_t BALL_DELAY_STEP = 5;
  static const uint8_t PADDLE_WIDTH = 5;

  // --- Game States ---
  enum GameState { STATE_RUNNING, STATE_GAME_OVER, STATE_LEVEL };
  GameState gameState;

  enum ControlMode { CONTROL_AUTO, CONTROL_MANUAL };
  ControlMode controlMode;

  // --- AI State & Target ---
  int aiTargetX;
  bool wasLastHitCenter; // NEW: The AI's "memory"

  // --- Game Data ---
  struct Coords { unsigned char x; unsigned char y; };
  Coords paddle[PADDLE_WIDTH];
  Coords bricks[BRICK_AMOUNT];
  Coords ball;
  int ballMovement[2];
  uint8_t ballDelay;
  unsigned char level;
  unsigned char destroyedBricks;

  // --- Non-Blocking Timers & AI Tuning ---
  unsigned long lastGameTick;
  unsigned long lastAutoPlayMove;
  static const unsigned int AUTO_PLAY_DELAY = 120;
  static const uint8_t AI_NERF_FACTOR = 1;
  static const uint8_t AI_TARGET_TOLERANCE = 1;

  // --- Network Control ---
  WiFiUDP udp;
  static const unsigned int UDP_PORT = 12345;
  static const unsigned long UDP_TIMEOUT = 5000;
  unsigned long lastUdpPacketTime;
  char packetBuffer[255];

  // --- Core Functions ---
  void initGame();
  void newLevel();
  void gameOver();
  void updateBall();
  void hitBrick(unsigned char i);
  void checkPaddleCollision();
  void movePaddle(int direction);
  void checkForUdp();
  void autoPlayMove();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};