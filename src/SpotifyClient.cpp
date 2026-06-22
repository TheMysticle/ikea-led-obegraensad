#include "SpotifyClient.h"
#include "config.h"
#include <ArduinoJson.h>
#include "mbedtls/base64.h"
#include "Logger.h"

SpotifyClient spotifyClient;

SpotifyClient::SpotifyClient() {
  data.isValid = false;
  data.isPlaying = false;
  lastPollTime = 0;
  tokenExpirationMs = 0;
  isInitialized = false;
  isTaskRunning = false;
  shouldStop = false;
  taskHandle = NULL;
  dataMutex = xSemaphoreCreateMutex();
}

void SpotifyClient::init() {
  if (!isInitialized) {
    wifiClient.setInsecure(); // Disable certificate validation for simplicity, or provide root CA
    isInitialized = true;
    shouldStop = false;
    
    if (!isTaskRunning) {
      isTaskRunning = true;
      xTaskCreatePinnedToCore(
        networkTask,
        "SpotifyTask",
        8192,
        this,
        1,
        &taskHandle,
        0 // Run on Core 0 (Network core)
      );
    }
  }
}

void SpotifyClient::stop() {
  shouldStop = true;
  isInitialized = false; // So it reinitializes on next loop()
}

void SpotifyClient::loop() {
  if (!isInitialized) {
    init();
  }
}

void SpotifyClient::networkTask(void* param) {
  SpotifyClient* client = (SpotifyClient*)param;
  
  while (true) {
    if (client->shouldStop) {
      client->wifiClient.stop();
      client->isTaskRunning = false;
      client->taskHandle = NULL;
      vTaskDelete(NULL); // Delete self cleanly
    }

    // If not configured, just wait and try again later
    if (config.getSpotifyClientId().isEmpty() || config.getSpotifyClientSecret().isEmpty() || config.getSpotifyRefreshToken().isEmpty()) {
      static bool configWarned = false;
      if (!configWarned) {
        Logger::println("[Spotify] Error: Missing Client ID, Secret, or Refresh Token in config!");
        configWarned = true;
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      continue;
    }

    // Poll interval
    if (millis() - client->lastPollTime >= client->POLL_INTERVAL || client->lastPollTime == 0) {
      client->lastPollTime = millis();
      
      // Check if token needs refresh
      if (millis() >= client->tokenExpirationMs || client->accessToken.isEmpty()) {
        if (!client->refreshAccessToken()) {
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          continue; // failed to refresh
        }
      }
      
      client->fetchCurrentlyPlaying();
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS); // Yield to watchdog
  }
}

bool SpotifyClient::refreshAccessToken() {
  
  String url = "https://accounts.spotify.com/api/token";
  String auth = config.getSpotifyClientId() + ":" + config.getSpotifyClientSecret();
  
  // Base64 encode the auth string
  size_t base64Len = ((auth.length() + 2) / 3) * 4;
  unsigned char* base64Buf = new unsigned char[base64Len + 1];
  size_t olen = 0;
  mbedtls_base64_encode(base64Buf, base64Len + 1, &olen, (const unsigned char*)auth.c_str(), auth.length());
  base64Buf[olen] = '\0'; // VERY IMPORTANT: null terminate the string!
  String encodedAuth = String((char*)base64Buf);
  delete[] base64Buf;
  
  httpClient.begin(wifiClient, url);
  httpClient.addHeader("Authorization", "Basic " + encodedAuth);
  httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
  httpClient.setReuse(false);
  
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
    Logger::printf("[Spotify] Token refresh failed, error: %d\n", httpCode);
    httpClient.end();
    return false;
  }
}

bool SpotifyClient::fetchCurrentlyPlaying() {
  
  String url = "https://api.spotify.com/v1/me/player/currently-playing";
  
  httpClient.begin(wifiClient, url);
  httpClient.addHeader("Authorization", "Bearer " + accessToken);
  httpClient.setReuse(false);
  
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
      if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
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
        xSemaphoreGive(dataMutex);
      }
    } else {
      Logger::print("[Spotify] JSON Parsing failed: ");
      Logger::println(error.c_str());
    }
  } else if (httpCode == 204) {
    // No content (nothing is playing)
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      data.isPlaying = false;
      data.isValid = true;
      data.lastUpdateMs = millis();
      xSemaphoreGive(dataMutex);
    }
  } else {
    Logger::printf("[Spotify] Fetch failed, error: %d\n", httpCode);
    if (httpCode == 401) {
      accessToken = ""; // Force refresh next time
    }
  }
  
  httpClient.end();
  return data.isValid;
}

SpotifyData SpotifyClient::getData() const {
  SpotifyData copy;
  if (dataMutex != NULL && xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    copy = data;
    xSemaphoreGive(dataMutex);
  } else {
    copy = data; // Fallback if mutex not initialized yet
  }
  return copy;
}
