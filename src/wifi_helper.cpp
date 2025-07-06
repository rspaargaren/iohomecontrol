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
#include <mqtt_handler.h>
#include <WiFiManager.h>

TimerHandle_t wifiReconnectTimer;

ConnState wifiStatus = ConnState::Disconnected;

void initWifi() {
    wifiReconnectTimer = xTimerCreate(
        "wifiTimer",
        pdMS_TO_TICKS(10000),  // 10 seconds retry interval
        pdFALSE,
        nullptr,
        reinterpret_cast<TimerCallbackFunction_t>(connectToWifi)
    );
    connectToWifi();
}

void connectToWifi() {
    Serial.println("Connecting to Wi-Fi via WiFiManager...");
    wifiStatus = ConnState::Connecting;
    updateDisplayStatus();

    WiFi.mode(WIFI_STA);
    WiFiManager wm;

    bool res = wm.autoConnect("iohc-setup");
    if (!res) {
        Serial.println("WiFiManager failed to connect");
        wifiStatus = ConnState::Disconnected;
        updateDisplayStatus();

        // Retry later
        xTimerStart(wifiReconnectTimer, 0);
    } else {
        Serial.printf("Connected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());
        wifiStatus = ConnState::Connected;
        updateDisplayStatus();
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
            xTimerStop(mqttReconnectTimer, 0);
#endif
            xTimerStart(wifiReconnectTimer, 0);
        }
    }
}