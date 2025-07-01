#include "plugins/MazePlugin.h"

void MazePlugin::setup() {
    gameState = STATE_GAME_OVER;
}

const char* MazePlugin::getName() const {
    return "Maze";
}

void MazePlugin::newGame() {
    udp.begin(UDP_PORT);
    controlMode = CONTROL_AUTO;
    lastUdpPacketTime = 0;
    lastAutoPlayMove = millis();
    Serial.println("Maze: Started in AUTO mode. Generating maze...");

    playerX = 1;
    playerY = 1;
    
    // --- FIX: Generate maze first, then place goal ---
    generateMaze(playerX, playerY);

    // Now find a valid, distant spot for the goal.
    // We'll place it on the rightmost path tile in the second to last row.
    goalY = MAZE_HEIGHT - 2;
    goalX = MAZE_WIDTH - 2;
    while(maze[goalY * MAZE_WIDTH + goalX] == WALL) {
        goalX--;
        if (goalX < 1) { // Failsafe if the row is somehow solid
            goalX = MAZE_WIDTH - 2;
            goalY--;
        }
    }
    
    aiPath.clear();
    aiPathIndex = 0;
    
    gameState = STATE_PLAYING;
}

// Procedural maze generation using iterative Depth-First Search
void MazePlugin::generateMaze(int startX, int startY) {
    memset(maze, WALL, sizeof(maze));
    std::vector<int> stack;
    int startIdx = startY * MAZE_WIDTH + startX;
    stack.push_back(startIdx);
    maze[startIdx] = PATH;

    int dx[] = {0, 0, 2, -2};
    int dy[] = {2, -2, 0, 0};

    while (!stack.empty()) {
        int current_idx = stack.back();
        int cx = current_idx % MAZE_WIDTH;
        int cy = current_idx / MAZE_WIDTH;

        int directions[] = {0, 1, 2, 3};
        for(int i=0; i<4; i++) std::swap(directions[i], directions[random(0,4)]);

        bool moved = false;
        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[directions[i]];
            int ny = cy + dy[directions[i]];
            int next_idx = ny * MAZE_WIDTH + nx;

            if (nx > 0 && nx < MAZE_WIDTH - 1 && ny > 0 && ny < MAZE_HEIGHT - 1 && maze[next_idx] == WALL) {
                maze[next_idx] = PATH;
                maze[(cy + dy[directions[i]] / 2) * MAZE_WIDTH + (cx + dx[directions[i]] / 2)] = PATH;
                stack.push_back(next_idx);
                moved = true;
                break;
            }
        }
        if (!moved) {
            stack.pop_back();
        }
    }
}

// AI Pathfinding using Breadth-First Search (BFS)
bool MazePlugin::solveMaze() {
    Serial.println("AI solving maze...");
    std::vector<int> q;
    q.push_back(playerY * MAZE_WIDTH + playerX);
    
    std::vector<int> parent(MAZE_WIDTH * MAZE_HEIGHT, -1);
    bool visited[MAZE_WIDTH * MAZE_HEIGHT] = {false};
    visited[playerY * MAZE_WIDTH + playerX] = true;
    
    int head = 0;
    bool pathFound = false;
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};
    
    while(head < q.size()){
        int current_idx = q[head++];
        int cx = current_idx % MAZE_WIDTH;
        int cy = current_idx / MAZE_WIDTH;

        if(cx == goalX && cy == goalY){ pathFound = true; break; }

        for(int i=0; i<4; i++){
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            int next_idx = ny * MAZE_WIDTH + nx;
            if(nx >= 0 && nx < MAZE_WIDTH && ny >= 0 && ny < MAZE_HEIGHT && maze[next_idx] == PATH && !visited[next_idx]){
                visited[next_idx] = true; parent[next_idx] = current_idx; q.push_back(next_idx);
            }
        }
    }

    if(pathFound){
        aiPath.clear();
        int current = goalY * MAZE_WIDTH + goalX;
        while(current != playerY * MAZE_WIDTH + playerX){
            aiPath.insert(aiPath.begin(), current);
            current = parent[current];
        }
        aiPathIndex = 0;
        Serial.printf("AI path found, length: %d\n", aiPath.size());
        return true;
    }
    Serial.println("AI could not find a path.");
    return false;
}

void MazePlugin::checkWinCondition() {
    if (playerX == goalX && playerY == goalY) {
        gameState = STATE_WON;
        Screen.scrollText("YOU WIN", 60);
        delay(2000);
    }
}

void MazePlugin::drawWorld() {
    Screen.clear();
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            // --- FIX: Draw walls at full brightness to prevent flicker ---
            if (maze[y * MAZE_WIDTH + x] == WALL) Screen.setPixel(x, y, 1);
        }
    }
    Screen.setPixel(playerX, playerY, 1, 255);
    if(millis() % 500 < 250) Screen.setPixel(goalX, goalY, 1, 200);
}

void MazePlugin::autoPlayMove() {
    // --- FIX: If the path is empty, solve for a new one ---
    if (aiPath.empty()) {
        if (!solveMaze()) {
            // This should not happen, but as a failsafe, we just end the game.
            gameState = STATE_GAME_OVER; 
            return;
        }
    }

    if (aiPathIndex < aiPath.size()) {
        int next_idx = aiPath[aiPathIndex];
        playerX = next_idx % MAZE_WIDTH;
        playerY = next_idx / MAZE_WIDTH;
        aiPathIndex++;
    }
}

void MazePlugin::checkForUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        lastUdpPacketTime = millis();
        if (controlMode == CONTROL_AUTO) { controlMode = CONTROL_MANUAL; Serial.println("Controller detected! Switching Maze to MANUAL mode."); }
        int len = udp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;

        int nextX = playerX, nextY = playerY;
        if (strcmp(packetBuffer, "LEFT") == 0) nextX--;
        else if (strcmp(packetBuffer, "RIGHT") == 0) nextX++;
        else if (strcmp(packetBuffer, "UP") == 0) nextY--;
        else if (strcmp(packetBuffer, "DOWN") == 0) nextY++;

        if (maze[nextY * MAZE_WIDTH + nextX] == PATH) {
            playerX = nextX;
            playerY = nextY;
        }
    }
    if (controlMode == CONTROL_MANUAL && millis() - lastUdpPacketTime > UDP_TIMEOUT && lastUdpPacketTime != 0) {
        controlMode = CONTROL_AUTO;
        // --- FIX: When switching back to AI, clear the old path so it re-solves ---
        aiPath.clear();
        lastUdpPacketTime = 0;
        Serial.println("Controller timed out. Switching Maze back to AUTO mode.");
    }
}

void MazePlugin::loop() {
    if (gameState == STATE_GAME_OVER || gameState == STATE_WON) {
        newGame();
        return;
    }
    
    checkForUdp();
    
    if (controlMode == CONTROL_AUTO) {
        if (millis() - lastAutoPlayMove > AUTO_PLAY_DELAY) {
            lastAutoPlayMove = millis();
            autoPlayMove();
        }
    }

    checkWinCondition();
    drawWorld();
}