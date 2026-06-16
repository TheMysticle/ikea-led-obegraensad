#include "Logger.h"
#include <ArduinoJson.h>

#ifdef ENABLE_SERVER
#include "websocket.h"
#endif

bool Logger::liveLoggingEnabled = false;

void Logger::println(const String& message) {
    Serial.println(message);
    
#ifdef ENABLE_SERVER
    if (liveLoggingEnabled) {
        JsonDocument doc;
        doc["event"] = "log";
        doc["message"] = message + "\n";
        String output;
        serializeJson(doc, output);
        sendWSMessage(output);
    }
#endif
}

void Logger::print(const String& message) {
    Serial.print(message);

#ifdef ENABLE_SERVER
    if (liveLoggingEnabled) {
        JsonDocument doc;
        doc["event"] = "log";
        doc["message"] = message;
        String output;
        serializeJson(doc, output);
        sendWSMessage(output);
    }
#endif
}

void Logger::printf(const char *format, ...) {
    char loc_buf[256];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return;
    }
    if(len >= (int)sizeof(loc_buf)){
        temp = (char*)malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            return;
        }
        vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);
    
    Serial.print(temp);

#ifdef ENABLE_SERVER
    if (liveLoggingEnabled) {
        JsonDocument doc;
        doc["event"] = "log";
        doc["message"] = String(temp);
        String output;
        serializeJson(doc, output);
        sendWSMessage(output);
    }
#endif

    if(temp != loc_buf){
        free(temp);
    }
}
