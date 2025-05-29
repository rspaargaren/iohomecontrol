#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>

// Forward declaration if ESPAsyncWebServer is used
class ESPAsyncWebServer; 

void setupWebServer();
void loopWebServer(); // If any loop processing is needed for the web server

#endif // WEB_SERVER_HANDLER_H
