#include "plugins/FroggerPlugin.h"

void FroggerPlugin::setup() {
    gameState = STATE_GAME_OVER;
}

const char* FroggerPlugin::getName() const {
    return "Frogger";
}

void FroggerPlugin::newGame() {
    udp.begin(UDP_PORT);
    controlMode = CONTROL_AUTO;
    lastUdpPacketTime = 0;
    lastAutoPlayMove = millis();
    Serial.println("Frogger: Started in AUTO mode. Listening for controller...");

    lives = 3;
    score = 0;
    for (int i=0; i<HOME_COUNT; i++) homesFilled[i] = false;
    
    // --- RE-BALANCED WORLD LAYOUT ---
    // Speeds are higher (slower movement) and gaps are larger.
    // Home bay row
    lanes[0] = { SAFE_GRASS, 0, 0, 0, {0} };
    lanes[1] = { SAFE_GRASS, 0, 0, 0, {0} };
    // River section (slower logs, more space)
    lanes[2] = { WATER,  1, 300, 0, {0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,0} }; // Slow logs
    lanes[3] = { WATER, -1, 250, 0, {1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0} }; // Medium logs
    lanes[4] = { WATER,  1, 400, 0, {0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0} }; // Very slow, long log
    lanes[5] = { WATER, -1, 220, 0, {0,0,1,1,0,0,0,0,1,1,0,0,0,0,1,1} }; // Fast turtles
    lanes[6] = { WATER,  1, 350, 0, {1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0} }; // Slow logs
    // Median grass
    lanes[7] = { SAFE_GRASS, 0, 0, 0, {0} };
    // Road section (slower cars, more space)
    lanes[8] = { ROAD, -1, 280, 0, {0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0} }; // Slow cars
    lanes[9] = { ROAD,  1, 350, 0, {0,0,0,1,1,1,0,0,0,0,0,1,1,1,0,0} }; // "Trucks"
    lanes[10]= { ROAD, -1, 200, 0, {0,1,1,0,0,0,0,1,1,0,0,0,0,1,1,0} }; // Fast racecars
    lanes[11]= { ROAD,  1, 300, 0, {0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,0} };
    lanes[12]= { ROAD, -1, 250, 0, {0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0} };
    // Starting grass
    lanes[13] = { SAFE_GRASS, 0, 0, 0, {0} };
    lanes[14] = { SAFE_GRASS, 0, 0, 0, {0} };
    lanes[15] = { SAFE_GRASS, 0, 0, 0, {0} };
    
    resetLevel();
}

void FroggerPlugin::resetLevel() {
    playerX = SCREEN_WIDTH / 2;
    playerY = 14;
    gameState = STATE_PLAYING;
    drawWorld();
}

void FroggerPlugin::playerDie() {
    lives--;
    if (lives == 0) {
        gameState = STATE_GAME_OVER;
        Screen.scrollText("GAME OVER", 60);
        delay(2000);
        udp.stop();
    } else {
        resetLevel();
    }
}

void FroggerPlugin::updateLanes() {
    for (int i = 0; i < LANE_COUNT; i++) {
        if (lanes[i].speed > 0 && millis() - lanes[i].lastMoveTime > lanes[i].speed) {
            lanes[i].lastMoveTime = millis();
            if (playerY == i && lanes[i].type == WATER) playerX += lanes[i].direction;
            if (lanes[i].direction == 1) {
                uint8_t last_pixel = lanes[i].data[SCREEN_WIDTH-1];
                memmove(&lanes[i].data[1], &lanes[i].data[0], SCREEN_WIDTH-1);
                lanes[i].data[0] = last_pixel;
            } else {
                uint8_t first_pixel = lanes[i].data[0];
                memmove(&lanes[i].data[0], &lanes[i].data[1], SCREEN_WIDTH-1);
                lanes[i].data[SCREEN_WIDTH-1] = first_pixel;
            }
        }
    }
}

void FroggerPlugin::checkPlayerState() {
    // Check for out of bounds
    if (playerX < 0 || playerX >= SCREEN_WIDTH) {
        playerDie();
        return;
    }

    Lane* currentLane = &lanes[playerY];
    bool isSafe = (currentLane->type == ROAD && currentLane->data[playerX] == 0) ||
                  (currentLane->type == WATER && currentLane->data[playerX] == 1) ||
                  (currentLane->type == SAFE_GRASS);

    if (!isSafe) {
        playerDie();
        return;
    }
    
    // Check for reaching a home bay
    if (playerY == 0) {
        bool homeFound = false;
        for (int i = 0; i < HOME_COUNT; i++) {
            if (playerX == homePositions[i] && !homesFilled[i]) {
                homesFilled[i] = true;
                score += 50;
                homeFound = true;

                // --- THIS IS THE FIX ---
                // Add a celebratory pause to give visual feedback.
                // We'll also quickly flash the successful frog.
                drawWorld(); // Draw the frog in the home bay
                delay(500);
                Screen.setPixel(playerX, playerY, 0); // Turn it off
                delay(250);
                Screen.setPixel(playerX, playerY, 1); // Turn it back on
                delay(250);
                // Total pause is 1 second.
                
                break;
            }
        }
        
        if (!homeFound) { // Hopped into a wall or a full bay
            playerDie();
        } else {
            // Check if all homes are filled
            bool allFull = true;
            for (int i = 0; i < HOME_COUNT; i++) if (!homesFilled[i]) allFull = false;
            
            if (allFull) {
                gameState = STATE_LEVEL_WON;
                Screen.scrollText("YOU WIN", 60);
                delay(2000);
            } else {
                resetLevel(); // Reset for next frog
            }
        }
    }
}

void FroggerPlugin::drawWorld() {
    Screen.clear();
    for (int y = 0; y < LANE_COUNT; y++) for (int x = 0; x < SCREEN_WIDTH; x++) if (lanes[y].data[x] == 1) Screen.setPixel(x, y, 1);
    for (int i = 0; i < HOME_COUNT; i++) Screen.setPixel(homePositions[i], 0, homesFilled[i] ? 1 : 0, homesFilled[i] ? 255 : 50);
    if (millis() % 400 < 200) Screen.setPixel(playerX, playerY, 1, 200);
}

void FroggerPlugin::autoPlayMove() {
    int bestMoveX = playerX, bestMoveY = playerY, bestScore = -1000;
    int dx[] = {0, -1, 1, 0, 0}, dy[] = {0, 0, 0, -1, 1};
    for (int i = 0; i < 5; i++) {
        int nextX = playerX + dx[i], nextY = playerY + dy[i], currentScore = 0;
        if (nextX < 0 || nextX >= SCREEN_WIDTH || nextY < 0 || nextY >= SCREEN_HEIGHT) continue;
        Lane* nextLane = &lanes[nextY];
        bool isSafe = false;
        currentScore += (playerY - nextY) * 10;
        int lookAheadX = (nextX - nextLane->direction + SCREEN_WIDTH) % SCREEN_WIDTH;
        if ((nextLane->type == ROAD && nextLane->data[nextX] == 0 && nextLane->data[lookAheadX] == 0) ||
            (nextLane->type == WATER && nextLane->data[nextX] == 1) || (nextLane->type == SAFE_GRASS)) isSafe = true;
        if (!isSafe) currentScore -= 100;
        if (nextY == 0) {
            bool validHome = false;
            for(int h=0; h<HOME_COUNT; h++) if(nextX == homePositions[h] && !homesFilled[h]) validHome = true;
            if (validHome) currentScore += 100; else currentScore -= 200;
        }
        if (currentScore > bestScore) { bestScore = currentScore; bestMoveX = nextX; bestMoveY = nextY; }
    }
    playerX = bestMoveX; playerY = bestMoveY;
}

void FroggerPlugin::checkForUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        lastUdpPacketTime = millis();
        if (controlMode == CONTROL_AUTO) { controlMode = CONTROL_MANUAL; Serial.println("Controller detected! Switching Frogger to MANUAL mode."); }
        int len = udp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;
        if (strcmp(packetBuffer, "LEFT") == 0 && playerX > 0) playerX--;
        else if (strcmp(packetBuffer, "RIGHT") == 0 && playerX < SCREEN_WIDTH - 1) playerX++;
        else if (strcmp(packetBuffer, "UP") == 0 && playerY > 0) playerY--;
        else if (strcmp(packetBuffer, "DOWN") == 0 && playerY < SCREEN_HEIGHT - 1) playerY++;
    }
    if (controlMode == CONTROL_MANUAL && millis() - lastUdpPacketTime > UDP_TIMEOUT && lastUdpPacketTime != 0) {
        controlMode = CONTROL_AUTO; lastUdpPacketTime = 0; Serial.println("Controller timed out. Switching Frogger back to AUTO mode.");
    }
}

void FroggerPlugin::loop() {
    if (gameState == STATE_GAME_OVER || gameState == STATE_LEVEL_WON) { newGame(); return; }
    checkForUdp();
    if (controlMode == CONTROL_AUTO) {
        if (millis() - lastAutoPlayMove > AUTO_PLAY_DELAY) {
            lastAutoPlayMove = millis();
            autoPlayMove();
        }
    }
    updateLanes();
    checkPlayerState();
    if (gameState == STATE_PLAYING) drawWorld();
    delay(20);
}