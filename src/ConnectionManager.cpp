#include "ConnectionManager.h"
#include <WiFiManager.h>
#ifdef ESP32
#include <ESPmDNS.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#include "config.h"
#include "secrets.h"
#include "screen.h"
#include "Logger.h"

WiFiManager wifiManager;

unsigned long lastConnectionAttempt = 0;
const unsigned long connectionInterval = 10000;
unsigned long reconnectionBackoff = 5000;            // Start with 5 seconds
const unsigned long maxReconnectionBackoff = 300000; // Max 5 minutes
uint8_t reconnectionAttempts = 0;

void ConnectionManager::init() {
    // Initialization if needed
}

void ConnectionManager::connectToWiFi()
{
  // if a WiFi setup AP was started, reboot is required to clear routes
  bool wifiWebServerStarted = false;
  wifiManager.setWebServerCallback([&wifiWebServerStarted]() { wifiWebServerStarted = true; });

  wifiManager.setHostname(WIFI_HOSTNAME);

#if defined(IP_ADDRESS) && defined(GWY) && defined(SUBNET) && defined(DNS1)
  auto ip = IPAddress();
  ip.fromString(IP_ADDRESS);

  auto gwy = IPAddress();
  gwy.fromString(GWY);

  auto subnet = IPAddress();
  subnet.fromString(SUBNET);

  auto dns = IPAddress();
  dns.fromString(DNS1);

  wifiManager.setSTAStaticIPConfig(ip, gwy, subnet, dns);
#endif

  wifiManager.setConnectRetries(5);
  wifiManager.setConnectTimeout(5);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setWiFiAutoReconnect(true);

  // Setup animated loading screen during connect
#ifdef ESP32
  TaskHandle_t loadingTaskHandle = NULL;
  xTaskCreate([](void *pvParameters) {
    while(true) {
      Screen.drawLoadingAnimation(7); // Center it vertically
      vTaskDelay(pdMS_TO_TICKS(30));
    }
  }, "LoadingTask", 2048, NULL, 1, &loadingTaskHandle);
#else
  Screen.drawLoadingAnimation(7);
#endif

  wifiManager.autoConnect(WIFI_MANAGER_SSID);

  // Stop animation
#ifdef ESP32
  if (loadingTaskHandle != NULL) {
    vTaskDelete(loadingTaskHandle);
  }
#endif
  Screen.clear();

#ifdef ESP32
  if (MDNS.begin(WIFI_HOSTNAME))
  {
    MDNS.addService("http", "tcp", 80);
    MDNS.setInstanceName(WIFI_HOSTNAME);
  }
  else
  {
    Serial.println("Could not start mDNS!");
  }
#endif

  if (wifiWebServerStarted)
  {
    // Reboot required, otherwise wifiManager server interferes with our server
    Logger::println("Done running WiFi Manager webserver - rebooting");
    ESP.restart();
  }

  lastConnectionAttempt = millis();
}

void ConnectionManager::checkWiFiConnection()
{
  // Check WiFi less frequently with exponential backoff
  if (WiFi.status() != WL_CONNECTED)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - lastConnectionAttempt >= reconnectionBackoff)
    {
      Logger::println("WiFi disconnected, attempting reconnection...");
      connectToWiFi();

      // Exponential backoff: double the wait time, up to max
      reconnectionAttempts++;
      reconnectionBackoff = min(reconnectionBackoff * 2, maxReconnectionBackoff);
    }
  }
  else
  {
    if (reconnectionAttempts > 0)
    {
      Logger::println("WiFi reconnected successfully");
      reconnectionAttempts = 0;
      reconnectionBackoff = 5000;
    }
  }
}
