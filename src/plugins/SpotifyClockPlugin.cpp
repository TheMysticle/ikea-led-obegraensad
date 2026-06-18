#include "plugins/SpotifyClockPlugin.h"
#include "screen.h"
#include <time.h>

void SpotifyClockPlugin::setup() {
    Screen.clear();
    lastSongId = "";
    isScrolling = false;
}

void SpotifyClockPlugin::drawProgressBar(int progressPct) {
    int litPixels = (progressPct * 16) / 100;
    if (litPixels > 16) litPixels = 16;
    
    for (int i = 0; i < 16; i++) {
        Screen.setPixel(i, 15, (i < litPixels) ? 255 : 0);
    }
}

void SpotifyClockPlugin::drawPlayPauseIcon(bool isPlaying) {
    for(int x=0; x<4; x++) {
        for(int y=0; y<4; y++) {
            Screen.setPixel(x, y, 0);
        }
    }
    
    if (isPlaying) {
        Screen.setPixel(0, 0, 255);
        Screen.setPixel(0, 1, 255); Screen.setPixel(1, 1, 255);
        Screen.setPixel(0, 2, 255); Screen.setPixel(1, 2, 255); Screen.setPixel(2, 2, 255);
        Screen.setPixel(0, 3, 255); Screen.setPixel(1, 3, 255);
        Screen.setPixel(0, 4, 255);
    } else {
        Screen.setPixel(0, 0, 255); Screen.setPixel(2, 0, 255);
        Screen.setPixel(0, 1, 255); Screen.setPixel(2, 1, 255);
        Screen.setPixel(0, 2, 255); Screen.setPixel(2, 2, 255);
        Screen.setPixel(0, 3, 255); Screen.setPixel(2, 3, 255);
        Screen.setPixel(0, 4, 255); Screen.setPixel(2, 4, 255);
    }
}

void SpotifyClockPlugin::drawClock() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return;
    }

    int hours = timeinfo.tm_hour;
    int minutes = timeinfo.tm_min;

    std::vector<int> numbers = {
        hours / 10,
        hours % 10,
        10, // colon
        minutes / 10,
        minutes % 10
    };

    Screen.drawNumbers(1, 5, numbers); // Starts at x=1, y=5. 3x5 font. 5 chars = 15 pixels wide.
}

void SpotifyClockPlugin::triggerScroll() {
    isScrolling = true;
    lastScrollTime = millis();
}

void SpotifyClockPlugin::loop() {
    spotifyClient.loop();
    const SpotifyData& data = spotifyClient.getData();

    if (!data.isValid) {
        Screen.clear();
        drawClock();
        delay(100);
        return;
    }

    if (!data.isPlaying && data.id.isEmpty()) {
        Screen.clear();
        drawClock();
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
        Screen.scrollText(scrollText.c_str(), 40);
        isScrolling = false;
        Screen.clear(); // Clear after scroll
    }

    Screen.clear();
    drawClock();
    drawPlayPauseIcon(data.isPlaying);
    
    int pct = (data.durationMs > 0) ? (data.progressMs * 100) / data.durationMs : 0;
    drawProgressBar(pct);

    delay(100);
}

const char *SpotifyClockPlugin::getName() const {
    return "Spotify Clock";
}
