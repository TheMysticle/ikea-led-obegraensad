#include "PluginManager.h"
#include "scheduler.h"
#include "Logger.h"

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#ifdef ENABLE_SERVER

AsyncWebSocket ws("/ws");

void sendInfo()
{
  JsonDocument jsonDocument;
  if (currentStatus == NONE)
  {
    for (int j = 0; j < ROWS * COLS; j++)
    {
      jsonDocument["data"][j] = Screen.getRenderBuffer()[j];
    }
  }

  jsonDocument["status"] = currentStatus;
  jsonDocument["plugin"] = pluginManager.getActivePlugin()->getId();
  jsonDocument["persist-plugin"] = pluginManager.getPersistedPluginId();
  jsonDocument["event"] = "info";
  jsonDocument["rotation"] = Screen.currentRotation;
  jsonDocument["brightness"] = Screen.getCurrentBrightness();
  jsonDocument["power"] = Screen.isPowerOn();
  jsonDocument["scheduleActive"] = Scheduler.isActive;
  
  Plugin* activePlugin = pluginManager.getActivePlugin();
  jsonDocument["hasSpeedControl"] = activePlugin->hasSpeedControl();
  if (activePlugin->hasSpeedControl()) {
    jsonDocument["speed"] = activePlugin->getSpeed();
    jsonDocument["defaultSpeed"] = activePlugin->getDefaultSpeed();
  }

  JsonArray scheduleArray = jsonDocument["schedule"].to<JsonArray>();
  for (const auto &item : Scheduler.schedule)
  {
    JsonObject scheduleItem = scheduleArray.add<JsonObject>();
    scheduleItem["pluginId"] = item.pluginId;
    
    char startTimeStr[6], endTimeStr[6];
    sprintf(startTimeStr, "%02d:%02d", item.startTime / 60, item.startTime % 60);
    sprintf(endTimeStr, "%02d:%02d", item.endTime / 60, item.endTime % 60);
    scheduleItem["startTime"] = startTimeStr;
    scheduleItem["endTime"] = endTimeStr;

    scheduleItem["brightness"] = item.brightness;
  }

  JsonArray plugins = jsonDocument["plugins"].to<JsonArray>();

  std::vector<Plugin *> &allPlugins = pluginManager.getAllPlugins();
  for (Plugin *plugin : allPlugins)
  {
    JsonObject object = plugins.add<JsonObject>();

    object["id"] = plugin->getId();
    object["name"] = plugin->getName();
  }
  String output;
  serializeJson(jsonDocument, output);
  ws.textAll(output);
  jsonDocument.clear();
}

void sendMinimalInfo()
{
  JsonDocument jsonDocument;

  jsonDocument["status"] = currentStatus;
  jsonDocument["plugin"] = pluginManager.getActivePlugin()->getId();
  jsonDocument["event"] = "minimal-info";
  jsonDocument["rotation"] = Screen.currentRotation;
  jsonDocument["brightness"] = Screen.getCurrentBrightness();
  jsonDocument["power"] = Screen.isPowerOn();
  jsonDocument["scheduleActive"] = Scheduler.isActive;
  jsonDocument["activeScheduleIndex"] = Scheduler.getActiveScheduleIndex(); // ADD THIS LINE

  Plugin* activePlugin = pluginManager.getActivePlugin();
  jsonDocument["hasSpeedControl"] = activePlugin->hasSpeedControl();
  if (activePlugin->hasSpeedControl()) {
    jsonDocument["speed"] = activePlugin->getSpeed();
    jsonDocument["defaultSpeed"] = activePlugin->getDefaultSpeed();
  }

  String output;
  serializeJson(jsonDocument, output);
  ws.textAll(output);
  jsonDocument.clear();
}

void sendWSMessage(String &message) {
  ws.textAll(message);
}

void onWsEvent(AsyncWebSocket *server,
               AsyncWebSocketClient *client,
               AwsEventType type,
               void *arg,
               uint8_t *data,
               size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    sendInfo();
  }

  if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      if (info->opcode == WS_BINARY && currentStatus == WSBINARY && info->len == 256)
      {
        Screen.setRenderBuffer(data, true);
      }
      else if (info->opcode == WS_TEXT)
      {
        data[len] = 0;

        JsonDocument wsRequest;
        DeserializationError error = deserializeJson(wsRequest, data);

        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }
        else
        {
          pluginManager.getActivePlugin()->websocketHook(wsRequest);

          const char *event = wsRequest["event"];

          if (!strcmp(event, "plugin"))
          {
            int pluginId = wsRequest["plugin"];

            Scheduler.clearSchedule();
            pluginManager.setActivePluginById(pluginId);
            sendMinimalInfo();
          }
          else if (!strcmp(event, "persist-plugin"))
          {
            pluginManager.persistActivePlugin();
            sendInfo();
          }
          else if (!strcmp(event, "rotate"))
          {
            bool isRight = (bool)!strcmp(wsRequest["direction"], "right");
            Screen.setCurrentRotation((Screen.currentRotation + (isRight ? 1 : 3)) % 4, true);
            sendInfo();
          }
          else if (!strcmp(event, "info"))
          {
            sendInfo();
          }
          else if (!strcmp(event, "brightness"))
          {
            uint8_t brightness = wsRequest["brightness"].as<uint8_t>();
            Screen.setBrightness(brightness, true);
            if (Scheduler.isActive) {
                Scheduler.isBrightnessOverridden = true;
            }
            sendMinimalInfo();
          }
          else if (!strcmp(event, "power"))
          {
            bool powerOn = wsRequest["state"].as<bool>();
            Screen.setPower(powerOn);
            if (Scheduler.isActive) {
                Scheduler.isBrightnessOverridden = true;
            }
            sendMinimalInfo();
          }
          else if (!strcmp(event, "speed"))
          {
            if (pluginManager.getActivePlugin()->hasSpeedControl()) {
              int speed = wsRequest["speed"].as<int>();
              pluginManager.getActivePlugin()->setSpeed(speed);
              sendMinimalInfo();
            }
          }
          else if (!strcmp(event, "data"))
          {
              // Lightweight pixel-only response for card matrix preview
              JsonDocument dataDoc;
              JsonArray dataArray = dataDoc["data"].to<JsonArray>();
              for (int j = 0; j < ROWS * COLS; j++)
              {
                  dataArray.add(Screen.getRenderBuffer()[j]);
              }
              dataDoc["event"] = "data";
              String output;
              serializeJson(dataDoc, output);
              client->text(output);
              dataDoc.clear();
          }
          else if (!strcmp(event, "enable-logging"))
          {
             bool enabled = wsRequest["enabled"];
             Logger::liveLoggingEnabled = enabled;
          }
          else if (!strcmp(event, "diagnostics"))
          {
             JsonDocument doc;
             doc["event"] = "diagnostics";
             doc["heap"] = ESP.getFreeHeap();
             doc["uptime"] = millis() / 1000;
             doc["wifi_rssi"] = WiFi.RSSI();
             doc["loggingEnabled"] = Logger::liveLoggingEnabled;
             String output;
             serializeJson(doc, output);
             client->text(output);
          }
        }
      }
    }
  }
}

void initWebsocketServer(AsyncWebServer &server)
{
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);
}

void cleanUpClients()
{
  ws.cleanupClients();
}

#endif
