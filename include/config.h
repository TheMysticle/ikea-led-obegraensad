#pragma once

#include <Arduino.h>
#include <string>
#include "constants.h"

#ifdef ENABLE_STORAGE
#include <Preferences.h>
#endif

class Config
{
private:
#ifdef ENABLE_STORAGE
  Preferences preferences;
  bool storageAvailable;
#endif
  
  // Current values with safe defaults
  String weatherLocation;
  String ntpServer;
  String tzInfo;
  String tessieVin;
  String tessieApiKey;
  bool autoStartSchedule;
  bool alexaEnabled;
  String alexaDeviceName;
  int doublePressPlugin;
  String spotifyClientId;
  String spotifyClientSecret;
  String spotifyRefreshToken;
  bool initialized;

public:
  Config();
  
  // Initialize configuration - always succeeds with defaults if storage fails
  void begin();
  
  // Load from storage (safe - uses defaults on failure)
  void load();
  
  // Save to storage (safe - fails silently)
  void save();
  
  // Reset to hardcoded defaults
  void setDefaults();
  
  // Getters (always return valid values)
  const String& getWeatherLocation() const;
  const String& getNtpServer() const;
  const String& getTzInfo() const;
  const String& getTessieVin() const;
  const String& getTessieApiKey() const;
  bool getAutoStartSchedule() const;
  bool getAlexaEnabled() const;
  const String& getAlexaDeviceName() const;
  int getDoublePressPlugin() const;
  const String& getSpotifyClientId() const;
  const String& getSpotifyClientSecret() const;
  const String& getSpotifyRefreshToken() const;
  bool isInitialized() const { return initialized; }
  
  // Setters with validation
  void setWeatherLocation(const String& location);
  void setNtpServer(const String& server);
  void setTzInfo(const String& tz);
  void setTessieVin(const String& vin);
  void setTessieApiKey(const String& key);
  void setAutoStartSchedule(bool autoStart);
  void setAlexaEnabled(bool enabled);
  void setAlexaDeviceName(const String& name);
  void setDoublePressPlugin(int pluginId);
  void setSpotifyClientId(const String& clientId);
  void setSpotifyClientSecret(const String& clientSecret);
  void setSpotifyRefreshToken(const String& refreshToken);
  
  // Export to JSON
  String toJson() const;
  
  // Import from JSON (validates input)
  bool fromJson(const String& json);
};

extern Config config;
