// copyright https://elektro.turanis.de/html/prj104/index.html
#include "plugins/BreakoutPlugin.h"

void BreakoutPlugin::initGame()
{
  Screen.clear();

  this->ballDelay = this->BALL_DELAY_MAX; // This can still be used to adjust ball speed dynamically
  this->score = 0;
  this->level = 0;
  newLevel();
}

void BreakoutPlugin::initBricks()
{
  this->destroyedBricks = 0;
  for (byte i = 0; i < this->BRICK_AMOUNT; i++)
  {
    this->bricks[i].x = i % this->X_MAX;
    this->bricks[i].y = i / this->X_MAX;
    Screen.setPixelAtIndex(this->bricks[i].y * this->X_MAX + this->bricks[i].x, this->LED_TYPE_ON);
    delay(25);
  }
}

void BreakoutPlugin::newLevel()
{
  udp.begin(UDP_PORT);
  this->controlMode = CONTROL_AUTO;
  this->lastUdpPacketTime = 0;
  this->lastGameTick = millis();
  this->lastAutoPaddleMove = millis();
  Serial.println("Breakout: Started in AUTO mode. Listening for controller...");

  this->initBricks();
  for (byte i = 0; i < this->PADDLE_WIDTH; i++)
  {
    this->paddle[i].x = (this->X_MAX / 2) - (this->PADDLE_WIDTH / 2) + i;
    this->paddle[i].y = this->Y_MAX - 1;
    Screen.setPixelAtIndex(this->paddle[i].y * this->X_MAX + this->paddle[i].x, this->LED_TYPE_ON);
  }
  this->ball.x = this->paddle[1].x;
  this->ball.y = this->paddle[1].y - 1;

  Screen.setPixelAtIndex(ball.y * this->X_MAX + ball.x, this->LED_TYPE_ON);
  this->ballMovement[0] = 1;
  this->ballMovement[1] = -1;
  
  this->level++;
  this->gameState = this->GAME_STATE_RUNNING;
}

void BreakoutPlugin::updateBall()
{

  Screen.setPixelAtIndex(this->ball.y * this->X_MAX + this->ball.x, this->LED_TYPE_OFF);

  if (this->ballMovement[1] == 1)
  {
    if (this->ball.y == (this->Y_MAX - 1))
    {
      this->end();
      return;
    }
    this->checkPaddleCollision();
  }

  for (byte i = 0; i < this->BRICK_AMOUNT; i++)
  {
    if (this->bricks[i].x == (this->ball.x + this->ballMovement[0]) && 
        this->bricks[i].y == (this->ball.y + this->ballMovement[1]))
    {
      this->hitBrick(i);
      this->ballMovement[1] *= -1;
      break;
    }
  }

  if (this->destroyedBricks >= this->BRICK_AMOUNT)
  {
    this->gameState = this->GAME_STATE_LEVEL;
    return;
  }

  if (this->ball.x <= 0 || this->ball.x >= (this->X_MAX - 1))
  {
    this->ballMovement[0] *= -1;
  }
  if (this->ball.y <= 0)
  {
    this->ballMovement[1] *= -1;
  }

  this->ball.x += this->ballMovement[0];
  this->ball.y += this->ballMovement[1];

  Screen.setPixelAtIndex(this->ball.y * this->X_MAX + this->ball.x, this->LED_TYPE_ON);
}

void BreakoutPlugin::hitBrick(byte i)
{
  Screen.setPixelAtIndex(this->bricks[i].y * this->X_MAX + this->bricks[i].x, this->LED_TYPE_OFF);
  this->bricks[i].x = 255;
  this->bricks[i].y = 255;
  this->score++;
  this->destroyedBricks++;
  if (this->ballDelay > this->BALL_DELAY_MIN)
  {
    this->ballDelay -= this->BALL_DELAY_STEP;
  }
}

void BreakoutPlugin::checkPaddleCollision()
{
  if ((this->paddle[0].y - 1) != this->ball.y) return;
  
  if (this->ballMovement[0] == 1 && (this->paddle[0].x - 1) == this->ball.x ||
      this->ballMovement[0] == -1 && (this->paddle[this->PADDLE_WIDTH - 1].x + 1) == this->ball.x) {
    this->ballMovement[0] *= -1;
    this->ballMovement[1] *= -1;
    return;
  }
  if (paddle[this->PADDLE_WIDTH / 2].x == this->ball.x) {
    this->ballMovement[0] = 0;
    this->ballMovement[1] *= -1;
    return;
  }
  for (byte i = 0; i < this->PADDLE_WIDTH; i++) {
    if (this->paddle[i].x == this->ball.x) {
      this->ballMovement[1] *= -1;
      if (random(2) == 0) this->ballMovement[0] = 1;
      else this->ballMovement[0] = -1;
      break;
    }
  }
}

void BreakoutPlugin::movePaddle(int direction)
{
  int newPaddlePosition = this->paddle[0].x + direction;
  if (newPaddlePosition >= 0 && newPaddlePosition + this->PADDLE_WIDTH <= this->X_MAX) {
    for (byte i = 0; i < this->PADDLE_WIDTH; i++) {
      Screen.setPixelAtIndex(this->paddle[i].y * this->X_MAX + this->paddle[i].x, this->LED_TYPE_OFF);
    }
    for (byte i = 0; i < this->PADDLE_WIDTH; i++) {
      this->paddle[i].x += direction;
    }
    for (byte i = 0; i < this->PADDLE_WIDTH; i++) {
      Screen.setPixelAtIndex(this->paddle[i].y * this->X_MAX + this->paddle[i].x, this->LED_TYPE_ON);
    }
  }
}

void BreakoutPlugin::autoUpdatePaddle()
{
  static int moveDirection = 1;
  int newPaddlePosition = this->paddle[0].x + moveDirection;
  if (newPaddlePosition < 0 || newPaddlePosition + this->PADDLE_WIDTH > this->X_MAX) {
    moveDirection *= -1;
  }
  movePaddle(moveDirection);
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
        Serial.println("Controller detected! Switching to MANUAL mode.");
      }
      if (strcmp(packetBuffer, "LEFT") == 0) movePaddle(-1);
      else if (strcmp(packetBuffer, "RIGHT") == 0) movePaddle(1);
    }
  }

  if (controlMode == CONTROL_MANUAL && millis() - lastUdpPacketTime > UDP_TIMEOUT && lastUdpPacketTime != 0) {
    controlMode = CONTROL_AUTO;
    lastUdpPacketTime = 0;
    Serial.println("Controller timed out. Switching back to AUTO mode.");
  }
}

void BreakoutPlugin::end()
{
  this->gameState = this->GAME_STATE_END;
  udp.stop();
  Screen.setPixelAtIndex(this->ball.y * this->X_MAX + this->ball.x, this->LED_TYPE_ON);
}

void BreakoutPlugin::setup()
{
  this->gameState = this->GAME_STATE_END;
}

// Final, non-blocking loop
void BreakoutPlugin::loop()
{
  switch (this->gameState)
  {
    case this->GAME_STATE_LEVEL:
      this->newLevel();
      return;
    case this->GAME_STATE_END:
      this->initGame();
      return;
    default:
      break;
  }

  // Always check for network input for instant response
  checkForUdp(); 

  // --- THIS IS THE FIX ---
  // Use the 'ballDelay' variable for the ball's timer.
  if (millis() - lastGameTick > this->ballDelay) 
  {
    lastGameTick = millis();
    this->updateBall();
  }

  // Use a separate, slower timer for the auto-paddle
  if (controlMode == CONTROL_AUTO) 
  {
    if (millis() - lastAutoPaddleMove > AUTO_PADDLE_DELAY) 
    {
        lastAutoPaddleMove = millis();
        this->autoUpdatePaddle();
    }
  }
}

const char *BreakoutPlugin::getName() const
{
  return "Breakout";
}