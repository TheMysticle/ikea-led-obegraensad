#include "plugins/SpotifyClockPlugin.h"
#include "screen.h"
#include <time.h>

void SpotifyClockPlugin::setup() {
    Screen.clear();
    lastSongId = "";
    isScrolling = false;
    lastState = -1;
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

    std::vector<int> hh = {timeinfo.tm_hour / 10, timeinfo.tm_hour % 10};
    std::vector<int> mm = {timeinfo.tm_min / 10, timeinfo.tm_min % 10};

    // Draw stacked like normal ClockPlugin, but moved to the right (x=6) to leave space for icons
    Screen.drawNumbers(6, 2, hh);
    Screen.drawNumbers(6, 8, mm);
}

void SpotifyClockPlugin::triggerScroll() {
    isScrolling = true;
    lastScrollTime = millis();
}

void SpotifyClockPlugin::loop() {
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
        drawClock();
        delay(100);
        return;
    }

    if (!data.isPlaying && data.id.isEmpty()) {
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
        lastState = -1; // Force redraw of full screen next loop
    }

    drawClock();
    drawPlayPauseIcon(data.isPlaying);
    
    int pct = (data.durationMs > 0) ? (data.progressMs * 100) / data.durationMs : 0;
    drawProgressBar(pct);

    delay(100);
}

const char *SpotifyClockPlugin::getName() const {
    return "Spotify Clock";
}
