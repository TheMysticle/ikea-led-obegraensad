#include "plugins/WeatherPlugin.h"
#include "config.h"

// https://github.com/chubin/wttr.in/blob/master/share/translations/en.txt
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
WiFiClient wiFiClient;
#endif

void WeatherPlugin::setup()
{
  Screen.clear();

  // If we have cached data and it's still fresh (< 30 minutes old), redraw it
  if (hasCachedData && lastUpdate > 0 && millis() >= lastUpdate &&
      millis() - lastUpdate < (1000UL * 60 * 30))
  {
    Serial.println("Using cached weather data");
    drawWeather();
  }
  else
  {
    // Show loading screen - data needs to be fetched
    isLoading = true;
    hasError = false;
    this->update();
  }
}

void WeatherPlugin::loop()
{
  if (isLoading) {
    // Draw loading animation
    currentStatus = LOADING;
    Screen.drawLoadingAnimation(13);
    currentStatus = NONE;
    return;
  }

  if (hasError) {
    Screen.clear();
    Screen.setPixel(7, 4, 1); Screen.setPixel(8, 4, 1);
    Screen.setPixel(7, 5, 1); Screen.setPixel(8, 5, 1);
    Screen.setPixel(7, 6, 1); Screen.setPixel(8, 6, 1);
    Screen.setPixel(7, 7, 1); Screen.setPixel(8, 7, 1);
    Screen.setPixel(7, 8, 1); Screen.setPixel(8, 8, 1);
    Screen.setPixel(7, 10, 1); Screen.setPixel(8, 10, 1);
    Screen.setPixel(7, 11, 1); Screen.setPixel(8, 11, 1);
    
    // Retry automatically after a while or allow it to remain errored
    if (millis() >= this->lastUpdate + (1000 * 60 * 5)) { // retry after 5 mins
      this->update();
    }
    return;
  }

  if (millis() >= this->lastUpdate + (1000 * 60 * 30))
  {
    this->update();
    Serial.println("updating weather");
  }

  if (dataReadyToDraw) {
    dataReadyToDraw = false;
    drawWeather();
  }
}

void WeatherPlugin::update()
{
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected, skipping weather update");
    return;
  }

  isLoading = true;
  hasError = false;
  this->lastUpdate = millis(); // Mark as updated so it doesn't spam

#ifdef ESP32
  if (networkTaskHandle == NULL) {
    xTaskCreatePinnedToCore(
      networkTaskFunction,
      "WeatherTask",
      8192,
      this,
      1,
      &networkTaskHandle,
      0 // Run on Core 0
    );
  }
#else
  networkTaskFunction(this);
#endif
}

void WeatherPlugin::networkTaskFunction(void *pvParameters)
{
  WeatherPlugin *plugin = (WeatherPlugin *)pvParameters;
  
  String weatherLocation = config.getWeatherLocation();
  Serial.print("[WeatherPlugin] Fetching weather for configured city: ");
  Serial.println(weatherLocation);
  
  String weatherApiString = "https://wttr.in/" + weatherLocation + "?format=j2&lang=en";
  Serial.print("[WeatherPlugin] API request: ");
  Serial.println(weatherApiString);

#ifdef ESP32
  plugin->secureClient.setInsecure();
  plugin->http.begin(plugin->secureClient, weatherApiString);
#endif
#ifdef ESP8266
  plugin->http.begin(plugin->wiFiClient, weatherApiString);
#endif

  plugin->http.setTimeout(20000);

  Serial.println("Sending HTTP GET request...");
  int code = plugin->http.GET();
  Serial.print("HTTP response code: ");
  Serial.println(code);

  if (code == HTTP_CODE_OK)
  {
    String payload = plugin->http.getString();
    Serial.print("Response size: ");
    Serial.println(payload.length());

    JsonDocument filter;
    filter["current_condition"][0]["temp_C"] = true;
    filter["current_condition"][0]["weatherCode"] = true;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

    if (error)
    {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      plugin->http.end();
#ifdef ESP32
      plugin->secureClient.stop();
#endif
      plugin->hasError = true;
      plugin->isLoading = false;
#ifdef ESP32
      plugin->networkTaskHandle = NULL;
      vTaskDelete(NULL);
#endif
      return;
    }

    int temperature = round(doc["current_condition"][0]["temp_C"].as<float>());
    int weatherCode = doc["current_condition"][0]["weatherCode"].as<int>();
    int weatherIcon = 0;
    int iconY = 1;
    int tempY = 10;

    if (std::find(plugin->thunderCodes.begin(), plugin->thunderCodes.end(), weatherCode) != plugin->thunderCodes.end())
    {
      weatherIcon = 1;
    }
    else if (std::find(plugin->rainCodes.begin(), plugin->rainCodes.end(), weatherCode) != plugin->rainCodes.end())
    {
      weatherIcon = 4;
    }
    else if (std::find(plugin->snowCodes.begin(), plugin->snowCodes.end(), weatherCode) != plugin->snowCodes.end())
    {
      weatherIcon = 5;
    }
    else if (std::find(plugin->fogCodes.begin(), plugin->fogCodes.end(), weatherCode) != plugin->fogCodes.end())
    {
      weatherIcon = 6;
      iconY = 2;
    }
    else if (std::find(plugin->clearCodes.begin(), plugin->clearCodes.end(), weatherCode) != plugin->clearCodes.end())
    {
      weatherIcon = 2;
      iconY = 1;
      tempY = 9;
    }
    else if (std::find(plugin->cloudyCodes.begin(), plugin->cloudyCodes.end(), weatherCode) != plugin->cloudyCodes.end())
    {
      weatherIcon = 0;
      iconY = 2;
      tempY = 9;
    }
    else if (std::find(plugin->partyCloudyCodes.begin(), plugin->partyCloudyCodes.end(), weatherCode) !=
             plugin->partyCloudyCodes.end())
    {
      weatherIcon = 3;
      iconY = 2;
    }

    // Cache the weather data
    plugin->hasCachedData = true;
    plugin->cachedTemperature = temperature;
    plugin->cachedWeatherIcon = weatherIcon;
    plugin->cachedIconY = iconY;
    plugin->cachedTempY = tempY;

    // Signal loop to draw the newly fetched data
    plugin->dataReadyToDraw = true;
  }
  else
  {
    Serial.print("HTTP request failed with code: ");
    Serial.println(code);
    plugin->hasError = true;
  }

  plugin->http.end();
#ifdef ESP32
  plugin->secureClient.stop();
#endif

  plugin->isLoading = false;
#ifdef ESP32
  plugin->networkTaskHandle = NULL;
  vTaskDelete(NULL);
#endif
}

void WeatherPlugin::teardown()
{
  isLoading = false;
  hasError = false;
}

void WeatherPlugin::drawWeather()
{
  Screen.clear();
  Screen.drawWeather(0, cachedIconY, cachedWeatherIcon, 100);

  int temperature = cachedTemperature;
  int tempY = cachedTempY;

  if (temperature >= 10)
  {
    Screen.drawCharacter(9, tempY, Screen.readBytes(degreeSymbol), 4, 50);
    Screen.drawNumbers(1, tempY, {(temperature - temperature % 10) / 10, temperature % 10});
  }
  else if (temperature <= -10)
  {
    Screen.drawCharacter(0, tempY, Screen.readBytes(minusSymbol), 4);
    Screen.drawCharacter(11, tempY, Screen.readBytes(degreeSymbol), 4, 50);
    temperature *= -1;
    Screen.drawNumbers(3, tempY, {(temperature - temperature % 10) / 10, temperature % 10});
  }
  else if (temperature >= 0)
  {
    Screen.drawCharacter(7, tempY, Screen.readBytes(degreeSymbol), 4, 50);
    Screen.drawNumbers(4, tempY, {temperature});
  }
  else
  {
    Screen.drawCharacter(0, tempY, Screen.readBytes(minusSymbol), 4);
    Screen.drawCharacter(9, tempY, Screen.readBytes(degreeSymbol), 4, 50);
    Screen.drawNumbers(3, tempY, {-temperature});
  }
}

const char *WeatherPlugin::getName() const
{
  return "Weather";
}
