#include "ota.h"

#ifdef ENABLE_SERVER

const char *otaUser = OTA_USERNAME;
const char *otaPassword = OTA_PASSWORD;

void onOTAStart()
{
  currentStatus = UPDATE;

  std::vector<int> bits = Screen.readBytes(letterU);

  for (int i = 0; i < bits.size(); i++)
  {
    Screen.setPixelAtIndex(i, bits[i]);
  }
}

void onOTAEnd(bool success)
{
  std::vector<int> bits = Screen.readBytes(letterR);

  for (int i = 0; i < bits.size(); i++)
  {
    Screen.setPixelAtIndex(i, bits[i]);
  }

  delay(1000);
  currentStatus = NONE;
  Screen.loadFromStorage();
}

void initOTA(AsyncWebServer &server)
{
  ElegantOTA.begin(&server, otaUser, otaPassword);
  ElegantOTA.onStart(onOTAStart);
}

#endif
