#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>
#include <user_config.h>

// Forward declaration if ESPAsyncWebServer is used
class ESPAsyncWebServer;

#if defined(WEBSERVER)
void setupWebServer();
void loopWebServer(); // If any loop processing is needed for the web server
#else
inline void setupWebServer() {}
inline void loopWebServer() {}
#endif

#endif // WEB_SERVER_HANDLER_H
