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

#include <WiFi.h>
#include <esp_wifi.h>
#include <Adafruit_SSD1306.h>
//#if defined(MQTT)
//  #include <AsyncMqttClient.h>
//  #include <ArduinoJson.h>
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

inline TimerHandle_t wifiReconnectTimer;
inline WiFiClient wifiClient;                 // Create an ESP32 WiFiClient class to connect to the MQTT server
extern Adafruit_SSD1306 display;

enum class ConnState { Connecting, Connected, Disconnected };
inline ConnState wifiStatus = ConnState::Connecting;
inline ConnState mqttStatus = ConnState::Disconnected;

inline void updateDisplayStatus() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WiFi: ");
  switch (wifiStatus) {
    case ConnState::Connected:
      display.println("connected");
      break;
    case ConnState::Connecting:
      display.println("connecting");
      break;
    default:
      display.println("disconnected");
      break;
  }
  display.setCursor(0, 10);
  display.print("IP: ");
  if (wifiStatus == ConnState::Connected) {
    display.println(WiFi.localIP());
  } else {
    display.println("-");
  }
  display.setCursor(0, 20);
  display.print("MQTT: ");
  switch (mqttStatus) {
    case ConnState::Connected:
      display.println("connected");
      break;
    case ConnState::Connecting:
      display.println("connecting");
      break;
    default:
      display.println("disconnected");
      break;
  }
  display.display();
}

  inline void tokenize(std::string const &str, const char delim, Tokens &out) {
      // construct a stream from the string 
      std::stringstream ss(str); 

      std::string s; 
      while (std::getline(ss, s, delim)) { 
          out.push_back(s); 
      } 
  }
  inline struct _cmdEntry {
      char cmd[15];
      char description[61];
      void (*handler)(Tokens*);
  } *_cmdHandler[MAXCMDS];

  inline uint8_t lastEntry = 0;



  inline void connectToWifi() {
    Serial.println("Connecting to Wi-Fi...");
    wifiStatus = ConnState::Connecting;
    updateDisplayStatus();
    WiFiClass::mode(WIFI_STA);
    // IPAddress ip(192, 168, 1, 68);
    // IPAddress dns(192, 168, 1, 1);
    // IPAddress gateway(192, 168, 1, 1);
    // IPAddress subnet(255, 255, 255, 0);
    // WiFi.config(ip, gateway, subnet, dns);

    // Dont use wifi N some errors happen
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    uint8_t primaryChan = 10;
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(primaryChan, secondChan);
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
     
    // TODO put the wifi task in core 1 as tickerUS cant be changed of core
    // constexpr wifi_init_config_t tweak = {.wifi_task_core_id = 0, }; // For fun
    // ESP_ERROR_CHECK(esp_wifi_init(&tweak));

    // esp_wifi_scan_stop do the job avoiding crash with MQTT
    ESP_ERROR_CHECK(esp_wifi_scan_stop()); 
    // WiFi.printDiag(Serial);
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  }

inline void WiFiEvent(WiFiEvent_t event) {
//    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        //byte mac[6];
        Serial.printf(WiFi.macAddress().c_str());
        Serial.printf(" IP address: %s ", WiFi.localIP().toString().c_str());
        wifiStatus = ConnState::Connected;
        updateDisplayStatus();
        xTimerStop(wifiReconnectTimer, 0);
        #if defined(MQTT)
          connectToMqtt();
        #endif
        printf("\n");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        wifiStatus = ConnState::Disconnected;
        updateDisplayStatus();
        #if defined(MQTT)
          xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        #endif
        xTimerStart(wifiReconnectTimer, 0);
        break;
    default: {}
    }
}

#if defined(DEBUG)
  #ifndef DEBUG_PORT
    #define DEBUG_PORT Serial
  #endif
  #define LOG(func, ...) DEBUG_PORT.func(__VA_ARGS__)
#else
  #define LOG(func, ...) ;
#endif

// using Tokens = std::vector<std::string>;
namespace Cmd {

  inline char _rxbuffer[512];
  inline uint8_t _len = 0;
  inline uint8_t _avail = 0;


    void createCommands();

    inline bool verbosity = true;
    inline bool pairMode = false;
    inline bool scanMode = false;


#if defined(ESP32)
      inline TimersUS::TickerUsESP32 kbd_tick;
#endif

    inline TimerHandle_t consoleTimer;

  inline bool addHandler(char *cmd, char *description, void (*handler)(Tokens*)) {
    for (uint8_t idx=0; idx<MAXCMDS; ++idx) {
      if (_cmdHandler[idx] != nullptr) {} // Skip already allocated cmd handler
      else {
        void* alloc = malloc(sizeof(struct _cmdEntry));
        if (!alloc)
          return false;

        _cmdHandler[idx] = static_cast<_cmdEntry *>(alloc);
        memset(alloc, 0, sizeof(struct _cmdEntry));
        strncpy(_cmdHandler[idx]->cmd, cmd, strlen(cmd)<sizeof(_cmdHandler[idx]->cmd)?strlen(cmd):sizeof(_cmdHandler[idx]->cmd) - 1);
        strncpy(_cmdHandler[idx]->description, description, strlen(cmd)<sizeof(_cmdHandler[idx]->description)?strlen(description):sizeof(_cmdHandler[idx]->description) - 1);
        _cmdHandler[idx]->handler = handler;

        if (idx > lastEntry)
          lastEntry = idx;
        return true;
      } 
    }
    return false;
  }

  inline char *cmdReceived(bool echo = false) {
      _avail = Serial.available();
      if (_avail)  {
        _len += Serial.readBytes(&_rxbuffer[_len], _avail);
  //      receivingSerial = true;
        if (echo) {
          _rxbuffer[_len] = '\0';
          Serial.printf("%s", &_rxbuffer[_len - _avail]);
        }
      }
      if (_rxbuffer[_len-1] == 0x0a) {
          _rxbuffer[_len-2] = '\0';
          _len = 0;
  //        receivingSerial = false;
          return _rxbuffer;
      }
      return nullptr;
  }

  inline void cmdFuncHandler() {
    constexpr char delim = ' ';
    Tokens segments; 

    char* _cmd = cmdReceived(true);
    if (!_cmd) return;
    if (!strlen(_cmd)) return;

    tokenize(_cmd, delim, segments);
    if (strcmp((char *)"help", segments[0].c_str()) == 0) {
      Serial.printf("\nRegistered commands:\n");
      for (uint8_t idx=0; idx<=lastEntry; ++idx) {
        if (_cmdHandler[idx] == nullptr) continue;
        Serial.printf("- %s\t%s\n", _cmdHandler[idx]->cmd, _cmdHandler[idx]->description);
      }
      Serial.printf("- %s\t%s\n\n", (char*)"help", (char*)"This command");
      Serial.printf("\n");
      return;
    }
    for (uint8_t idx=0; idx<=lastEntry; ++idx) {
      if (_cmdHandler[idx] == nullptr) continue;
      if (strcmp(_cmdHandler[idx]->cmd, segments[0].c_str()) == 0) {
        _cmdHandler[idx]->handler(&segments);
        return;
      }
    }
    Serial.printf("*> Unknown <*\n");
  }
  
  inline void init() {
    
    #if defined(MQTT)
      initMqtt();
    #endif

    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, nullptr, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    WiFi.onEvent(WiFiEvent);
    connectToWifi();

//    consoleTimer = xTimerCreate("consoleTimer", pdMS_TO_TICKS(500), pdFALSE, nullptr, reinterpret_cast<TimerCallbackFunction_t>(cmdFuncHandler));
    kbd_tick.attach_ms(500, cmdFuncHandler);

  }  
}
#endif