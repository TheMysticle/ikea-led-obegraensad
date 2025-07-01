#pragma once

#include "PluginManager.h"
#include <WiFiUdp.h>
#include <vector>

class SnakePlugin : public Plugin
{
private:
    // --- Game Constants ---
    static const uint8_t SCREEN_WIDTH = 16;
    static const uint8_t SCREEN_HEIGHT = 16;
    static const int MAX_PIXELS = SCREEN_WIDTH * SCREEN_HEIGHT;
    static const uint8_t LED_TYPE_OFF = 0;
    static const uint8_t LED_TYPE_ON = 1;

    // --- Game States & Control ---
    enum GameState { STATE_PLAYING, STATE_GAME_OVER };
    GameState gameState;
    enum ControlMode { CONTROL_AUTO, CONTROL_MANUAL };
    ControlMode controlMode;

    // --- Game Data ---
    std::vector<int> snakeBody;
    int dotPosition;
    int currentDirection; // 0: Up, 1: Right, 2: Down, 3: Left

    // --- AI Data ---
    std::vector<int> aiPath;
    int aiPathIndex;
    static const uint8_t AI_RANDOM_MOVE_CHANCE = 15; // 15% chance to make a random (but safe) move

    // --- Timers & Network ---
    unsigned long lastGameTick;
    static const unsigned int GAME_SPEED_MS = 150;
    WiFiUDP udp;
    static const unsigned int UDP_PORT = 12345;
    static const unsigned long UDP_TIMEOUT = 5000;
    unsigned long lastUdpPacketTime;
    char packetBuffer[255];

    // --- Core Functions ---
    void newGame();
    void spawnDot();
    void moveSnake();
    void gameOver();
    bool isCollision(int headPos);

    // --- AI & Input ---
    bool findPathToDot(); // The AI pathfinding brain
    void checkForUdp();
    void autoPlayMove();

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};