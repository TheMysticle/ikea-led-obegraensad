#include "plugins/BreakoutPlugin.h"

// --- setup, getName, initGame are unchanged ---
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
  // REMOVED: aiHasMadeDecision is no longer used
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

// --- gameOver is unchanged ---
void BreakoutPlugin::gameOver() {
  udp.stop();
  gameState = STATE_GAME_OVER;
  Screen.scrollText("GAME OVER", 60);
  delay(2000);
}

void BreakoutPlugin::autoPlayMove() {
  // Only think if the ball is coming down
  if (ballMovement[1] > 0) {
    // --- STEP 1: PREDICT BALL'S PATH ---
    int predictedBallX = ball.x;
    int simY = ball.y;
    int simMoveX = ballMovement[0];
    while (simY < paddle[0].y - 1) {
      predictedBallX += simMoveX;
      simY++;
      if (predictedBallX <= 0 || predictedBallX >= X_MAX - 1) {
        simMoveX *= -1;
      }
    }

    // --- STEP 2: STRATEGIC DECISION ---
    if (hitsSinceBrickBreak >= STALE_RALLY_THRESHOLD) {
      // PATTERN BREAK: The current aggressive strategy is failing.
      // NEW TOP PRIORITY: Look for a vertical strike opportunity.
      bool verticalStrikeFound = false;
      for (int i = 0; i < BRICK_AMOUNT; i++) {
        // Is there a remaining brick in the predicted column?
        if (bricks[i].x != 255 && bricks[i].x == predictedBallX) {
          Serial.println("AI: Stale rally! Found a vertical strike target!");
          aiTargetX = predictedBallX; // This is the golden shot. Take it.
          verticalStrikeFound = true;
          break; // Found our target, no need to look further.
        }
      }

      // If no vertical target was found, fall back to the old pattern break.
      if (!verticalStrikeFound) {
        Serial.println("AI: Stale rally! No vertical target. Forcing random break...");
        if (random(4) == 0) { // 25% chance of a risky cross-court shot
          int targetForLeftHit = predictedBallX + (PADDLE_WIDTH / 2);
          int targetForRightHit = predictedBallX - (PADDLE_WIDTH / 2);
          int paddleCenter = paddle[PADDLE_WIDTH / 2].x;
          aiTargetX = (abs(targetForLeftHit - paddleCenter) > abs(targetForRightHit - paddleCenter)) ? targetForLeftHit : targetForRightHit;
        } else { // 75% chance of a safe (but different) center shot
          aiTargetX = predictedBallX;
        }
      }

    } else {
      // NORMAL AGGRESSIVE PLAY: Choose the most efficient edge-shot.
      int targetForLeftHit = predictedBallX + (PADDLE_WIDTH / 2);
      int targetForRightHit = predictedBallX - (PADDLE_WIDTH / 2);
      int paddleCenter = paddle[PADDLE_WIDTH / 2].x;
      aiTargetX = (abs(targetForLeftHit - paddleCenter) < abs(targetForRightHit - paddleCenter)) ? targetForLeftHit : targetForRightHit;
    }
  } else {
    // Defensively center while ball is moving away
    aiTargetX = X_MAX / 2;
  }

  // --- STEP 3: EXECUTE THE MOVE ---
  // The continuous re-evaluation prevents the AI from making suicidal moves.
  const int minPaddleCenter = PADDLE_WIDTH / 2;
  const int maxPaddleCenter = X_MAX - 1 - (PADDLE_WIDTH / 2);
  aiTargetX = constrain(aiTargetX, minPaddleCenter, maxPaddleCenter);

  int paddleCenter = paddle[PADDLE_WIDTH / 2].x;
  if (paddleCenter < aiTargetX) {
    movePaddle(1);
  } else if (paddleCenter > aiTargetX) {
    movePaddle(-1);
  }
}

// --- updateBall and hitBrick are unchanged ---
void BreakoutPlugin::updateBall() {
  Screen.setPixel(ball.x, ball.y, 0);

  if (ball.y >= Y_MAX - 1) {
    gameOver();
    return;
  }

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

  if (destroyedBricks >= BRICK_AMOUNT) {
    gameState = STATE_LEVEL;
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
            // Update counters
            if (destroyedBricks == bricksDestroyedAtLastHit) {
              hitsSinceBrickBreak++;
            } else {
              hitsSinceBrickBreak = 0;
            }
            bricksDestroyedAtLastHit = destroyedBricks;

            // --- BOUNCE LOGIC WITH PADDLE SPIN ---
            ballMovement[1] *= -1; // Always bounce up
            int paddleCenterIndex = PADDLE_WIDTH / 2;

            if (i < paddleCenterIndex) { // Left side hit
                ballMovement[0] = -1;
                // APPLY SPIN: If paddle was moving, give the ball an extra kick
                if (lastPaddleDirection != 0) ball.x += lastPaddleDirection;
            } else if (i > paddleCenterIndex) { // Right side hit
                ballMovement[0] = 1;
                // APPLY SPIN: If paddle was moving, give the ball an extra kick
                if (lastPaddleDirection != 0) ball.x += lastPaddleDirection;
            } else { // Center hit
                bool columnHasBricks = false;
                for (int j = 0; j < BRICK_AMOUNT; j++) {
                    if (bricks[j].x != 255 && bricks[j].x == ball.x) {
                        columnHasBricks = true;
                        break;
                    }
                }
                if (columnHasBricks) {
                    ballMovement[0] = 0; // Execute the strategic vertical strike
                } else {
                    ballMovement[0] = (random(2) == 0) ? 1 : -1; // Failsafe
                }
            }

            // Ensure the "spin" kick doesn't push the ball out of bounds
            ball.x = constrain(ball.x, 0, X_MAX - 1);
            return;
        }
    }
}

void BreakoutPlugin::movePaddle(int direction) {
  lastPaddleDirection = direction; // NEW: Record the direction of movement for this frame
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
      lastPaddleDirection = 0; // NEW: Reset paddle direction before the AI thinks
      autoPlayMove();
    }
  }
}