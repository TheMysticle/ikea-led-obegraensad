#include <Arduino.h>
#include <SPI.h>
#include <BfButton.h>

#ifdef ESP82666
/* Fix duplicate defs of HTTP_GET, HTTP_POST, ... in ESPAsyncWebServer.h */
#define WEBSERVER_H
#endif
#include <WiFiManager.h>

#ifdef ESP32
#include <ESPmDNS.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "PluginManager.h"
#include "scheduler.h"

#include "plugins/BreakoutPlugin.h"
#include "plugins/CirclePlugin.h"
#include "plugins/DrawPlugin.h"
#include "plugins/FireworkPlugin.h"
#include "plugins/GameOfLifePlugin.h"
#include "plugins/LinesPlugin.h"
#include "plugins/OffPlugin.h"
#include "plugins/RainPlugin.h"
#include "plugins/SnakePlugin.h"
#include "plugins/StarsPlugin.h"
#include "plugins/PongClockPlugin.h"
#include "plugins/TickingClockPlugin.h"
#include "plugins/DDPPlugin.h"

#ifdef ENABLE_SERVER
#include "plugins/AnimationPlugin.h"
#include "plugins/BigClockPlugin.h"
#include "plugins/ClockPlugin.h"
#include "plugins/WeatherPlugin.h"
#include "plugins/AnimationPlugin.h"
#endif

#include "asyncwebserver.h"
#include "ota.h"
#include "screen.h"
#include "secrets.h"
#include "websocket.h"
#include "messages.h"

// --- ADDITIONS FOR ESPALEXA ---
#define ESPALEXA_ASYNC // Important: Define this before including Espalexa.h!
#include <Espalexa.h>

Espalexa espalexa; // Create Espalexa instance
// --- END ADDITIONS FOR ESPALEXA ---

BfButton btn(BfButton::STANDALONE_DIGITAL, PIN_BUTTON, true, LOW);

unsigned long previousMillis = 0;
unsigned long interval = 30000;

PluginManager pluginManager;
SYSTEM_STATUS currentStatus = NONE;
WiFiManager wifiManager;

unsigned long lastConnectionAttempt = 0;
const unsigned long connectionInterval = 10000;

void connectToWiFi()
{
  // if a WiFi setup AP was started, reboot is required to clear routes
  bool wifiWebServerStarted = false;
  wifiManager.setWebServerCallback(
      [&wifiWebServerStarted]()
      { wifiWebServerStarted = true; });

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

  wifiManager.setConnectRetries(10);
  wifiManager.setConnectTimeout(10);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect(WIFI_MANAGER_SSID);

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
    Serial.println("Done running WiFi Manager webserver - rebooting");
    ESP.restart();
  }

  lastConnectionAttempt = millis();
}

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
  switch (pattern)
  {
  case BfButton::SINGLE_PRESS:
    if (currentStatus != LOADING)
    {
      Scheduler.clearSchedule();
      pluginManager.activateNextPlugin();
    }
    break;

  case BfButton::LONG_PRESS:
    if (currentStatus != LOADING)
    {
      pluginManager.activatePersistedPlugin();
    }
    break;
  }
}

// --- ADDITION FOR ESPALEXA ---
// Callback function for Alexa control
void setLedWallPower(uint8_t brightness)
{
  Serial.print("LED Wall brightness changed to: ");
  Serial.println(brightness);

  if (brightness == 0)
  {
    Screen.clear(); // Clear the screen when off
    Screen.setBrightness(0, true); // Set brightness to 0 and persist
  }
  else
  {
    // If turning on or dimming, set brightness and ensure a plugin is running
    Screen.setBrightness(brightness, true); // Set new brightness and persist
    // You might want to reactivate the last active plugin or a default one
    // if the screen was completely off.
    // For simplicity, we'll just ensure brightness is set.
    // If pluginManager.runActivePlugin() is called in loop(), it will display.
    // If not, you might need to call pluginManager.activatePersistedPlugin();
    // or pluginManager.activateNextPlugin(); here or start a default plugin.
  }
}
// --- END ADDITION FOR ESPALEXA ---


void baseSetup()
{
  Serial.begin(115200);

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);

#if !defined(ESP32) && !defined(ESP8266)
  Screen.setup();
#endif

// server
#ifdef ENABLE_SERVER
  connectToWiFi();

  // set time server
  configTzTime(TZ_INFO, NTP_SERVER);

  initOTA(server);
  initWebsocketServer(server);
  initWebServer();

  // --- ADDITIONS FOR ESPALEXA ---
  // Define your Alexa device
  espalexa.addDevice("LED Wall", setLedWallPower); // "LED Wall" is the name Alexa will recognize
  espalexa.begin(&server); // Pass your existing AsyncWebServer instance to Espalexa
  // --- END ADDITIONS FOR ESPALEXA ---

#endif
  pluginManager.addPlugin(new DrawPlugin());
  pluginManager.addPlugin(new BreakoutPlugin());
  pluginManager.addPlugin(new SnakePlugin());
  pluginManager.addPlugin(new GameOfLifePlugin());
  pluginManager.addPlugin(new StarsPlugin());
  pluginManager.addPlugin(new LinesPlugin());
  pluginManager.addPlugin(new CirclePlugin());
  pluginManager.addPlugin(new RainPlugin());
  pluginManager.addPlugin(new FireworkPlugin());
  pluginManager.addPlugin(new OffPlugin());

#ifdef ENABLE_SERVER
  pluginManager.addPlugin(new BigClockPlugin());
  pluginManager.addPlugin(new ClockPlugin());
  pluginManager.addPlugin(new PongClockPlugin());
  pluginManager.addPlugin(new TickingClockPlugin());
  pluginManager.addPlugin(new WeatherPlugin());
  pluginManager.addPlugin(new AnimationPlugin());
  pluginManager.addPlugin(new DDPPlugin());
#endif

  pluginManager.init();
  Scheduler.init();

  btn.onPress(pressHandler)
      .onDoublePress(pressHandler)
      .onPressFor(pressHandler, 1000);
}

#ifdef ESP32
TaskHandle_t screenDrawingTaskHandle = NULL;

void screenDrawingTask(void *parameter)
{
  Screen.setup();
  for (;;)
  {
    pluginManager.runActivePlugin();
    vTaskDelay(10);
  }
}

void setup()
{
  baseSetup();
  xTaskCreatePinnedToCore(
      screenDrawingTask,
      "screenDrawingTask",
      10000,
      NULL,
      1,
      &screenDrawingTaskHandle,
      0);
}
#endif
#ifdef ESP8266
#include <Scheduler.h>
void screenDrawingTask()
{
  Screen.setup();
  pluginManager.runActivePlugin();
  yield();
}

void setup()
{
  baseSetup();
  Scheduler.start(&screenDrawingTask);
}
#endif

void loop()
{
  static uint8_t taskCounter = 0;
  const unsigned long currentMillis = millis();
  btn.read();

#if !defined(ESP32) && !defined(ESP8266)
  pluginManager.runActivePlugin();
#endif

  if (currentStatus == NONE)
  {
    Scheduler.update();

    if ((taskCounter % 4) == 0)
    {
      Messages.scrollMessageEveryMinute();
    }
  }

  if ((taskCounter % 16) == 0)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      connectToWiFi();
    }
  }

  taskCounter++;
  if (taskCounter > 16)
  {
    taskCounter = 0;
  }

#ifdef ENABLE_SERVER
  cleanUpClients();
  espalexa.loop(); // --- ADDITION FOR ESPALEXA --- Run Espalexa's loop function ---
  ElegantOTA.loop();
#endif
  delay(1);
}