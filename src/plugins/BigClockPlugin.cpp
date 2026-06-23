#include "plugins/BigClockPlugin.h"
#include "SpotifyClient.h"
#include "config.h"

void BigClockPlugin::setup()
{
  // loading screen
  Screen.setPixel(4, 7, 1);
  Screen.setPixel(5, 7, 1);
  Screen.setPixel(7, 7, 1);
  Screen.setPixel(8, 7, 1);
  Screen.setPixel(10, 7, 1);
  Screen.setPixel(11, 7, 1);

  previousMinutes = -1;
  previousHour = -1;
  previousHH.clear();
  previousHH.clear();
  previousMM.clear();
  previousLeadingZero = false;

  lastSongId = "";
  lastPlayingTime = 0;
  isScrolling = false;
}

void BigClockPlugin::teardown()
{
  spotifyClient.stop();
}

void BigClockPlugin::loop()
{
  bool usesSpotify = config.getBigClockShowSpotify() || config.getBigClockShowProgress();
  
  if (usesSpotify) {
      spotifyClient.loop();
  } else {
      spotifyClient.stop();
  }

  const SpotifyData& data = spotifyClient.getData();

  if (data.isPlaying != wasPlaying) {
      wasPlaying = data.isPlaying;
      if (data.isPlaying && config.getBigClockShowSpotify() && data.isValid && !hasScrolledCurrentSong) {
          isScrolling = true;
          hasScrolledCurrentSong = true;
      }
  }

  if (config.getBigClockShowSpotify() && data.isValid && data.id != lastSongId) {
      lastSongId = data.id;
      scrollText = data.artist + " - " + data.title;
      hasScrolledCurrentSong = false;
      if (data.isPlaying) {
          isScrolling = true;
          hasScrolledCurrentSong = true;
      }
  }

  if (isScrolling) {
      Screen.clear();
      Screen.scrollText(scrollText.c_str(), 40);
      isScrolling = false;
      Screen.clear();
      previousHH.clear();
      previousMM.clear();
  }

  if (data.isPlaying) {
      lastPlayingTime = millis();
  }

  unsigned long now = millis();
  static unsigned long lastUpdate = 0;
  bool clockUpdate = (now - lastUpdate >= 1000);

  if (config.getBigClockShowProgress()) {
    unsigned long fadeDelayMs = (unsigned long)config.getBigClockProgressFadeDelay() * 1000UL;
    unsigned long fadeDurationMs = 500; // 500ms fast fade
    unsigned long timeSincePause = now > lastPlayingTime ? now - lastPlayingTime : 0;
    
    bool isFading = (!data.isPlaying && timeSincePause > fadeDelayMs && timeSincePause <= fadeDelayMs + fadeDurationMs);

    if (clockUpdate || isFading) {
        uint8_t barBrightness = 0;
        
        if (data.isPlaying || timeSincePause <= fadeDelayMs) {
            barBrightness = 255;
        } else if (isFading) {
            float fadeFactor = 1.0f - (float)(timeSincePause - fadeDelayMs) / fadeDurationMs;
            barBrightness = (uint8_t)(255.0f * fadeFactor);
        }

        if (data.isValid && data.durationMs > 0) {
            int progressPct = (data.progressMs * 100) / data.durationMs;
            int litPixels = (progressPct * 16) / 100;
            if (litPixels > 16) litPixels = 16;
            for (int i = 0; i < 16; i++) {
                if (i < litPixels && barBrightness > 0) {
                    Screen.setPixel(i, 15, 255, barBrightness);
                } else {
                    Screen.setPixel(i, 15, 0);
                }
            }
        } else {
            for (int i = 0; i < 16; i++) {
                Screen.setPixel(i, 15, 0);
            }
        }
    }
  } else if (clockUpdate) {
      // If disabled, ensure row 15 is clean on clock updates
      for (int i = 0; i < 16; i++) {
          Screen.setPixel(i, 15, 0);
      }
  }
  
  if (clockUpdate) // Only update once per second
  {
    lastUpdate = now;
    if (getLocalTime(&timeinfo))
    {
      std::vector<int> hh = {(timeinfo.tm_hour - timeinfo.tm_hour % 10) / 10, timeinfo.tm_hour % 10};
      std::vector<int> mm = {(timeinfo.tm_min - timeinfo.tm_min % 10) / 10, timeinfo.tm_min % 10};
      bool leadingZero = (hh.at(0) == 0);

      bool layoutChanged = (previousHH.empty() || previousLeadingZero != leadingZero);

      if (layoutChanged)
      {
        Screen.clear();
        if (leadingZero)
        {
          hh.erase(hh.begin());
          Screen.drawBigNumbers(COLS / 2, 0, hh);
          Screen.drawBigNumbers(0, ROWS / 2, mm);
        }
        else
        {
          Screen.drawBigNumbers(0, 0, hh);
          Screen.drawBigNumbers(0, ROWS / 2, mm);
        }
      }
      else
      {
        std::vector<int> displayHH = hh;
        if (leadingZero)
        {
          displayHH.erase(displayHH.begin());
        }

        if (displayHH != previousHH)
        {
          int startX = leadingZero ? COLS / 2 : 0;
          Screen.drawBigNumbers(startX, 0, displayHH);
        }

        if (mm != previousMM)
        {
          Screen.drawBigNumbers(0, ROWS / 2, mm);
        }

        previousHH = displayHH;
        previousMM = mm;
      }

      if (layoutChanged)
      {
        std::vector<int> displayHH = hh;
        if (leadingZero)
        {
          displayHH.erase(displayHH.begin());
        }
        previousHH = displayHH;
        previousMM = mm;
      }

      previousMinutes = timeinfo.tm_min;
      previousHour = timeinfo.tm_hour;
      previousLeadingZero = leadingZero;
    }
  }
}

const char *BigClockPlugin::getName() const
{
  return "Big Clock";
}