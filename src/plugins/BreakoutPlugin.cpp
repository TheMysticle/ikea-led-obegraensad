// copyright https://elektro.turanis.de/html/prj104/index.html
#include "plugins/BreakoutPlugin.h"

void BreakoutPlugin::initGame()
{
  Screen.clear();

  this->ballDelay = this->BALL_DELAY_MAX;
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
    // CHANGE: Removed custom brightness of 50. Let it default to 255.
    Screen.setPixelAtIndex(this->bricks[i].y * this->X_MAX + this->bricks[i].x, this->LED_TYPE_ON); 

    delay(25);
  }
}

void BreakoutPlugin::newLevel()
{
  this->initBricks();
  for (byte i = 0; i < this->PADDLE_WIDTH; i++)
  {
    this->paddle[i].x = (this->X_MAX / 2) - (this->PADDLE_WIDTH / 2) + i;
    this->paddle[i].y = this->Y_MAX - 1;
    // CHANGE: Removed custom brightness of 50.
    Screen.setPixelAtIndex(this->paddle[i].y * this->X_MAX + this->paddle[i].x, this->LED_TYPE_ON);
  }
  this->ball.x = this->paddle[1].x;
  this->ball.y = this->paddle[1].y - 1;

  // CHANGE: Removed custom brightness of 128.
  Screen.setPixelAtIndex(ball.y * this->X_MAX + ball.x, this->LED_TYPE_ON);
  this->ballMovement[0] = 1;
  this->ballMovement[1] = -1;
  this->lastBallUpdate = 0;

  this->level++;
  this->gameState = this->GAME_STATE_RUNNING;
}

void BreakoutPlugin::updateBall()
{
  if ((millis() - this->lastBallUpdate) < this->ballDelay)
  {
    return;
  }
  this->lastBallUpdate = millis();
  // Erase the ball from its current position
  Screen.setPixelAtIndex(this->ball.y * this->X_MAX + this->ball.x, this->LED_TYPE_OFF);

  if (this->ballMovement[1] == 1)
  {
    // collision with bottom
    if (this->ball.y == (this->Y_MAX - 1))
    {
      this->end();
      return;
    }
    this->checkPaddleCollision();
  }

  // --- START OF FIX ---
  // Proactive collision detection with bricks
  for (byte i = 0; i < this->BRICK_AMOUNT; i++)
  {
    // Check if the NEXT position of the ball will hit a valid brick
    if (this->bricks[i].x == (this->ball.x + this->ballMovement[0]) && 
        this->bricks[i].y == (this->ball.y + this->ballMovement[1]))
    {
      this->hitBrick(i); // Destroy the brick
      this->ballMovement[1] *= -1; // Reverse ball's vertical direction
      break; // Only handle one brick collision per frame
    }
  }
  // --- END OF FIX ---

  if (this->destroyedBricks >= this->BRICK_AMOUNT)
  {
    this->gameState = this->GAME_STATE_LEVEL;
    return;
  }

  // collision detection with wall
  if (this->ball.x <= 0 || this->ball.x >= (this->X_MAX - 1))
  {
    this->ballMovement[0] *= -1;
  }
  if (this->ball.y <= 0)
  {
    this->ballMovement[1] *= -1;
  }

  // Now, update the ball's position using the (potentially updated) movement vector
  this->ball.x += this->ballMovement[0];
  this->ball.y += this->ballMovement[1];

  // And draw the ball at its new final position
  Screen.setPixelAtIndex(this->ball.y * this->X_MAX + this->ball.x, this->LED_TYPE_ON);
}

void BreakoutPlugin::hitBrick(byte i)
{
  // First, turn off the pixel at the brick's CURRENT location.
  Screen.setPixelAtIndex(this->bricks[i].y * this->X_MAX + this->bricks[i].x, this->LED_TYPE_OFF);

  // Now, mark the brick as destroyed. Using a value clearly off-screen.
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
  if ((this->paddle[0].y - 1) != this->ball.y)
  {
    return;
  }

  // reverse movement direction on the edges
  if (this->ballMovement[0] == 1 && (this->paddle[0].x - 1) == this->ball.x ||
      this->ballMovement[0] == -1 && (this->paddle[this->PADDLE_WIDTH - 1].x + 1) == this->ball.x)
  {
    this->ballMovement[0] *= -1;
    this->ballMovement[1] *= -1;

    return;
  }
  if (paddle[this->PADDLE_WIDTH / 2].x == this->ball.x)
  {
    this->ballMovement[0] = 0;
    this->ballMovement[1] *= -1;

    return;
  }

  for (byte i = 0; i < this->PADDLE_WIDTH; i++)
  {
    if (this->paddle[i].x == this->ball.x)
    {
      this->ballMovement[1] *= -1;
      if (random(2) == 0)
      {
        this->ballMovement[0] = 1;
      }
      else
      {
        this->ballMovement[0] = -1;
      }

      break;
    }
  }
}

void BreakoutPlugin::updatePaddle()
{
  static int moveDirection = 1;

  int newPaddlePosition = this->paddle[0].x + moveDirection;

  if (newPaddlePosition >= 0 && newPaddlePosition + this->PADDLE_WIDTH <= this->X_MAX)
  {
    for (byte i = 0; i < this->PADDLE_WIDTH; i++)
    {
      Screen.setPixelAtIndex(this->paddle[i].y * this->X_MAX + this->paddle[i].x, this->LED_TYPE_OFF);
    }
    for (byte i = 0; i < this->PADDLE_WIDTH; i++)
    {
      this->paddle[i].x += moveDirection;
    }
    for (byte i = 0; i < this->PADDLE_WIDTH; i++)
    {
      Screen.setPixelAtIndex(this->paddle[i].y * this->X_MAX + this->paddle[i].x, this->LED_TYPE_ON);
    }
  }
  else
  {
    moveDirection *= -1;
  }
}

void BreakoutPlugin::end()
{
  this->gameState = this->GAME_STATE_END;
  Screen.setPixelAtIndex(this->ball.y * this->X_MAX + this->ball.x, this->LED_TYPE_ON);
}

void BreakoutPlugin::setup()
{
  this->gameState = this->GAME_STATE_END;
}

void BreakoutPlugin::loop()
{
  switch (this->gameState)
  {
  case this->GAME_STATE_LEVEL:
    this->newLevel();
    break;
  case this->GAME_STATE_RUNNING:
    this->updateBall();
    this->updatePaddle();
    delay(random(100, 200));
    break;
  case this->GAME_STATE_END:
    this->initGame();
    break;
  }
}

const char *BreakoutPlugin::getName() const
{
  return "Breakout";
}