#pragma once

#include "PluginManager.h"
#include "SpotifyClient.h"

class SpotifyClockPlugin : public Plugin {
private:
    unsigned long lastScrollTime = 0;
    bool isScrolling = false;
    String lastSongId = "";
    String scrollText = "";
    
    void drawProgressBar(int progressPct);
    void drawPlayPauseIcon(bool isPlaying);
    void triggerScroll();
    void drawClock();

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};
