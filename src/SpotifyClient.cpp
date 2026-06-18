#include "SpotifyClient.h"
#include "config.h"
#include <ArduinoJson.h>
#include "mbedtls/base64.h"

SpotifyClient spotifyClient;

SpotifyClient::SpotifyClient() {
  data.isValid = false;
  data.isPlaying = false;
  lastPollTime = 0;
  tokenExpirationMs = 0;
  isInitialized = false;
}

void SpotifyClient::init() {
  if (!isInitialized) {
    wifiClient.setInsecure(); // Disable certificate validation for simplicity, or provide root CA
    isInitialized = true;
  }
}

void SpotifyClient::loop() {
  // If not configured, don't do anything
  if (config.getSpotifyClientId().isEmpty() || config.getSpotifyClientSecret().isEmpty() || config.getSpotifyRefreshToken().isEmpty()) {
    return;
  }

  // Poll interval
  if (millis() - lastPollTime >= POLL_INTERVAL || lastPollTime == 0) {
    lastPollTime = millis();
    
    // Check if token needs refresh
    if (millis() >= tokenExpirationMs || accessToken.isEmpty()) {
      if (!refreshAccessToken()) {
        return; // failed to refresh
      }
    }
    
    fetchCurrentlyPlaying();
  }
}

bool SpotifyClient::refreshAccessToken() {
  init();
  
  String url = "https://accounts.spotify.com/api/token";
  String auth = config.getSpotifyClientId() + ":" + config.getSpotifyClientSecret();
  
  // Base64 encode the auth string
  size_t base64Len = 0;
  mbedtls_base64_encode(nullptr, 0, &base64Len, (const unsigned char*)auth.c_str(), auth.length());
  unsigned char* base64Buf = new unsigned char[base64Len + 1];
  mbedtls_base64_encode(base64Buf, base64Len + 1, &base64Len, (const unsigned char*)auth.c_str(), auth.length());
  String encodedAuth = String((char*)base64Buf);
  delete[] base64Buf;
  
  httpClient.begin(wifiClient, url);
  httpClient.addHeader("Authorization", "Basic " + encodedAuth);
  httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String payload = "grant_type=refresh_token&refresh_token=" + config.getSpotifyRefreshToken();
  
  int httpCode = httpClient.POST(payload);
  if (httpCode == 200) {
    StaticJsonDocument<512> doc;
    deserializeJson(doc, httpClient.getStream());
    
    accessToken = doc["access_token"].as<String>();
    int expiresIn = doc["expires_in"].as<int>();
    tokenExpirationMs = millis() + ((expiresIn - 60) * 1000); // refresh 60s early
    
    httpClient.end();
    return true;
  } else {
    Serial.printf("[Spotify] Token refresh failed, error: %d\n", httpCode);
    httpClient.end();
    return false;
  }
}

bool SpotifyClient::fetchCurrentlyPlaying() {
  init();
  
  String url = "https://api.spotify.com/v1/me/player/currently-playing";
  
  httpClient.begin(wifiClient, url);
  httpClient.addHeader("Authorization", "Bearer " + accessToken);
  
  int httpCode = httpClient.GET();
  if (httpCode == 200) {
    // Optimization: Filter out everything except what we need
    StaticJsonDocument<256> filter;
    filter["is_playing"] = true;
    filter["progress_ms"] = true;
    filter["item"]["duration_ms"] = true;
    filter["item"]["name"] = true;
    filter["item"]["id"] = true;
    filter["item"]["artists"][0]["name"] = true;
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, httpClient.getStream(), DeserializationOption::Filter(filter));
    
    if (!error) {
      data.isPlaying = doc["is_playing"] | false;
      data.progressMs = doc["progress_ms"] | 0;
      data.durationMs = doc["item"]["duration_ms"] | 0;
      
      JsonVariant item = doc["item"];
      if (!item.isNull()) {
        data.title = item["name"].as<String>();
        data.id = item["id"].as<String>();
        JsonVariant artistArray = item["artists"];
        if (!artistArray.isNull() && artistArray.size() > 0) {
            data.artist = artistArray[0]["name"].as<String>();
        } else {
            data.artist = "Unknown";
        }
      } else {
          data.title = "";
          data.id = "";
          data.artist = "";
      }

      data.lastUpdateMs = millis();
      data.isValid = true;
    } else {
      Serial.print("[Spotify] JSON Parsing failed: ");
      Serial.println(error.c_str());
    }
  } else if (httpCode == 204) {
    // No content (nothing is playing)
    data.isPlaying = false;
    data.isValid = true;
    data.lastUpdateMs = millis();
  } else {
    Serial.printf("[Spotify] Fetch failed, error: %d\n", httpCode);
    if (httpCode == 401) {
      accessToken = ""; // Force refresh next time
    }
  }
  
  httpClient.end();
  return data.isValid;
}

const SpotifyData& SpotifyClient::getData() const {
  return data;
}
