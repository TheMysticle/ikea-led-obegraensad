#pragma once

#include "PluginManager.h"
#include <WiFiUdp.h>

class TetrisPlugin : public Plugin
{
private:
    // --- Game Constants ---
    static const uint8_t FIELD_WIDTH = 16;
    static const uint8_t FIELD_HEIGHT = 16;

    enum GameState { STATE_RUNNING, STATE_GAME_OVER };
    GameState gameState;
    
    enum ControlMode { CONTROL_AUTO, CONTROL_MANUAL };
    ControlMode controlMode;

    // --- AI Target ---
    int aiTargetX;
    int aiTargetRotation;
    bool aiMoveCalculated; // Flag to check if we've found a move for the current piece

    // --- Game Data ---
    uint8_t playfield[FIELD_WIDTH * FIELD_HEIGHT];
    int currentPiece;
    int currentRotation;
    int currentX;
    int currentY;
    int score;

    // --- Non-Blocking Timers ---
    unsigned long lastGameTick;
    unsigned int gameTickDelay;
    unsigned long lastAutoPlayMove;
    static const unsigned int AUTO_PLAY_DELAY = 100; // AI can move faster now

    // --- Network Control ---
    WiFiUDP udp;
    static const unsigned int UDP_PORT = 12345;
    static const unsigned long UDP_TIMEOUT = 5000;
    unsigned long lastUdpPacketTime;
    char packetBuffer[255];

    // --- Core Game Functions ---
    void newGame();
    void newPiece();
    void gameOver();
    void findBestMove(); // The new AI brain

    // --- Piece & Board Functions ---
    int getPieceData(int piece, int rotation, int x, int y);
    bool doesPieceFit(int piece, int rotation, int x, int y);
    void lockPiece();
    void checkLines();

    // --- Drawing Functions ---
    void drawPlayfield();
    void drawPiece(int piece, int rotation, int x, int y, bool draw);

    // --- Input Handling & AI ---
    void checkForUdp();
    void autoPlayMove();

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};