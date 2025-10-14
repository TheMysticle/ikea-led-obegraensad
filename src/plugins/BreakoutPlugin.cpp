#include "plugins/BreakoutPlugin.h"

void BreakoutPlugin::setup() {
  gameState = STATE_GAME_OVER;
}

const char *BreakoutPlugin::getName() const {
  return "Breakout";
}

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
  aiTargetX = X_MAX / 2; // Start by targeting the center
  hitsSinceBrickBreak = 0;
  bricksDestroyedAtLastHit = 0;
  wasLastHitCenter = false;
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
  ball.x = paddle[PADDLE_WIDTH / 2].x;
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

void BreakoutPlugin::autoPlayMove() {
    int predictedBallX = ball.x;
    int simY = ball.y;
    int simMoveX = ballMovement[0];
    int simMoveY = ballMovement[1];

    const int paddleRow = paddle[0].y;
    const int maxSteps = (X_MAX + Y_MAX) * 4;
    int steps = 0;

    // --- Simulate ball path ---
    while (simY < paddleRow - 1 && steps++ < maxSteps) {
        predictedBallX += simMoveX;
        simY += simMoveY;

        if (predictedBallX < 0) {
            predictedBallX = 0;
            simMoveX = abs(simMoveX);
        } else if (predictedBallX > X_MAX - 1) {
            predictedBallX = X_MAX - 1;
            simMoveX = -abs(simMoveX);
        }

        if (simY <= 0) {
            simY = 0;
            simMoveY = 1;
        }

        for (byte i = 0; i < BRICK_AMOUNT; i++) {
            if (bricks[i].x == 255) continue;
            if (bricks[i].x == predictedBallX && bricks[i].y == simY) {
                simMoveY = -simMoveY;
                simY += simMoveY;
                break;
            }
        }

        #if defined(ESP8266) || defined(ESP32)
        yield();
        #endif
    }

    if (steps >= maxSteps) predictedBallX = X_MAX / 2;

    // --- Brick bias ---
    int targetBrickX = -1;
    int minBrickY = Y_MAX;
    for (byte i = 0; i < BRICK_AMOUNT; i++) {
        if (bricks[i].x == 255) continue;
        if (bricks[i].y < minBrickY) {
            minBrickY = bricks[i].y;
            targetBrickX = bricks[i].x;
        }
    }

    int idealTargetX = predictedBallX;
    if (targetBrickX != -1 && abs(targetBrickX - predictedBallX) <= PADDLE_WIDTH / 2) {
        idealTargetX = targetBrickX;
    }

    // --- Smart edge handling ---
    const int minPaddleCenter = PADDLE_WIDTH / 2;
    const int maxPaddleCenter = X_MAX - 1 - (PADDLE_WIDTH / 2);
    int paddleCenter = paddle[PADDLE_WIDTH / 2].x;

    int safeTargetX = idealTargetX;

    // Only commit to corners if the ball will *actually* hit near the wall
    if (predictedBallX <= minPaddleCenter + 1) {
        safeTargetX = minPaddleCenter;  // fully left
    } else if (predictedBallX >= maxPaddleCenter - 1) {
        safeTargetX = maxPaddleCenter;  // fully right
    } else {
        // Otherwise, slightly bias toward center to avoid hanging near corners
        const int centerBias = (X_MAX / 2 - safeTargetX) / 4; // 25% pull to center
        safeTargetX += centerBias;
    }

    safeTargetX = constrain(safeTargetX, minPaddleCenter, maxPaddleCenter);

    // --- Movement logic ---
    const int MOVE_THRESHOLD = 0;
    if (paddleCenter < safeTargetX - MOVE_THRESHOLD)
        movePaddle(1);
    else if (paddleCenter > safeTargetX + MOVE_THRESHOLD)
        movePaddle(-1);
}

void BreakoutPlugin::updateBall() {
    // erase current ball
    Screen.setPixel(ball.x, ball.y, 0);

    // Compute next position
    int nextX = ball.x + ballMovement[0];
    int nextY = ball.y + ballMovement[1];

    // --- Wall collisions ---
    if (nextX < 0) {
        nextX = 0;
        ballMovement[0] = abs(ballMovement[0]);
    } else if (nextX > X_MAX - 1) {
        nextX = X_MAX - 1;
        ballMovement[0] = -abs(ballMovement[0]);
    }
    if (nextY < 0) {
        nextY = 0;
        ballMovement[1] = abs(ballMovement[1]);
    }

    // --- Paddle collision ---
    const int paddleRow = paddle[0].y;
    bool hitPaddle = false;

    // Check if ball is about to hit the paddle this frame
    if (ball.y < paddleRow && nextY >= paddleRow) {
        for (byte i = 0; i < PADDLE_WIDTH; i++) {
            if (paddle[i].x == nextX) {
                hitPaddle = true;

                // Clamp ball on top of paddle
                nextY = paddleRow - 1;

                // Update AI memory
                if (destroyedBricks == bricksDestroyedAtLastHit) hitsSinceBrickBreak++;
                else hitsSinceBrickBreak = 0;
                bricksDestroyedAtLastHit = destroyedBricks;
                wasLastHitCenter = (i == PADDLE_WIDTH / 2);

                // Bounce logic (vertical invert + horizontal adjustment)
                ballMovement[1] *= -1;
                if (i < PADDLE_WIDTH / 2) ballMovement[0] = -1;
                else if (i > PADDLE_WIDTH / 2) ballMovement[0] = 1;
                else {
                    int r = random(100);
                    if (r < 30) ballMovement[0] = -1;
                    else if (r < 60) ballMovement[0] = 1;
                    else ballMovement[0] = 0;
                }

                // After bounce, next position is computed based on updated movement
                nextX = ball.x + ballMovement[0];
                nextY = ball.y + ballMovement[1];
                nextX = constrain(nextX, 0, X_MAX - 1);
                break;
            }
        }
    }

    // Ball passed paddle without hitting -> game over
    if (nextY >= paddleRow && !hitPaddle) {
        gameOver();
        return;
    }

    // --- Brick collision ---
    for (byte i = 0; i < BRICK_AMOUNT; i++) {
        if (bricks[i].x == 255) continue;

        // Simple robust collision: if ball moves into or across brick
        if ((nextX == bricks[i].x && nextY == bricks[i].y) ||
            (ball.x == bricks[i].x && nextY == bricks[i].y) ||
            (nextX == bricks[i].x && ball.y == bricks[i].y)) {

            hitBrick(i);

            // Reflect vertically immediately
            ballMovement[1] *= -1;
            nextY = ball.y + ballMovement[1];
            nextX = ball.x + ballMovement[0];
            nextX = constrain(nextX, 0, X_MAX - 1);
            break;
        }
    }

    // --- Level complete check ---
    if (destroyedBricks >= BRICK_AMOUNT) {
        destroyedBricks = BRICK_AMOUNT; // clamp
        gameState = STATE_LEVEL;
        return;
    }

    // --- Apply new position ---
    ball.x = constrain(nextX, 0, X_MAX - 1);
    ball.y = constrain(nextY, 0, Y_MAX - 1);
    Screen.setPixel(ball.x, ball.y, 1);
}

void BreakoutPlugin::hitBrick(byte i) {
  Screen.setPixel(bricks[i].x, bricks[i].y, 0);
  bricks[i].x = 255; // Move off-screen
  bricks[i].y = 255; // fully mark destroyed
  destroyedBricks++;
  if (ballDelay > BALL_DELAY_MIN) ballDelay -= BALL_DELAY_STEP;
}

void BreakoutPlugin::checkPaddleCollision() {
    if (ball.y != paddle[0].y - 1 || ballMovement[1] < 0) return;

    for (byte i = 0; i < PADDLE_WIDTH; i++) {
        if (paddle[i].x == ball.x) {
            // --- UPDATE AI PROGRESS MEMORY ---
            if (destroyedBricks == bricksDestroyedAtLastHit) {
                hitsSinceBrickBreak++;
            } else {
                hitsSinceBrickBreak = 0;
            }
            bricksDestroyedAtLastHit = destroyedBricks;
            wasLastHitCenter = (i == PADDLE_WIDTH / 2);

            // Bounce logic
            ballMovement[1] *= -1;
            if (i < PADDLE_WIDTH / 2) {
                ballMovement[0] = -1;
            } else if (i > PADDLE_WIDTH / 2) {
                ballMovement[0] = 1;
            } else {
                // Center hit: 40% straight up, 30% left, 30% right
                int r = random(100);
                if (r < 30) {
                    ballMovement[0] = -1; // Diagonal left
                } else if (r < 60) {
                    ballMovement[0] = 1;  // Diagonal right
                } else {
                    ballMovement[0] = 0;  // Straight up
                }
            }
            return;
        }
    }
}

void BreakoutPlugin::movePaddle(int direction) {
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
    lastUdpPacketTime = 0;
    Serial.println("Controller timed out. Switching Breakout back to AUTO mode.");
  }
}

void BreakoutPlugin::loop() {
  switch (gameState) {
    case STATE_GAME_OVER: initGame(); return;
    case STATE_LEVEL: newLevel(); return;
    case STATE_RUNNING: break; // Continue
  }

  checkForUdp();

  if (millis() - lastGameTick > ballDelay) {
    lastGameTick = millis();
    updateBall();
  }

  if (controlMode == CONTROL_AUTO) {
    if (millis() - lastAutoPlayMove > AUTO_PLAY_DELAY) {
      lastAutoPlayMove = millis();
      autoPlayMove();
    }
  }
}