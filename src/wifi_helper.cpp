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

#include <wifi_helper.h>
#include <oled_display.h>
#include <user_config.h>
#if defined(MQTT)
#include <mqtt_handler.h>
#endif
#include <WiFiManager.h>

TimerHandle_t wifiReconnectTimer;

ConnState wifiStatus = ConnState::Disconnected;

void initWifi() {
    wifiReconnectTimer = xTimerCreate(
        "wifiTimer",
        pdMS_TO_TICKS(10000),  // 10 seconds retry interval
        pdFALSE,
        nullptr,
        connectToWifi
    );
    if (!wifiReconnectTimer) {
        Serial.println("Failed to create WiFi reconnect timer");
    }
    connectToWifi(nullptr);
}

void connectToWifi(TimerHandle_t /*timer*/) {
    Serial.println("Connecting to Wi-Fi via WiFiManager...");
    wifiStatus = ConnState::Connecting;
    updateDisplayStatus();

    WiFi.mode(WIFI_STA);
    WiFiManager wm;
    wm.setConnectTimeout(30);        // 10 sec voor verbinding met AP
    wm.setConfigPortalTimeout(180);  // 3 min captive portal open

    bool res = wm.autoConnect("iohc-setup");
    if (!res) {
        Serial.println("WiFiManager failed to connect");
        wifiStatus = ConnState::Disconnected;
        updateDisplayStatus();

        // Retry later
        if (wifiReconnectTimer) {
            xTimerStart(wifiReconnectTimer, 0);
        }
    } else {
        Serial.printf("Connected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());
        wifiStatus = ConnState::Connected;
        updateDisplayStatus();
#if defined(MQTT)
        // Establish MQTT connection if needed
        if (!mqttClient.connected() && mqttStatus != ConnState::Connecting) {
            connectToMqtt();
        }
#endif
    }
}

void checkWifiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiStatus == ConnState::Connected) {
            Serial.println("WiFi connection lost");
            wifiStatus = ConnState::Disconnected;
            updateDisplayStatus();

#if defined(MQTT)
            Serial.println("Stopping MQTT reconnect timer");
            if (mqttReconnectTimer) {
                xTimerStop(mqttReconnectTimer, 0);
            }
#endif
            if (wifiReconnectTimer) {
                xTimerStart(wifiReconnectTimer, 0);
            }
        }
    }
}
