#pragma once

#include "PluginManager.h"
#include <WiFiUdp.h>

class FroggerPlugin : public Plugin
{
private:
    // --- Game Constants ---
    static const uint8_t SCREEN_WIDTH = 16;
    static const uint8_t SCREEN_HEIGHT = 16;
    static const uint8_t LANE_COUNT = SCREEN_HEIGHT;
    static const uint8_t HOME_COUNT = 3;

    // --- Game States ---
    enum GameState { STATE_PLAYING, STATE_GAME_OVER, STATE_LEVEL_WON };
    GameState gameState;
    
    // NEW: Control Mode for AI/Manual switching
    enum ControlMode { CONTROL_AUTO, CONTROL_MANUAL };
    ControlMode controlMode;

    // --- Lane Configuration ---
    enum LaneType { SAFE_GRASS, ROAD, WATER };
    struct Lane {
        LaneType type;
        int8_t direction; // -1 for left, 1 for right
        uint16_t speed;   // Delay in ms between moves
        unsigned long lastMoveTime;
        uint8_t data[SCREEN_WIDTH]; // Represents cars, logs, etc.
    };
    Lane lanes[LANE_COUNT];

    // --- Player Data ---
    int8_t playerX;
    int8_t playerY;
    uint8_t lives;
    int score;

    // --- Home Bays ---
    bool homesFilled[HOME_COUNT];
    const uint8_t homePositions[HOME_COUNT] = {3, 8, 13};

    // --- Non-Blocking Timers & Network ---
    unsigned long lastAutoPlayMove; // NEW: Timer for AI thinking speed
    static const unsigned int AUTO_PLAY_DELAY = 300; // AI thinks every 300ms
    WiFiUDP udp;
    static const unsigned int UDP_PORT = 12345;
    static const unsigned long UDP_TIMEOUT = 5000;
    unsigned long lastUdpPacketTime;
    char packetBuffer[255];

    // --- Core Game Functions ---
    void newGame();
    void resetLevel();
    void playerDie();
    void checkPlayerState();
    void updateLanes();
    void drawWorld();
    void checkForUdp();
    void autoPlayMove(); // NEW: The AI brain

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};