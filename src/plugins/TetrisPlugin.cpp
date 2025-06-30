#include "plugins/TetrisPlugin.h"

static const char* TETROMINOES[7] = {
    "..X...X...X...X.", ".XX..XX.........", ".X..XX..X.......", ".X..XX...X......", "X...XX..-X......", "X...X...XX......", ".X...X..XX......"
};

void TetrisPlugin::setup() { gameState = STATE_GAME_OVER; }
const char* TetrisPlugin::getName() const { return "Tetris"; }

void TetrisPlugin::newGame() {
    Screen.clear();
    memset(playfield, 0, FIELD_WIDTH * FIELD_HEIGHT);
    score = 0;
    gameTickDelay = 500;
    lastGameTick = millis();
    lastAutoPlayMove = millis();
    udp.begin(UDP_PORT);
    controlMode = CONTROL_AUTO;
    lastUdpPacketTime = 0;
    Serial.println("Tetris: Started in smart AUTO mode. Listening for controller...");
    newPiece();
    gameState = STATE_RUNNING;
    drawPlayfield();
}

void TetrisPlugin::newPiece() {
    currentPiece = random(0, 7);
    currentRotation = 0;
    currentX = FIELD_WIDTH / 2 - 2;
    currentY = 0;
    aiMoveCalculated = false; // Need to calculate a new best move for this piece
    if (!doesPieceFit(currentPiece, currentRotation, currentX, currentY)) {
        gameOver();
    }
}

void TetrisPlugin::gameOver() {
    udp.stop();
    gameState = STATE_GAME_OVER;
    Screen.scrollText("GAME OVER", 60);
    delay(2000);
}

void TetrisPlugin::findBestMove() {
    long bestScore = 1000000; // Lower is better

    // Iterate through every possible rotation
    for (int r = 0; r < 4; r++) {
        // Iterate through every possible column
        for (int x = -2; x < FIELD_WIDTH; x++) {
            // Check if this move is possible from the top
            if (doesPieceFit(currentPiece, r, x, 0)) {
                
                int y = 0;
                while (doesPieceFit(currentPiece, r, x, y + 1)) {
                    y++;
                }

                uint8_t tempPlayfield[FIELD_WIDTH * FIELD_HEIGHT];
                memcpy(tempPlayfield, playfield, sizeof(playfield));
                for (int px = 0; px < 4; px++) {
                    for (int py = 0; py < 4; py++) {
                        if (getPieceData(currentPiece, r, px, py)) {
                            if ((y + py) >= 0) tempPlayfield[(y + py) * FIELD_WIDTH + (x + px)] = 1;
                        }
                    }
                }

                int landingHeight = (FIELD_HEIGHT - y);
                int completedLines = 0;
                int holes = 0;
                int bumpiness = 0;
                
                for (int ly = 0; ly < FIELD_HEIGHT; ly++) {
                    bool lineComplete = true;
                    for (int lx = 0; lx < FIELD_WIDTH; lx++) if (tempPlayfield[ly * FIELD_WIDTH + lx] == 0) { lineComplete = false; break; }
                    if (lineComplete) completedLines++;
                }

                int columnHeights[FIELD_WIDTH] = {0};
                for (int cx = 0; cx < FIELD_WIDTH; cx++) {
                    int cy = 0;
                    while (cy < FIELD_HEIGHT && tempPlayfield[cy * FIELD_WIDTH + cx] == 0) cy++;
                    columnHeights[cx] = FIELD_HEIGHT - cy;
                    for (; cy < FIELD_HEIGHT; cy++) if (tempPlayfield[cy * FIELD_WIDTH + cx] == 0) holes++;
                }
                for (int i = 0; i < FIELD_WIDTH - 1; i++) bumpiness += abs(columnHeights[i] - columnHeights[i+1]);

                // --- THIS IS THE NERF ---
                // Add a random "noise" value to the score. This makes the AI's choice less deterministic.
                long randomNoise = random(0, AI_INACCURACY_WEIGHT);
                long currentScore = (10 * landingHeight) + (25 * holes) + (5 * bumpiness) - (50 * completedLines * completedLines) + randomNoise;

                if (currentScore < bestScore) {
                    bestScore = currentScore;
                    aiTargetX = x;
                    aiTargetRotation = r;
                }
            }
        }
    }
    aiMoveCalculated = true;
}


int TetrisPlugin::getPieceData(int piece, int rotation, int x, int y) {
    int index=0;
    switch(rotation%4){case 0:index=y*4+x;break;case 1:index=12+y-(x*4);break;case 2:index=15-(y*4)-x;break;case 3:index=3-y+(x*4);break;}
    return (TETROMINOES[piece][index]=='X');
}
bool TetrisPlugin::doesPieceFit(int p,int r,int x,int y){for(int px=0;px<4;px++)for(int py=0;py<4;py++)if(getPieceData(p,r,px,py)){int fx=x+px,fy=y+py;if(fx<0||fx>=FIELD_WIDTH||fy>=FIELD_HEIGHT)return false;if(fy>=0&&playfield[fy*FIELD_WIDTH+fx]!=0)return false;}return true;}
void TetrisPlugin::lockPiece(){for(int px=0;px<4;px++)for(int py=0;py<4;py++)if(getPieceData(currentPiece,currentRotation,px,py)){int fx=currentX+px,fy=currentY+py;if(fy>=0)playfield[fy*FIELD_WIDTH+fx]=1;}}
void TetrisPlugin::checkLines(){for(int y=0;y<FIELD_HEIGHT;y++){bool f=true;for(int x=0;x<FIELD_WIDTH;x++)if(playfield[y*FIELD_WIDTH+x]==0){f=false;break;}if(f){for(int k=y;k>0;k--)memcpy(&playfield[k*FIELD_WIDTH],&playfield[(k-1)*FIELD_WIDTH],FIELD_WIDTH);memset(&playfield[0],0,FIELD_WIDTH);score+=10;if(gameTickDelay>100)gameTickDelay-=10;}}}
void TetrisPlugin::drawPlayfield(){for(int y=0;y<FIELD_HEIGHT;y++)for(int x=0;x<FIELD_WIDTH;x++)Screen.setPixel(x,y,playfield[y*FIELD_WIDTH+x]);}
void TetrisPlugin::drawPiece(int p,int r,int x,int y,bool d){for(int px=0;px<4;px++)for(int py=0;py<4;py++)if(getPieceData(p,r,px,py))Screen.setPixel(x+px,y+py,d?1:0);}

void TetrisPlugin::autoPlayMove() {
    if (!aiMoveCalculated) {
        findBestMove();
    }
    
    drawPiece(currentPiece, currentRotation, currentX, currentY, false); // Erase
    
    // Step 1: Rotate to match target
    if (currentRotation != aiTargetRotation) {
        if (doesPieceFit(currentPiece, currentRotation + 1, currentX, currentY)) currentRotation++;
    }
    // Step 2: Move horizontally to match target
    else if (currentX < aiTargetX) {
        if (doesPieceFit(currentPiece, currentRotation, currentX + 1, currentY)) currentX++;
    } else if (currentX > aiTargetX) {
        if (doesPieceFit(currentPiece, currentRotation, currentX - 1, currentY)) currentX--;
    }

    drawPiece(currentPiece, currentRotation, currentX, currentY, true); // Redraw
}

void TetrisPlugin::checkForUdp() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;

        if(strcmp(packetBuffer,"LEFT")==0||strcmp(packetBuffer,"RIGHT")==0||strcmp(packetBuffer,"UP")==0||strcmp(packetBuffer,"DOWN")==0||strcmp(packetBuffer,"PING")==0){
            lastUdpPacketTime = millis();
            if (controlMode == CONTROL_AUTO) {
                controlMode = CONTROL_MANUAL;
                Serial.println("Controller detected! Switching Tetris to MANUAL mode.");
            }
            drawPiece(currentPiece,currentRotation,currentX,currentY,false);
            if(strcmp(packetBuffer,"LEFT")==0&&doesPieceFit(currentPiece,currentRotation,currentX-1,currentY))currentX--;
            else if(strcmp(packetBuffer,"RIGHT")==0&&doesPieceFit(currentPiece,currentRotation,currentX+1,currentY))currentX++;
            else if(strcmp(packetBuffer,"DOWN")==0&&doesPieceFit(currentPiece,currentRotation,currentX,currentY+1))currentY++;
            else if(strcmp(packetBuffer,"UP")==0&&doesPieceFit(currentPiece,currentRotation+1,currentX,currentY))currentRotation++;
            drawPiece(currentPiece,currentRotation,currentX,currentY,true);
        }
    }
    if(controlMode==CONTROL_MANUAL&&millis()-lastUdpPacketTime>UDP_TIMEOUT&&lastUdpPacketTime!=0){controlMode=CONTROL_AUTO;lastUdpPacketTime=0;Serial.println("Controller timed out. Switching Tetris back to AUTO mode.");}
}

void TetrisPlugin::loop() {
    if (gameState == STATE_GAME_OVER) { newGame(); return; }

    checkForUdp();

    if (millis() - lastGameTick > gameTickDelay) {
        lastGameTick = millis();
        drawPiece(currentPiece, currentRotation, currentX, currentY, false);
        if (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) {
            currentY++;
        } else {
            lockPiece();
            checkLines();
            newPiece();
            drawPlayfield();
        }
        drawPiece(currentPiece, currentRotation, currentX, currentY, true);
    }

    if (controlMode == CONTROL_AUTO) {
        if (millis() - lastAutoPlayMove > AUTO_PLAY_DELAY) {
            lastAutoPlayMove = millis();
            autoPlayMove();
        }
    }
}