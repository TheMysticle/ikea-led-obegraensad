#include "asyncwebserver.h"
#include "messages.h"
#include "webhandler.h"

#define ESPALEXA_ASYNC
#include <Espalexa.h>

#ifdef ENABLE_SERVER

AsyncWebServer server(80);
extern Espalexa espalexa;

void initWebServer()
{
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.on("/", HTTP_GET, startGui);

  // Route to handle  http://your-server/message?text=Hello&repeat=3&id=42&delay=30&graph=1,2,3,4&miny=0&maxy=15
  server.on("/api/message", HTTP_GET, handleMessage);
  server.on("/api/removemessage", HTTP_GET, handleMessageRemove);

  server.on("/api/info", HTTP_GET, handleGetInfo);

  // Handle API request to set an active plugin by ID
  server.on("/api/plugin", HTTP_PATCH, handleSetPlugin);

  // Handle API request to set the brightness (0..255);
  server.on("/api/brightness", HTTP_PATCH, handleSetBrightness);
  server.on("/api/data", HTTP_GET, handleGetData);

  // --- START OF THE CRITICAL FIX for Scheduler ---
  // This route now uses a body handler to accept JSON data.
  server.on(
      "/api/schedule", HTTP_POST,
      [](AsyncWebServerRequest *request) {}, // We don't need the simple request handler
      NULL,                                 // No file upload
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // This lambda is called with the request body
        if (index == 0)
        {
          // Create a String from the received data
          String body;
          body.reserve(len);
          for (size_t i = 0; i < len; i++)
          {
            body += (char)data[i];
          }
          // Call our updated handler function
          handleSetSchedule(request, body);
        }
      });
  // --- END OF THE CRITICAL FIX for Scheduler ---

  server.on("/api/schedule/clear", HTTP_GET, handleClearSchedule);
  server.on("/api/schedule/stop", HTTP_GET, handleStopSchedule);
  server.on("/api/schedule/start", HTTP_GET, handleStartSchedule);

  server.on("/api/storage/clear", HTTP_GET, handleClearStorage);


  // Add a "Not Found" handler that first tries to process the request with Espalexa
  server.onNotFound([](AsyncWebServerRequest *request) {
    // First, check if Espalexa can handle the request
    if (espalexa.handleAlexaApiCall(request)) {
      // If it returns true, the request was for Alexa, and has been handled.
      return;
    }
    
    // If Espalexa did not handle it, then it's a true 404
    request->send(404, "text/plain", "Page not found!");
  });
}

#endif