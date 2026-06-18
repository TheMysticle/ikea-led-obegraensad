#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

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
  void loop(); // Polls Spotify API
  const SpotifyData& getData() const;

private:
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
