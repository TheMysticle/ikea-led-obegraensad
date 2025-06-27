#include "scheduler.h"
#include "websocket.h"
#include <time.h>

PluginScheduler &PluginScheduler::getInstance()
{
  static PluginScheduler instance;
  return instance;
}

void PluginScheduler::addItem(int pluginId, int startTime, int endTime, int brightness)
{
  ScheduleItem item = {
      .pluginId = pluginId,
      .startTime = startTime,
      .endTime = endTime,
      .brightness = brightness};
  schedule.push_back(item);
}

void PluginScheduler::clearSchedule(bool emptyStorage)
{
  schedule.clear();
  currentScheduleIndex = -1;
  isActive = false;
#ifdef ENABLE_STORAGE
  if (emptyStorage)
  {
    storage.begin("led-wall", false);
    storage.putString("schedule", "[]"); // Store as empty JSON array
    storage.putBool("scheduleactive", false);
    storage.end();
  }
#endif
}

void PluginScheduler::start()
{
  if (!schedule.empty())
  {
    isActive = true;
    isBrightnessOverridden = false; // Reset override when starting
    currentScheduleIndex = -1;     // Force re-evaluation on next update
#ifdef ENABLE_STORAGE
    storage.begin("led-wall", false);
    storage.putBool("scheduleactive", true);
    storage.end();
#endif
    update(); // Run an initial update to set the state immediately
  }
}

void PluginScheduler::stop()
{
  isActive = false;
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", false);
  storage.putBool("scheduleactive", false);
  storage.end();
#endif
  // Optional: Revert to the default persisted plugin when scheduler stops
  pluginManager.activatePersistedPlugin();
  sendMinimalInfo();
}

void PluginScheduler::update()
{
  if (!isActive || schedule.empty())
  {
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return; // Cannot get time
  }

  int nowMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int newActiveIndex = -1;

  // Find which schedule item should be active
  for (int i = 0; i < schedule.size(); ++i)
  {
    const auto &item = schedule[i];
    bool isCurrentlyActive = false;
    if (item.startTime <= item.endTime)
    {
      // Normal case: 08:00 - 17:00
      isCurrentlyActive = (nowMinutes >= item.startTime && nowMinutes < item.endTime);
    }
    else
    {
      // Spans midnight: 22:00 - 02:00
      isCurrentlyActive = (nowMinutes >= item.startTime || nowMinutes < item.endTime);
    }

    if (isCurrentlyActive)
    {
      newActiveIndex = i;
      break; // First match wins
    }
  }

  // Check if the active schedule has changed
  if (newActiveIndex != currentScheduleIndex)
  {
    currentScheduleIndex = newActiveIndex;
    applyScheduleItem(currentScheduleIndex);
  }
}

void PluginScheduler::applyScheduleItem(int index)
{
  isBrightnessOverridden = false; // A new schedule is taking over, reset override

  if (index == -1)
  {
    // No schedule is active, revert to default plugin
    pluginManager.activatePersistedPlugin();
  }
  else
  {
    const auto &item = schedule[index];
    pluginManager.setActivePluginById(item.pluginId);
    if (item.brightness != -1)
    {
      Screen.setBrightness(item.brightness, false); // Don't persist schedule brightness
    }
  }
  sendMinimalInfo(); // Notify clients of the change
}

void PluginScheduler::init()
{
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", true);
  isActive = storage.getBool("scheduleactive", false);
  String scheduleStr = storage.getString("schedule", "[]");
  storage.end();

  setScheduleByJSONString(scheduleStr);

  if (isActive)
  {
    update(); // Immediately check and apply the current schedule
  }
#endif
}

bool PluginScheduler::setScheduleByJSONString(String scheduleJson)
{
  if (scheduleJson.length() == 0)
  {
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, scheduleJson);

  if (error)
  {
    Serial.println("Failed to parse schedule JSON");
    return false;
  }

#ifdef ENABLE_STORAGE
  storage.begin("led-wall", false);
  storage.putString("schedule", scheduleJson);
  storage.end();
#endif

  clearSchedule(false); // Clear current schedule without touching storage

  JsonArray schedule = doc.as<JsonArray>();
  for (JsonObject item : schedule)
  {
    if (!item.containsKey("pluginId") || !item.containsKey("startTime") || !item.containsKey("endTime"))
    {
      return false;
    }

    int pluginId = item["pluginId"].as<int>();
    const char *startTimeStr = item["startTime"]; // "HH:MM"
    const char *endTimeStr = item["endTime"];   // "HH:MM"
    int brightness = item["brightness"] | -1; // Default to -1 if not present

    int startHour, startMin, endHour, endMin;
    sscanf(startTimeStr, "%d:%d", &startHour, &startMin);
    sscanf(endTimeStr, "%d:%d", &endHour, &endMin);

    int startTimeInMinutes = startHour * 60 + startMin;
    int endTimeInMinutes = endHour * 60 + endMin;

    addItem(pluginId, startTimeInMinutes, endTimeInMinutes, brightness);
  }

  return true;
}

int PluginScheduler::getActiveScheduleIndex() const
{
  return currentScheduleIndex;
}

PluginScheduler &Scheduler = PluginScheduler::getInstance();