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

TimerHandle_t wifiReconnectTimer;
WiFiClient wifiClient;
ConnState wifiStatus = ConnState::Connecting;

void initWifi() {
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE,
                                    nullptr,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.onEvent(WiFiEvent);
  connectToWifi();
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi via WiFiManager...");
  wifiStatus = ConnState::Connecting;
  updateDisplayStatus();
  WiFiClass::mode(WIFI_STA);

  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  uint8_t primaryChan = 10;
  wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
  esp_wifi_set_channel(primaryChan, secondChan);
  ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
  ESP_ERROR_CHECK(esp_wifi_scan_stop());

  WiFiManager wm;
  bool res = wm.autoConnect("iohc-setup");
  if (!res) {
    Serial.println("WiFiManager failed to connect");
    wifiStatus = ConnState::Disconnected;
  } else {
    Serial.printf("Connected to WiFi. IP address: %s\n", WiFi.localIP().toString().c_str());
    wifiStatus = ConnState::Connected;
  }
  updateDisplayStatus();
}

void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.printf(WiFi.macAddress().c_str());
        Serial.printf(" IP address: %s ", WiFi.localIP().toString().c_str());
        wifiStatus = ConnState::Connected;
        updateDisplayStatus();
        setupWebServer();
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
        xTimerStop(mqttReconnectTimer, 0);
#endif
        xTimerStart(wifiReconnectTimer, 0);
        break;
    default:
        break;
    }
}

