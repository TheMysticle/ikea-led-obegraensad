#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct SpotifyData {
  bool isPlaying;
  String artist;
  String title;
  String id;
  unsigned long progressMs;
  unsigned long durationMs;
  unsigned long lastUpdateMs;
  bool isValid;
};

class SpotifyClient {
public:
  SpotifyClient();
  void init();
  void stop();
  void loop(); // Used to process any non-blocking main-thread tasks, now empty
  SpotifyData getData() const;

private:
  TaskHandle_t taskHandle;
  SemaphoreHandle_t dataMutex;
  static void networkTask(void* param);
  bool isTaskRunning;
  volatile bool shouldStop;

  WiFiClientSecure wifiClient;
  HTTPClient httpClient;
  
  String accessToken;
  unsigned long tokenExpirationMs;
  bool isInitialized;
  
  SpotifyData data;
  unsigned long lastPollTime;
  const unsigned long POLL_INTERVAL = 3000; // 3 seconds
  
  bool refreshAccessToken();
  bool fetchCurrentlyPlaying();
};

extern SpotifyClient spotifyClient;
