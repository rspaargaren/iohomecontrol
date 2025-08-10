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

#ifndef IOHC_USER_H
#define IOHC_USER_H

#include <board-config.h>
#include <string>

// WiFi credentials are handled via WiFiManager
#define WIFI_SSID ""
#define WIFI_PASSWD ""
inline const char *wifi_ssid = "";
inline const char *wifi_passwd = "";

#define MQTT

// Default MQTT configuration. These values can be changed at runtime through
// the interactive command interface.
inline std::string mqtt_server = "XX";
inline std::string mqtt_user = "mosquitto";
inline std::string mqtt_password = "XX";
inline std::string mqtt_discovery_topic = "homeassistant";

// Comment out the next line if no display is connected
#define SSD1306_DISPLAY

// Comment out the next line to disable the built-in web server
#define WEBSERVER

#define HTTPS_LISTEN_PORT   443
#define HTTPS_CERT_PATH     "/miopenio.local.crt"
#define HTTPS_KEY_PATH      "/miopenio.local.key"
#define MDNS_HOSTNAME       "Miopenio"
#define HTTP_USERNAME       "admin"
#define HTTP_PASSWORD       "admin"
#if defined(ESP8266)
        #define SERIALSPEED         460800
#elif defined(ESP32)
        #define SERIALSPEED         115200 //921600
#endif

#endif
