#include "plugins/SnakePlugin.h"

void SnakePlugin::setup() {
    gameState = STATE_GAME_OVER;
}

const char* SnakePlugin::getName() const {
    return "Snake";
}

void SnakePlugin::newGame() {
    Screen.clear();
    udp.begin(UDP_PORT);
    controlMode = CONTROL_AUTO;
    lastUdpPacketTime = 0;
    lastGameTick = millis();
    Serial.println("Snake: Started in AUTO mode. Listening for controller...");

    snakeBody.clear();
    snakeBody.push_back(SCREEN_WIDTH * (SCREEN_HEIGHT / 2) + 3);
    snakeBody.push_back(SCREEN_WIDTH * (SCREEN_HEIGHT / 2) + 2);
    snakeBody.push_back(SCREEN_WIDTH * (SCREEN_HEIGHT / 2) + 1);

    currentDirection = 1; // Start moving right
    aiPath.clear();
    aiPathIndex = 0;

    spawnDot();
    
    for (int segment : snakeBody) Screen.setPixelAtIndex(segment, LED_TYPE_ON);
    
    gameState = STATE_PLAYING;
}

void SnakePlugin::spawnDot() {
    bool dotIsValid;
    do {
        dotIsValid = true;
        dotPosition = random(0, MAX_PIXELS);
        for (int segment : snakeBody) {
            if (dotPosition == segment) {
                dotIsValid = false;
                break;
            }
        }
    } while (!dotIsValid);
    Screen.setPixelAtIndex(dotPosition, LED_TYPE_ON, 100);
}

void SnakePlugin::gameOver() {
    udp.stop();
    gameState = STATE_GAME_OVER;
    // Simple flashing game over animation
    for (int i = 0; i < 3; i++) {
        for (int segment : snakeBody) Screen.setPixelAtIndex(segment, LED_TYPE_OFF);
        delay(250);
        for (int segment : snakeBody) Screen.setPixelAtIndex(segment, LED_TYPE_ON);
        delay(250);
    }
}

bool SnakePlugin::isCollision(int headPos) {
    // We no longer check for wall collisions here.
    // The moveSnake function will handle wrapping.

    // Check for collision with the snake's own body.
    for (size_t i = 0; i < snakeBody.size() - 1; i++) {
        if (headPos == snakeBody[i]) {
            return true; // Body collision
        }
    }
    return false; // No collision
}

void SnakePlugin::moveSnake() {
    int head = snakeBody.front();
    int newHead = head;

    // Calculate the next theoretical position
    if (currentDirection == 0) newHead -= SCREEN_WIDTH;      // Up
    else if (currentDirection == 1) newHead += 1;          // Right
    else if (currentDirection == 2) newHead += SCREEN_WIDTH; // Down
    else if (currentDirection == 3) newHead -= 1;          // Left

    // --- SCREEN WRAP LOGIC ---
    int headX = head % SCREEN_WIDTH;

    if (currentDirection == 1 && headX == SCREEN_WIDTH - 1) { // Moving right off the right edge
        newHead = head - (SCREEN_WIDTH - 1); // Wrap to the left edge on the same row
    } else if (currentDirection == 3 && headX == 0) { // Moving left off the left edge
        newHead = head + (SCREEN_WIDTH - 1); // Wrap to the right edge on the same row
    } else if (currentDirection == 0 && newHead < 0) { // Moving up off the top edge
        newHead = head + (SCREEN_WIDTH * (SCREEN_HEIGHT - 1)); // Wrap to the bottom row in the same column
    } else if (currentDirection == 2 && newHead >= MAX_PIXELS) { // Moving down off the bottom edge
        newHead = head - (SCREEN_WIDTH * (SCREEN_HEIGHT - 1)); // Wrap to the top row in the same column
    }

    // Now, check for collision (only with the body, since walls are handled)
    if (isCollision(newHead)) {
        gameOver();
        return;
    }

    // Insert the new head into the snake's body
    snakeBody.insert(snakeBody.begin(), newHead);
    Screen.setPixelAtIndex(newHead, LED_TYPE_ON);

    // Handle dot collection or tail movement
    if (newHead == dotPosition) {
        spawnDot();
        aiPath.clear();
    } else {
        Screen.setPixelAtIndex(snakeBody.back(), LED_TYPE_OFF);
        snakeBody.pop_back();
    }
}

bool SnakePlugin::findPathToDot() {
    std::vector<int> q;
    q.push_back(snakeBody.front());
    std::vector<int> parent(MAX_PIXELS, -1);
    bool visited[MAX_PIXELS] = {false};
    // Mark the entire snake body as "visited" so the pathfinder avoids it.
    for(int seg : snakeBody) {
        if (seg >= 0 && seg < MAX_PIXELS) visited[seg] = true;
    }
    
    int head = 0;
    bool pathFound = false;
    
    while(head < q.size()){
        int current = q[head++];
        int currentX = current % SCREEN_WIDTH;
        
        if(current == dotPosition){ 
            pathFound = true; 
            break; 
        }

        // Define potential moves (Up, Right, Down, Left)
        int moves[] = {
            current - SCREEN_WIDTH, // Up
            current + 1,            // Right
            current + SCREEN_WIDTH, // Down
            current - 1             // Left
        };

        // --- APPLY WRAPPING TO POTENTIAL MOVES ---
        if (currentX == SCREEN_WIDTH - 1) moves[1] = current - (SCREEN_WIDTH - 1); // Wrap right
        if (currentX == 0) moves[3] = current + (SCREEN_WIDTH - 1); // Wrap left
        if (current < SCREEN_WIDTH) moves[0] = current + (SCREEN_WIDTH * (SCREEN_HEIGHT - 1)); // Wrap up
        if (current >= MAX_PIXELS - SCREEN_WIDTH) moves[2] = current - (SCREEN_WIDTH * (SCREEN_HEIGHT - 1)); // Wrap down

        for(int next : moves){
            if(next >= 0 && next < MAX_PIXELS && !visited[next]){
                visited[next] = true; 
                parent[next] = current; 
                q.push_back(next);
            }
        }
    }

    if(pathFound){
        aiPath.clear();
        int p_current = dotPosition;
        while(p_current != -1 && p_current != snakeBody.front()){
            aiPath.insert(aiPath.begin(), p_current);
            p_current = parent[p_current];
        }
        aiPathIndex = 0;
        return true;
    }
    return false; // No path found
}

void SnakePlugin::autoPlayMove() {
    // Imperfection: Occasionally make a random (but safe) move instead of following the path
    if (random(100) < AI_RANDOM_MOVE_CHANCE) {
        int directions[] = {0, 1, 2, 3};
        for(int i=0; i<4; i++) std::swap(directions[i], directions[random(0,4)]);
        for(int dir : directions) {
            if (dir == (currentDirection + 2) % 4) continue; // Don't move backwards
            int nextHead = snakeBody.front();
            if (dir == 0) nextHead -= SCREEN_WIDTH; else if (dir == 1) nextHead += 1;
            else if (dir == 2) nextHead += SCREEN_WIDTH; else if (dir == 3) nextHead -= 1;
            if (!isCollision(nextHead)) { currentDirection = dir; return; }
        }
    }

    // Follow the pre-calculated path
    if (aiPath.empty() || aiPathIndex >= aiPath.size()) {
        if (!findPathToDot()) { /* No path found, just keep moving straight until you die */ return; }
    }

    if (aiPathIndex < aiPath.size()) {
        int nextMove = aiPath[aiPathIndex];
        int head = snakeBody.front();
        if (nextMove == head - SCREEN_WIDTH) currentDirection = 0;
        else if (nextMove == head + 1) currentDirection = 1;
        else if (nextMove == head + SCREEN_WIDTH) currentDirection = 2;
        else if (nextMove == head - 1) currentDirection = 3;
        aiPathIndex++;
    }
}

void SnakePlugin::checkForUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        lastUdpPacketTime = millis();
        if (controlMode == CONTROL_AUTO) { controlMode = CONTROL_MANUAL; Serial.println("Controller detected! Switching Snake to MANUAL mode."); }
        int len = udp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;

        if (strcmp(packetBuffer, "UP") == 0 && currentDirection != 2) currentDirection = 0;
        else if (strcmp(packetBuffer, "RIGHT") == 0 && currentDirection != 3) currentDirection = 1;
        else if (strcmp(packetBuffer, "DOWN") == 0 && currentDirection != 0) currentDirection = 2;
        else if (strcmp(packetBuffer, "LEFT") == 0 && currentDirection != 1) currentDirection = 3;
    }
    if (controlMode == CONTROL_MANUAL && millis() - lastUdpPacketTime > UDP_TIMEOUT && lastUdpPacketTime != 0) {
        controlMode = CONTROL_AUTO; lastUdpPacketTime = 0; Serial.println("Controller timed out. Switching Snake back to AUTO mode.");
    }
}

void SnakePlugin::loop() {
    if (gameState == STATE_GAME_OVER) {
        newGame();
        return;
    }

    checkForUdp();
    
    if (millis() - lastGameTick > GAME_SPEED_MS) {
        lastGameTick = millis();
        if (controlMode == CONTROL_AUTO) {
            autoPlayMove();
        }
        moveSnake();
    }
}