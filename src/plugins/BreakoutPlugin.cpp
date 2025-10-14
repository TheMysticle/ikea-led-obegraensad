#include "plugins/BreakoutPlugin.h"

void BreakoutPlugin::setup() { gameState = STATE_GAME_OVER; }
const char *BreakoutPlugin::getName() const { return "Breakout"; }

void BreakoutPlugin::initGame() {
  Screen.clear();
  ballDelay = BALL_DELAY_MAX;
  level = 0;
  newLevel();
}

void BreakoutPlugin::newLevel() {
  gameState = STATE_RUNNING;
  udp.begin(UDP_PORT);
  controlMode = CONTROL_AUTO;
  lastUdpPacketTime = 0;
  lastGameTick = millis();
  lastAutoPlayMove = millis();
  aiTargetX = X_MAX / 2;
  hitsSinceBrickBreak = 0;
  bricksDestroyedAtLastHit = 0;
  lastPaddleDirection = 0;
  Serial.println("Breakout: Started in smart AUTO mode. Listening for controller...");

  // Init Bricks
  destroyedBricks = 0;
  for (byte i = 0; i < BRICK_AMOUNT; i++) {
    bricks[i].x = i % X_MAX;
    bricks[i].y = i / X_MAX;
    Screen.setPixel(bricks[i].x, bricks[i].y, 1);
  }
  
  // Init Paddle
  for (byte i = 0; i < PADDLE_WIDTH; i++) {
    paddle[i].x = (X_MAX / 2) - (PADDLE_WIDTH / 2) + i;
    paddle[i].y = Y_MAX - 1;
    Screen.setPixel(paddle[i].x, paddle[i].y, 1);
  }

  // Init Ball
  ball.x = paddle[PADDLE_WIDTH / 2].x + random(-1, 2);
  ball.y = paddle[0].y - 1;
  ballMovement[0] = (random(2) == 0) ? 1 : -1;
  ballMovement[1] = -1;
  Screen.setPixel(ball.x, ball.y, 1);
  level++;
}

void BreakoutPlugin::gameOver() {
  udp.stop();
  gameState = STATE_GAME_OVER;
  Screen.scrollText("GAME OVER", 60);
  delay(2000);
}

// NEW: Function to display the victory message
void BreakoutPlugin::levelComplete() {
  udp.stop();
  gameState = STATE_VICTORY;
  Screen.scrollText("YOU WIN", 60);
  delay(2000);
}

void BreakoutPlugin::autoPlayMove() {
  if (ballMovement[1] > 0) {
    int predictedBallX = ball.x;
    int simY = ball.y;
    int simMoveX = ballMovement[0];
    while (simY < paddle[0].y - 1) {
      predictedBallX += simMoveX;
      simY++;
      if (predictedBallX <= 0 || predictedBallX >= X_MAX - 1) simMoveX *= -1;
    }

    if (hitsSinceBrickBreak >= STALE_RALLY_THRESHOLD) {
      bool verticalStrikeFound = false;
      for (int i = 0; i < BRICK_AMOUNT; i++) {
        if (bricks[i].x != 255 && bricks[i].x == predictedBallX) {
          aiTargetX = predictedBallX;
          verticalStrikeFound = true;
          break;
        }
      }
      if (!verticalStrikeFound) {
        if (random(4) == 0) {
          int targetForLeftHit = predictedBallX + (PADDLE_WIDTH / 2);
          int targetForRightHit = predictedBallX - (PADDLE_WIDTH / 2);
          int paddleCenter = paddle[PADDLE_WIDTH / 2].x;
          aiTargetX = (abs(targetForLeftHit - paddleCenter) > abs(targetForRightHit - paddleCenter)) ? targetForLeftHit : targetForRightHit;
        } else {
          aiTargetX = predictedBallX;
        }
      }
    } else {
      int targetForLeftHit = predictedBallX + (PADDLE_WIDTH / 2);
      int targetForRightHit = predictedBallX - (PADDLE_WIDTH / 2);
      int paddleCenter = paddle[PADDLE_WIDTH / 2].x;
      aiTargetX = (abs(targetForLeftHit - paddleCenter) < abs(targetForRightHit - paddleCenter)) ? targetForLeftHit : targetForRightHit;
    }
  } else {
    aiTargetX = X_MAX / 2;
  }

  const int minPaddleCenter = PADDLE_WIDTH / 2;
  const int maxPaddleCenter = X_MAX - 1 - (PADDLE_WIDTH / 2);
  aiTargetX = constrain(aiTargetX, minPaddleCenter, maxPaddleCenter);

  int paddleCenter = paddle[PADDLE_WIDTH / 2].x;
  if (paddleCenter < aiTargetX) movePaddle(1);
  else if (paddleCenter > aiTargetX) movePaddle(-1);
}

void BreakoutPlugin::updateBall() {
  Screen.setPixel(ball.x, ball.y, 0);
  if (ball.y >= Y_MAX - 1) { gameOver(); return; }

  bool brickWasHit = false;
  int nextBallX = ball.x + ballMovement[0];
  int nextBallY = ball.y + ballMovement[1];
  for (byte i = 0; i < BRICK_AMOUNT; i++) {
    if (bricks[i].x != 255 && bricks[i].x == nextBallX && bricks[i].y == nextBallY) {
      hitBrick(i);
      ballMovement[1] *= -1;
      brickWasHit = true;
      break;
    }
  }
  if (!brickWasHit) {
    if (nextBallY < 0) ballMovement[1] *= -1;
    if (nextBallX < 0 || nextBallX >= X_MAX) ballMovement[0] *= -1;
  }
  checkPaddleCollision();

  // CHANGED: This is the trigger for the victory screen
  if (destroyedBricks >= BRICK_AMOUNT) {
    levelComplete();
    return;
  }
  
  ball.x += ballMovement[0];
  ball.y += ballMovement[1];
  Screen.setPixel(ball.x, ball.y, 1);
}

void BreakoutPlugin::hitBrick(byte i) {
  Screen.setPixel(bricks[i].x, bricks[i].y, 0);
  bricks[i].x = 255;
  destroyedBricks++;
  if (ballDelay > BALL_DELAY_MIN) ballDelay -= BALL_DELAY_STEP;
}

void BreakoutPlugin::checkPaddleCollision() {
    if (ball.y != paddle[0].y - 1 || ballMovement[1] < 0) return;
    for (byte i = 0; i < PADDLE_WIDTH; i++) {
        if (paddle[i].x == ball.x) {
            if (destroyedBricks == bricksDestroyedAtLastHit) hitsSinceBrickBreak++;
            else hitsSinceBrickBreak = 0;
            bricksDestroyedAtLastHit = destroyedBricks;

            ballMovement[1] *= -1;
            int paddleCenterIndex = PADDLE_WIDTH / 2;
            if (i < paddleCenterIndex) {
                ballMovement[0] = -1;
                if (lastPaddleDirection != 0) ball.x += lastPaddleDirection;
            } else if (i > paddleCenterIndex) {
                ballMovement[0] = 1;
                if (lastPaddleDirection != 0) ball.x += lastPaddleDirection;
            } else {
                bool columnHasBricks = false;
                for (int j = 0; j < BRICK_AMOUNT; j++) {
                    if (bricks[j].x != 255 && bricks[j].x == ball.x) { columnHasBricks = true; break; }
                }
                if (columnHasBricks) ballMovement[0] = 0;
                else ballMovement[0] = (random(2) == 0) ? 1 : -1;
            }
            ball.x = constrain(ball.x, 0, X_MAX - 1);
            return;
        }
    }
}

void BreakoutPlugin::movePaddle(int direction) {
  lastPaddleDirection = direction;
  int newPos = paddle[0].x + direction;
  if (newPos >= 0 && (newPos + PADDLE_WIDTH) <= X_MAX) {
    for(byte i=0; i<PADDLE_WIDTH; i++) Screen.setPixel(paddle[i].x, paddle[i].y, 0);
    for(byte i=0; i<PADDLE_WIDTH; i++) paddle[i].x += direction;
    for(byte i=0; i<PADDLE_WIDTH; i++) Screen.setPixel(paddle[i].x, paddle[i].y, 1);
  }
}

void BreakoutPlugin::checkForUdp() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    if (strcmp(packetBuffer, "LEFT") == 0 || strcmp(packetBuffer, "RIGHT") == 0 || strcmp(packetBuffer, "PING") == 0) {
      lastUdpPacketTime = millis();
      if (controlMode == CONTROL_AUTO) {
        controlMode = CONTROL_MANUAL;
        Serial.println("Controller detected! Switching Breakout to MANUAL mode.");
      }
      if (strcmp(packetBuffer, "LEFT") == 0) movePaddle(-1);
      else if (strcmp(packetBuffer, "RIGHT") == 0) movePaddle(1);
    }
  }
  if (controlMode == CONTROL_MANUAL && millis() - lastUdpPacketTime > UDP_TIMEOUT && lastUdpPacketTime != 0) {
    controlMode = CONTROL_AUTO;
    Serial.println("Controller timed out. Switching Breakout back to AUTO mode.");
  }
}

void BreakoutPlugin::loop() {
  switch (gameState) {
    case STATE_GAME_OVER: initGame(); return;
    case STATE_LEVEL: newLevel(); return;
    // NEW: Handle the victory state
    case STATE_VICTORY: gameState = STATE_LEVEL; return;
    case STATE_RUNNING: break;
  }
  checkForUdp();
  if (millis() - lastGameTick > ballDelay) {
    lastGameTick = millis();
    updateBall();
  }
  if (controlMode == CONTROL_AUTO) {
    if (millis() - lastAutoPlayMove > AUTO_PLAY_DELAY) {
      lastAutoPlayMove = millis();
      lastPaddleDirection = 0;
      autoPlayMove();
    }
  }
}