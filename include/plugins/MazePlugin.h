#pragma once

#include "PluginManager.h"
#include <WiFiUdp.h>
#include <vector> // We'll use a vector to store the AI's path

class MazePlugin : public Plugin
{
private:
    // --- Game Constants ---
    static const uint8_t MAZE_WIDTH = 16;
    static const uint8_t MAZE_HEIGHT = 16;
    static const uint8_t WALL = 1;
    static const uint8_t PATH = 0;

    // --- Game States ---
    enum GameState { STATE_PLAYING, STATE_GAME_OVER, STATE_WON };
    GameState gameState;
    
    enum ControlMode { CONTROL_AUTO, CONTROL_MANUAL };
    ControlMode controlMode;

    // --- Game Data ---
    uint8_t maze[MAZE_WIDTH * MAZE_HEIGHT];
    int8_t playerX, playerY;
    int8_t goalX, goalY;

    // --- AI Pathfinding Data ---
    std::vector<int> aiPath; // Stores the sequence of moves for the AI
    int aiPathIndex;

    // --- Non-Blocking Timers & Network ---
    unsigned long lastAutoPlayMove;
    static const unsigned int AUTO_PLAY_DELAY = 100; // AI moves quickly
    WiFiUDP udp;
    static const unsigned int UDP_PORT = 12345;
    static const unsigned long UDP_TIMEOUT = 5000;
    unsigned long lastUdpPacketTime;
    char packetBuffer[255];

    // --- Core Game Functions ---
    void newGame();
    void generateMaze(int startX, int startY);
    bool solveMaze(); // The AI brain (pathfinder)
    void checkWinCondition();

    // --- Drawing & Input ---
    void drawWorld();
    void checkForUdp();
    void autoPlayMove();

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};