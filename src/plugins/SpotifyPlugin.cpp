#include "plugins/SpotifyPlugin.h"
#include "screen.h"

void SpotifyPlugin::setup() {
    Screen.clear();
    lastSongId = "";
    isScrolling = false;
    lastState = -1;
}

void SpotifyPlugin::teardown() {
    spotifyClient.stop();
}

void SpotifyPlugin::drawProgressBar(int progressPct) {
    // 16x16 screen, total perimeter is 16+15+15+14 = 60 pixels
    // Let's just do a bottom row progress bar for simplicity. 16 pixels.
    int litPixels = (progressPct * 16) / 100;
    if (litPixels > 16) litPixels = 16;
    
    for (int i = 0; i < 16; i++) {
        Screen.setPixel(i, 15, (i < litPixels) ? 255 : 0);
    }
}

void SpotifyPlugin::drawPlayPauseIcon(bool isPlaying) {
    // Play icon (triangle)
    // 0,1
    // 0,2 1,2
    // 0,3 1,3 2,3
    // 0,4 1,4
    // 0,5
    
    // Pause icon (two bars)
    // 0,1 2,1
    // 0,2 2,2
    // 0,3 2,3
    // 0,4 2,4
    
    // Clear top left corner 4x5
    for(int x=0; x<4; x++) {
        for(int y=0; y<6; y++) {
            Screen.setPixel(x, y, 0);
        }
    }
    
    if (isPlaying) {
        Screen.setPixel(0, 1, 255);
        Screen.setPixel(0, 2, 255); Screen.setPixel(1, 2, 255);
        Screen.setPixel(0, 3, 255); Screen.setPixel(1, 3, 255); Screen.setPixel(2, 3, 255);
        Screen.setPixel(0, 4, 255); Screen.setPixel(1, 4, 255);
        Screen.setPixel(0, 5, 255);
    } else {
        Screen.setPixel(0, 1, 255); Screen.setPixel(2, 1, 255);
        Screen.setPixel(0, 2, 255); Screen.setPixel(2, 2, 255);
        Screen.setPixel(0, 3, 255); Screen.setPixel(2, 3, 255);
        Screen.setPixel(0, 4, 255); Screen.setPixel(2, 4, 255);
        Screen.setPixel(0, 5, 255); Screen.setPixel(2, 5, 255);
    }
}

void SpotifyPlugin::triggerScroll() {
    isScrolling = true;
    lastScrollTime = millis();
}

void SpotifyPlugin::loop() {
    spotifyClient.loop();
    const SpotifyData& data = spotifyClient.getData();

    int currentState = 0;
    if (!data.isValid) currentState = 1;
    else if (!data.isPlaying && data.id.isEmpty()) currentState = 2;
    else currentState = 3;

    if (currentState != lastState) {
        Screen.clear();
        lastState = currentState;
    }

    if (!data.isValid) {
        Screen.drawLoadingAnimation(7);
        delay(100);
        return;
    }

    if (!data.isPlaying && data.id.isEmpty()) {
        // Draw a Z manually
        Screen.setPixel(6, 5, 255); Screen.setPixel(7, 5, 255); Screen.setPixel(8, 5, 255);
        Screen.setPixel(8, 6, 255);
        Screen.setPixel(7, 7, 255);
        Screen.setPixel(6, 8, 255);
        Screen.setPixel(6, 9, 255); Screen.setPixel(7, 9, 255); Screen.setPixel(8, 9, 255);
        delay(100);
        return;
    }

    if (data.id != lastSongId) {
        lastSongId = data.id;
        scrollText = data.artist + " - " + data.title;
        triggerScroll();
    }

    if (isScrolling) {
        Screen.clear();
        // scrollText is blocking in this Screen implementation, so we call it and then reset
        Screen.scrollText(scrollText.c_str(), 40);
        isScrolling = false;
        Screen.clear(); // Clear after scroll
        lastState = -1; // Force redraw next frame
    }

    // Static dashboard
    drawPlayPauseIcon(data.isPlaying);
    
    int pct = (data.durationMs > 0) ? (data.progressMs * 100) / data.durationMs : 0;
    drawProgressBar(pct);
    
    // Draw music note
    Screen.setPixel(10, 2, 255); Screen.setPixel(11, 2, 255); Screen.setPixel(12, 2, 255); Screen.setPixel(13, 2, 255);
    Screen.setPixel(10, 3, 255); Screen.setPixel(13, 3, 255);
    Screen.setPixel(10, 4, 255); Screen.setPixel(13, 4, 255);
    Screen.setPixel(10, 5, 255); Screen.setPixel(13, 5, 255);
    Screen.setPixel(9,  6, 255); Screen.setPixel(10, 6, 255); Screen.setPixel(12, 6, 255); Screen.setPixel(13, 6, 255);
    Screen.setPixel(9,  7, 255); Screen.setPixel(10, 7, 255); Screen.setPixel(12, 7, 255); Screen.setPixel(13, 7, 255);

    delay(100);
}

const char *SpotifyPlugin::getName() const {
    return "Spotify Now Playing";
}
