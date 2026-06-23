#pragma once

#include "PluginManager.h"

class BigClockPlugin : public Plugin
{
private:
  struct tm timeinfo;

  int previousMinutes;
  int previousHour;
  std::vector<int> previousHH;
  std::vector<int> previousMM;
  bool previousLeadingZero;

  String lastSongId;
  unsigned long lastPlayingTime;
  bool isScrolling;
  String scrollText;
  bool wasPlaying;

public:
  void setup() override;
  void teardown() override;
  void loop() override;
  const char *getName() const override;
};
