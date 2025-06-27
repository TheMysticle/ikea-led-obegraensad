#pragma once

#include "PluginManager.h"
#include <vector>
#include <Arduino.h>

struct ScheduleItem
{
  int pluginId;
  int startTime; // Start time in minutes from midnight (0-1439)
  int endTime;   // End time in minutes from midnight (0-1439)
  int brightness; // Brightness level (0-255), -1 to ignore
};

class PluginScheduler
{
private:
  PluginScheduler() = default;
  int currentScheduleIndex = -1; // -1 means no schedule item is active

public:
  static PluginScheduler &getInstance();

  PluginScheduler(const PluginScheduler &) = delete;
  PluginScheduler &operator=(const PluginScheduler &) =delete;

  bool isActive = false;
  bool isBrightnessOverridden = false;
  std::vector<ScheduleItem> schedule;

  void addItem(int pluginId, int startTime, int endTime, int brightness);
  void clearSchedule(bool emptyStorage = false);
  void start();
  void stop();
  void update();
  void init();
  bool setScheduleByJSONString(String scheduleJson);
  int getActiveScheduleIndex() const;

private:
  void applyScheduleItem(int index);
};

extern PluginScheduler &Scheduler;