/*
   Copyright (c) 2024. CRIDP https://github.com/cridp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef INTERACT_H
#define INTERACT_H
/*
  MQTT & Command Line interaction
*/
#include <board-config.h>
#include <user_config.h>

#include <vector>
#include <sstream>
#include <cstring>
#include <algorithm>

extern "C" {
        #include "freertos/FreeRTOS.h"
        #include "freertos/timers.h"
}

#if defined(SSD1306_DISPLAY)
#include <Adafruit_SSD1306.h>
#endif
#include <web_server_handler.h>
//#if defined(MQTT)
//#  include <AsyncMqttClient.h>
//#  include <ArduinoJson.h>
//#endif

#if defined(MQTT)
#include "mqtt_handler.h"
#endif

#include <utils.h>
#include <tokens.h>

namespace IOHC {
  class iohcRemote1W;
}

#if defined(ESP32)
  #include <TickerUsESP32.h>
  #define MAXCMDS 50
#endif

#if defined(SSD1306_DISPLAY)
extern Adafruit_SSD1306 display;
#endif

enum class ConnState { Connecting, Connected, Disconnected };
extern ConnState mqttStatus;
void tokenize(std::string const &str, const char delim, Tokens &out);

struct _cmdEntry {
    char cmd[15];
    char description[61];
    void (*handler)(Tokens*);
};
extern _cmdEntry* _cmdHandler[MAXCMDS];
extern uint8_t lastEntry;


#if defined(DEBUG)
  #ifndef DEBUG_PORT
    #define DEBUG_PORT Serial
  #endif
  #define LOG(func, ...) DEBUG_PORT.func(__VA_ARGS__)
#else
  #define LOG(func, ...)
#endif

namespace Cmd {

extern bool verbosity;
extern bool pairMode;
extern bool scanMode;

#if defined(ESP32)
  extern TimersUS::TickerUsESP32 kbd_tick;
#endif

extern TimerHandle_t consoleTimer;


bool addHandler(char *cmd, char *description, void (*handler)(Tokens*));
char *cmdReceived(bool echo = false);
void cmdFuncHandler();
void createCommands();
void init();

}

#endif
