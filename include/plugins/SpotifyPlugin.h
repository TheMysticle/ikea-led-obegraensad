#pragma once

#include "PluginManager.h"
#include "SpotifyClient.h"

class SpotifyPlugin : public Plugin {
private:
    unsigned long lastScrollTime = 0;
    bool isScrolling = false;
    String lastSongId = "";
    String scrollText = "";
    int lastState = -1;
    
    void drawProgressBar(int progressPct);
    void drawPlayPauseIcon(bool isPlaying);
    void triggerScroll();

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};
