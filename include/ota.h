#pragma once

#include "constants.h"

#ifdef ENABLE_SERVER

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include "screen.h"
#include "PluginManager.h"
#include "secrets.h"
#include "signs.h"

void onOTAStart();
void onOTAEnd(bool success);
void initOTA(AsyncWebServer &server);

#endif
