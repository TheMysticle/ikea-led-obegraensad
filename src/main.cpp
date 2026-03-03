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
#include "plugins/TixyPlugin.h"
#include "plugins/TessiePlugin.h"
#include "plugins/TetrisPlugin.h"
#include "plugins/FroggerPlugin.h"
#include "plugins/MazePlugin.h"

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

BfButton btn(BfButton::STANDALONE_DIGITAL, PIN_BUTTON, true, LOW);
uint8_t lastKnownBrightness = 128; // Default to 50% brightness

Espalexa espalexa; // Create Espalexa instance
// --- END ADDITIONS FOR ESPALEXA ---

// --- CUSTOM BUTTON CONFIGURATION ---
// Add the plugin IDs you want to cycle through with a SINGLE press.
std::vector<int> buttonCyclePlugins = {12, 16, 3, 7, 6, 11, 2}; 
int currentCycleIndex = 0;

// Define the plugin to activate on a DOUBLE press.
const int DOUBLE_PRESS_PLUGIN_ID = 19; // TessiePlugin

// NOTE: Long press is handled in the pressHandler to activate the persisted default plugin.
// --- END CUSTOM BUTTON CONFIGURATION ---

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

  wifiManager.setConnectRetries(5);
  wifiManager.setConnectTimeout(5);
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
  // Any button press should stop the scheduler and allow manual control.
  if (Scheduler.isActive) {
    Scheduler.stop();
  }

  // Get the currently active plugin
  Plugin *activePlugin = pluginManager.getActivePlugin();

  switch (pattern)
  {
  case BfButton::SINGLE_PRESS:
    if (currentStatus != LOADING && !buttonCyclePlugins.empty())
    {
      currentCycleIndex = (currentCycleIndex + 1) % buttonCyclePlugins.size();
      int pluginToActivate = buttonCyclePlugins[currentCycleIndex];
      pluginManager.setActivePluginById(pluginToActivate);
    }
    break;

  case BfButton::DOUBLE_PRESS:
     if (currentStatus != LOADING)
    {
      pluginManager.setActivePluginById(DOUBLE_PRESS_PLUGIN_ID);
    }
    break;

  case BfButton::LONG_PRESS:
    if (currentStatus != LOADING && activePlugin != nullptr)
    {
      // Check if the Tixy plugin is currently active
      if (strcmp(activePlugin->getName(), "TixyLand") == 0)
      {
        // Cast the active plugin to our TixyPlugin type and call the new method
        static_cast<TixyPlugin*>(activePlugin)->nextPreset();
      }
      else
      {
        // If it's not the Tixy plugin, perform the default long-press action
        pluginManager.activatePersistedPlugin();
      }
    }
    break;
  }
}

// --- ADDITION FOR ESPALEXA ---
// Callback function for Alexa control
void setLedWallPower(uint8_t brightness)
{
    Serial.print("Alexa command received. New brightness: ");
    Serial.println(brightness);

    // This is the "Turn Off" command
    if (brightness == 0)
    {
        // Get the brightness from the screen class BEFORE we turn it off
        uint8_t currentBrightness = Screen.getCurrentBrightness();
        if (currentBrightness > 0)
        {
            // Save the current brightness level into our runtime variable
            lastKnownBrightness = currentBrightness;
            Serial.print("Saving last known brightness for this session: ");
            Serial.println(lastKnownBrightness);
        }
        
        Screen.clear(); // Clear the display buffer
        // Set brightness to 0 and PERSIST this "off" state
        Screen.setBrightness(0, true);
        if (Scheduler.isActive) {
            Scheduler.isBrightnessOverridden = true;
        }
    }
    // This is the "Turn On" or "Dim" command
    else
    {
        // This is a generic "Turn On" command (value=255) from a fully off state
        if (Screen.getCurrentBrightness() == 0 && brightness == 255)
        {
            Serial.print("Restoring last known brightness: ");
            Serial.println(lastKnownBrightness);
            // Restore the saved brightness instead of using Alexa's 255, and PERSIST it
            Screen.setBrightness(lastKnownBrightness, true);
        }
        else
        {
            // This is a specific dimming command (e.g., "set to 30%").
            // Use the value from Alexa and PERSIST it.
            Serial.print("Setting specific brightness to: ");
            Serial.println(brightness);
            Screen.setBrightness(brightness, true);
            
            // Also update our runtime variable with this new specific value
            lastKnownBrightness = brightness;
        }
        if (Scheduler.isActive) {
            Scheduler.isBrightnessOverridden = true;
        }
    }
}


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
  pluginManager.addPlugin(new TixyPlugin());

#ifdef ENABLE_SERVER
  pluginManager.addPlugin(new BigClockPlugin());
  pluginManager.addPlugin(new ClockPlugin());
  pluginManager.addPlugin(new PongClockPlugin());
  pluginManager.addPlugin(new TickingClockPlugin());
  pluginManager.addPlugin(new WeatherPlugin());
  pluginManager.addPlugin(new AnimationPlugin());
  pluginManager.addPlugin(new DDPPlugin());
  pluginManager.addPlugin(new TessiePlugin());
  pluginManager.addPlugin(new TetrisPlugin());
  pluginManager.addPlugin(new FroggerPlugin());
  pluginManager.addPlugin(new MazePlugin());
#endif

  pluginManager.init();
  Scheduler.init();

#ifdef ENABLE_STORAGE
    // Sync our runtime brightness variable with the value loaded from storage
    uint8_t storedBrightness = Screen.getCurrentBrightness();
    if (storedBrightness > 0) {
        lastKnownBrightness = storedBrightness;
    }
#endif

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

#include <WiFiUdp.h>
WiFiUDP remoteUdp;
char remotePacketBuffer[64];

void checkForRemotePluginSwitch() {
    int packetSize = remoteUdp.parsePacket();
    if (packetSize) {
        int len = remoteUdp.read(remotePacketBuffer, sizeof(remotePacketBuffer) - 1);
        if (len > 0) remotePacketBuffer[len] = 0;
        if (strncmp(remotePacketBuffer, "PLUGIN:", 7) == 0) {
            int pluginId = atoi(remotePacketBuffer + 7);
            pluginManager.setActivePluginById(pluginId);
            Serial.print("Remote plugin switch to ID: ");
            Serial.println(pluginId);
        }
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
      remoteUdp.begin(4211); // Pick a port for plugin switching
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

  if (currentStatus == NONE || currentStatus == SCROLLING)
  {
    if (currentStatus == NONE) Scheduler.update();

    if (Messages.hasMessages())
    {
      currentStatus = SCROLLING;
      Messages.scroll();
      if (!Messages.hasMessages() && Messages.wasScrolling())
      {
        currentStatus = NONE;
        Messages.clearScrollingFlag();
      }
    }
    else if (Messages.wasScrolling())
    {
      currentStatus = NONE;
      Messages.clearScrollingFlag();
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
  checkForRemotePluginSwitch();
  delay(10);
}