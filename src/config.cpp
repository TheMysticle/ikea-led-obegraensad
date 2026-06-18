#include "config.h"
#include <ArduinoJson.h>
#include "crypto.h"

Config config;


Config::Config()
{
  initialized = false;
#ifdef ENABLE_STORAGE
  storageAvailable = false;
#endif
  setDefaults();
}


void Config::begin()
{
  Serial.println("[Config] Initializing configuration system...");
  
  // Set safe defaults first
  setDefaults();
  
#ifdef ENABLE_STORAGE
  // Try to initialize storage
  try {
    if (preferences.begin("config", false)) {
      storageAvailable = true;
      Serial.println("[Config] Storage initialized successfully");
      
      // Try to load saved configuration
      load();
    } else {
      Serial.println("[Config] Warning: Could not initialize storage, using defaults");
      storageAvailable = false;
    }
  } catch (...) {
    Serial.println("[Config] Error: Exception during storage init, using defaults");
    storageAvailable = false;
  }
#else
  Serial.println("[Config] Storage disabled, using defaults");
#endif
  
  initialized = true;
  
  Serial.println("[Config] ============================================");
  Serial.println("[Config] Current Configuration:");
  Serial.print("[Config] Weather Location: ");
  Serial.println(weatherLocation);
  Serial.print("[Config] NTP Server: ");
  Serial.println(ntpServer);
  Serial.print("[Config] Timezone: ");
  Serial.println(tzInfo);
  Serial.print("[Config] Auto-Start Schedule: ");
  Serial.println(autoStartSchedule ? "enabled" : "disabled");
  Serial.println("[Config] ============================================");
}

void Config::setDefaults()
{
  // Use constants from constants.h as defaults
  weatherLocation = String(WEATHER_LOCATION);
  ntpServer = String(NTP_SERVER);
  tzInfo = String(TZ_INFO);
  tessieVin = "";
  tessieApiKey = "";
  autoStartSchedule = false;
  alexaEnabled = false;
  alexaDeviceName = "LED Wall";
  doublePressPlugin = 19;
  spotifyClientId = "";
  spotifyClientSecret = "";
  spotifyRefreshToken = "";
  crashReportingEnabled = false;
}

void Config::load()
{
#ifdef ENABLE_STORAGE
  if (!storageAvailable) {
    Serial.println("[Config] Storage not available, keeping defaults");
    return;
  }
  
  try {
    // Load with fallback to defaults
    weatherLocation = preferences.getString("weatherLoc", String(WEATHER_LOCATION));
    ntpServer = preferences.getString("ntpServer", String(NTP_SERVER));
    tzInfo = preferences.getString("tzInfo", String(TZ_INFO));
    tessieVin = preferences.getString("tessieVin", "");
    String encryptedKey = preferences.getString("tessieKey", "");
    tessieApiKey = encryptedKey.length() > 0 ? crypto.decryptString(encryptedKey) : "";
    autoStartSchedule = preferences.getBool("autoSchedule", false);
    alexaEnabled = preferences.getBool("alexaEnabled", false);
    alexaDeviceName = preferences.getString("alexaName", "LED Wall");
    doublePressPlugin = preferences.getInt("dblPrssPlugin", 19);
    
    spotifyClientId = preferences.getString("spClientId", "");
    String encSpSecret = preferences.getString("spClientSec", "");
    spotifyClientSecret = encSpSecret.length() > 0 ? crypto.decryptString(encSpSecret) : "";
    String encSpRefresh = preferences.getString("spRefresh", "");
    spotifyRefreshToken = encSpRefresh.length() > 0 ? crypto.decryptString(encSpRefresh) : "";
    
    crashReportingEnabled = preferences.getBool("crashRep", false);
    
    Serial.println("[Config] Configuration loaded from storage");
  } catch (...) {
    Serial.println("[Config] Error loading config, using defaults");
    setDefaults();
  }
#endif
}

void Config::save()
{
#ifdef ENABLE_STORAGE
  if (!storageAvailable) {
    Serial.println("[Config] Storage not available, cannot save");
    return;
  }
  
  try {
    preferences.putString("weatherLoc", weatherLocation);
    preferences.putString("ntpServer", ntpServer);
    preferences.putString("tzInfo", tzInfo);
    preferences.putString("tessieVin", tessieVin);
    preferences.putString("tessieKey", tessieApiKey.length() > 0 ? crypto.encryptString(tessieApiKey) : "");
    preferences.putBool("autoSchedule", autoStartSchedule);
    preferences.putBool("alexaEnabled", alexaEnabled);
    preferences.putString("alexaName", alexaDeviceName);
    preferences.putInt("dblPrssPlugin", doublePressPlugin);
    
    preferences.putString("spClientId", spotifyClientId);
    preferences.putString("spClientSec", spotifyClientSecret.length() > 0 ? crypto.encryptString(spotifyClientSecret) : "");
    preferences.putString("spRefresh", spotifyRefreshToken.length() > 0 ? crypto.encryptString(spotifyRefreshToken) : "");
    
    preferences.putBool("crashRep", crashReportingEnabled);
    
    Serial.println("[Config] Configuration saved");
  } catch (...) {
    Serial.println("[Config] Error: Could not save configuration");
  }
#else
  Serial.println("[Config] Storage disabled, cannot save");
#endif
}


const String& Config::getWeatherLocation() const
{
  return weatherLocation;
}

const String& Config::getNtpServer() const
{
  return ntpServer;
}

const String& Config::getTzInfo() const
{
  return tzInfo;
}

const String& Config::getTessieVin() const
{
  return tessieVin;
}

const String& Config::getTessieApiKey() const
{
  return tessieApiKey;
}

bool Config::getAutoStartSchedule() const
{
  return autoStartSchedule;
}

bool Config::getAlexaEnabled() const
{
  return alexaEnabled;
}

const String& Config::getAlexaDeviceName() const
{
  return alexaDeviceName;
}

int Config::getDoublePressPlugin() const
{
  return doublePressPlugin;
}

const String& Config::getSpotifyClientId() const
{
  return spotifyClientId;
}

const String& Config::getSpotifyClientSecret() const
{
  return spotifyClientSecret;
}

const String& Config::getSpotifyRefreshToken() const
{
  return spotifyRefreshToken;
}

void Config::setWeatherLocation(const String& location)
{
  if (location.length() > 0 && location.length() < 100) {
    weatherLocation = location;
  }
}


void Config::setNtpServer(const String& server)
{
  if (server.length() > 0 && server.length() < 100) {
    ntpServer = server;
  }
}

void Config::setTzInfo(const String& tz)
{
  if (tz.length() > 0 && tz.length() < 100) {
    tzInfo = tz;
  }
}

void Config::setTessieVin(const String& vin)
{
  tessieVin = vin;
}

void Config::setTessieApiKey(const String& key)
{
  tessieApiKey = key;
}

void Config::setAutoStartSchedule(bool autoStart)
{
  autoStartSchedule = autoStart;
}

void Config::setAlexaEnabled(bool enabled)
{
  alexaEnabled = enabled;
}

void Config::setAlexaDeviceName(const String& name)
{
  if (name.length() > 0 && name.length() < 50) {
    alexaDeviceName = name;
  }
}

void Config::setDoublePressPlugin(int pluginId)
{
  doublePressPlugin = pluginId;
}

void Config::setSpotifyClientId(const String& clientId)
{
  spotifyClientId = clientId;
}

void Config::setSpotifyClientSecret(const String& clientSecret)
{
  spotifyClientSecret = clientSecret;
}

void Config::setSpotifyRefreshToken(const String& refreshToken)
{
  spotifyRefreshToken = refreshToken;
}

void Config::setCrashReportingEnabled(bool enabled)
{
  crashReportingEnabled = enabled;
}


String Config::toJson() const
{
  JsonDocument doc;
  doc["weatherLocation"] = weatherLocation;
  doc["ntpServer"] = ntpServer;
  doc["tzInfo"] = tzInfo;
  doc["tessieVin"] = tessieVin;
  doc["tessieApiKey"] = tessieApiKey.length() > 0 ? "sk_live_************************" : "";
  doc["autoStartSchedule"] = autoStartSchedule;
  doc["alexaEnabled"] = alexaEnabled;
  doc["alexaDeviceName"] = alexaDeviceName;
  doc["doublePressPlugin"] = doublePressPlugin;
  doc["spotifyClientId"] = spotifyClientId;
  doc["spotifyClientSecret"] = spotifyClientSecret.length() > 0 ? "sk_live_************************" : "";
  doc["spotifyRefreshToken"] = spotifyRefreshToken.length() > 0 ? "sk_live_************************" : "";
  doc["crashReportingEnabled"] = crashReportingEnabled;
  
  String output;
  serializeJson(doc, output);
  return output;
}


bool Config::fromJson(const String& json)
{
  if (json.length() == 0) {
    Serial.println("[Config] Empty JSON");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print("[Config] JSON parse error: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Validate and set each field
  if (doc["weatherLocation"].is<String>()) {
    String loc = doc["weatherLocation"].as<String>();
    if (loc.length() > 0 && loc.length() < 100) {
      weatherLocation = loc;
    }
  }
  
  if (doc["ntpServer"].is<String>()) {
    String ntp = doc["ntpServer"].as<String>();
    if (ntp.length() > 0 && ntp.length() < 100) {
      ntpServer = ntp;
    }
  }
  
  if (doc["tzInfo"].is<String>()) {
    String tz = doc["tzInfo"].as<String>();
    if (tz.length() > 0 && tz.length() < 100) {
      tzInfo = tz;
    }
  }
  
  if (doc["tessieVin"].is<String>()) {
    tessieVin = doc["tessieVin"].as<String>();
  }
  
  if (doc["tessieApiKey"].is<String>()) {
    String incomingKey = doc["tessieApiKey"].as<String>();
    if (incomingKey != "sk_live_************************") {
      tessieApiKey = incomingKey;
    }
  }
  
  if (doc["autoStartSchedule"].is<bool>()) {
    autoStartSchedule = doc["autoStartSchedule"].as<bool>();
  }
  
  if (doc["alexaEnabled"].is<bool>()) {
    alexaEnabled = doc["alexaEnabled"].as<bool>();
  }
  
  if (doc["alexaDeviceName"].is<String>()) {
    String name = doc["alexaDeviceName"].as<String>();
    if (name.length() > 0 && name.length() < 50) {
      alexaDeviceName = name;
    }
  }
  
  if (doc["doublePressPlugin"].is<int>()) {
    doublePressPlugin = doc["doublePressPlugin"].as<int>();
  }
  
  if (doc["spotifyClientId"].is<String>()) {
    spotifyClientId = doc["spotifyClientId"].as<String>();
  }
  
  if (doc["spotifyClientSecret"].is<String>()) {
    String incomingSecret = doc["spotifyClientSecret"].as<String>();
    if (incomingSecret != "sk_live_************************") {
      spotifyClientSecret = incomingSecret;
    }
  }
  
  if (doc["spotifyRefreshToken"].is<String>()) {
    String incomingRefresh = doc["spotifyRefreshToken"].as<String>();
    if (incomingRefresh != "sk_live_************************") {
      spotifyRefreshToken = incomingRefresh;
    }
  }
  
  if (doc["crashReportingEnabled"].is<bool>()) {
    crashReportingEnabled = doc["crashReportingEnabled"].as<bool>();
  }
  
  return true;
}

