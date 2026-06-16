#include <Arduino.h>
#include <BfButton.h>
#include <SPI.h>

#ifdef ESP8266
/* Fix duplicate defs of HTTP_GET, HTTP_POST, ... in ESPAsyncWebServer.h */
#define WEBSERVER_H
#endif

#ifdef ESP32
#include <ESPmDNS.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "PluginManager.h"
#include "config.h"
#include "Logger.h"
#include "scheduler.h"

// Included new Managers
#include "ConnectionManager.h"
#include "AlexaManager.h"
#include "PluginRegistration.h"
#include "plugins/TixyPlugin.h"

#include "asyncwebserver.h"
#include "messages.h"
#include "ota.h"
#include "screen.h"
#include "secrets.h"
#include "websocket.h"

BfButton btn(BfButton::STANDALONE_DIGITAL, PIN_BUTTON, true, LOW);

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
#ifdef ESP32
DRAM_ATTR volatile SYSTEM_STATUS currentStatus = NONE;
#else
volatile SYSTEM_STATUS currentStatus = NONE;
#endif

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

  // Initialize configuration system (always safe)
  config.begin();

// server
#ifdef ENABLE_SERVER
  ConnectionManager::connectToWiFi();

  // set time server using config values
  configTzTime(config.getTzInfo().c_str(), config.getNtpServer().c_str());

  initOTA(server);
  initWebsocketServer(server);
  initWebServer();

  if (config.getAlexaEnabled()) {
    AlexaManager::init(&server);
  } else {
    server.begin(); // Espalexa usually starts the server, so we must start it manually if disabled
  }
#endif

  registerAllPlugins();

  Screen.clear();
  pluginManager.init();
  Scheduler.init();

#ifdef ENABLE_STORAGE
    // Sync our runtime brightness variable with the value loaded from storage
      uint8_t storedBrightness = Screen.getCurrentBrightness();
      if (storedBrightness > 0)
      {
        Screen.setBrightness(storedBrightness, false);
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
    vTaskDelay(1);
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
void screenDrawingTask()
{
  Screen.setup();
  pluginManager.runActivePlugin();
  yield();
}

void setup()
{
  baseSetup();
  Scheduler.start();
}
#endif

void loop()
{
  static uint8_t taskCounter = 0;

  btn.read();

#ifdef ENABLE_SERVER
  ElegantOTA.loop();
#endif

#if !defined(ESP32) && !defined(ESP8266)
  pluginManager.runActivePlugin();
#endif

  if (currentStatus == NONE || currentStatus == SCROLLING)
  {
    if (currentStatus == NONE) Scheduler.update();

    if ((taskCounter & 0x03) == 0)
    {
      if (Messages.hasMessages())
      {
        currentStatus = SCROLLING;
        Messages.scroll();
        if (!Messages.hasMessages() && Messages.wasScrolling())
        {
          currentStatus = NONE;
          Messages.clearScrollingFlag();
          if (pluginManager.getActivePlugin()) {
            pluginManager.getActivePlugin()->setup();
          }
        }
      }
      else if (Messages.wasScrolling())
      {
        currentStatus = NONE;
        Messages.clearScrollingFlag();
        if (pluginManager.getActivePlugin()) {
          pluginManager.getActivePlugin()->setup();
        }
      }
    }
  }

#ifdef ENABLE_SERVER
  ConnectionManager::checkWiFiConnection();
#endif

  taskCounter++;
  if (taskCounter > 16)
  {
    taskCounter = 0;
  }

#ifdef ENABLE_SERVER
  cleanUpClients();
  if (config.getAlexaEnabled()) {
    AlexaManager::loop();
  }
  ElegantOTA.loop();
  
  static unsigned long lastLogTime = 0;
  if (Logger::liveLoggingEnabled && millis() - lastLogTime > 5000) {
    lastLogTime = millis();
    Logger::printf("[System] Running normally. Uptime: %lu seconds. Free Heap: %d bytes\n", millis() / 1000, ESP.getFreeHeap());
  }
#endif

#ifdef ESP32
  checkForRemotePluginSwitch();
  vTaskDelay(1);
#else
  delay(1);
#endif
}
