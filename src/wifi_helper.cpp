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
#include <esp_wifi.h>
#if defined(MQTT)
#include <mqtt_handler.h>
#endif
#include <WiFiManager.h>
#include <ESPmDNS.h>

TimerHandle_t wifiReconnectTimer;
WiFiStatus wifiStatus = { ConnState::Disconnected, 0 };

static TaskHandle_t s_wifiTaskHandle = nullptr;
static bool s_portalNeeded = false; // true only on first boot if no credentials exist
static TimerHandle_t s_rssiTimer = nullptr;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Replicate WiFiManager::getRSSIasQuality() without constructing a WiFiManager object.
static int rssiToQuality(int rssi) {
    if (rssi <= -100) return 0;
    if (rssi >= -50)  return 100;
    return 2 * (rssi + 100);
}

static void rssiTimerCb(TimerHandle_t) {
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus.signalStrengthPercent = rssiToQuality(WiFi.RSSI());
    }
}

static void onMqttAfterWifi() {
#if defined(MQTT)
    if (mqttReconnectTimer && !mqttClient.connected() &&
        mqttStatus != ConnState::Connecting) {
        connectToMqtt();
    }
#endif
}

static void wifiConnectTask(void * /*arg*/) {
    for (;;) {
        // Wait for notification from timer callback or WiFi event handler
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // The auto-reconnect may have already restored the connection by the time
        // this task runs (race between the reconnect timer and the GOT_IP event).
        // If we're already connected, just ensure MQTT is up and do nothing else.
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi: connect task woken but already connected, skipping");
            wifiStatus = { ConnState::Connected, rssiToQuality(WiFi.RSSI()) };
            updateDisplayStatus();
            onMqttAfterWifi();
            continue;
        }

        Serial.println("WiFi: connect task woken, attempting connection...");
        wifiStatus = { ConnState::Connecting, 0 };
        updateDisplayStatus();

        // ensure to have all channels available so it connects to the AP with strongest signal if you have multiple with the same name
        WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN); 

        // safety measures
        wifi_config_t conf;
        if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
#ifdef REQUIRE_MINIMUM_WPA2_PSK  
            // This is necessary to prevent the device from Evil Twin attacks, where an attacker creates an additional network with the same
            // SSID as the one selected. WPA2_PSK will detect that and even prevent sending the password. 

            // Enable minimal WPA2_PSK level (also allows WPA3 or other more secure modes)
            conf.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
#else
            conf.sta.threshold.authmode = WIFI_AUTH_OPEN;
#endif
            esp_wifi_set_config(WIFI_IF_STA, &conf);
        }

        // Try the stored credentials quickly first
        WiFi.mode(WIFI_STA);
        WiFi.begin();

        wl_status_t result = static_cast<wl_status_t>(WiFi.waitForConnectResult(10000UL));

        if (result == WL_CONNECTED) {
            Serial.printf("WiFi: connected, IP=%s RSSI=%d\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());
            wifiStatus = { ConnState::Connected, rssiToQuality(WiFi.RSSI()) };
            updateDisplayStatus();

            if (!MDNS.begin("miopenio")) {
                Serial.println("WiFi: mDNS start failed");
            } else {
                Serial.println("WiFi: mDNS started at http://miopenio.local");
            }

            onMqttAfterWifi();

        } else if (s_portalNeeded) {
            // First boot with no valid credentials — open the captive portal once
            Serial.println("WiFi: no stored credentials, launching WiFiManager portal");
            WiFiManager wm;
            wm.setConnectTimeout(30);
            wm.setConfigPortalTimeout(180);
            bool ok = wm.autoConnect("iohc-setup");
            s_portalNeeded = false; // never open again after this attempt

            if (ok) {
                Serial.printf("WiFi: portal success, IP=%s\n",
                              WiFi.localIP().toString().c_str());
                wifiStatus = { ConnState::Connected, rssiToQuality(WiFi.RSSI()) };
                updateDisplayStatus();

                if (!MDNS.begin("miopenio")) {
                    Serial.println("WiFi: mDNS start failed");
                } else {
                    Serial.println("WiFi: mDNS started at http://miopenio.local");
                }

                onMqttAfterWifi();
            } else {
                Serial.println("WiFi: portal timed out, will retry");
                wifiStatus = { ConnState::Disconnected, 0 };
                updateDisplayStatus();
                if (wifiReconnectTimer) xTimerStart(wifiReconnectTimer, 0);
            }

        } else {
            // Reconnect attempt after a dropout — just retry later, no portal
            Serial.printf("WiFi: reconnect failed (status=%d), will retry in 10s\n",
                          static_cast<int>(result));
            wifiStatus = { ConnState::Disconnected, 0 };
            updateDisplayStatus();
            if (wifiReconnectTimer) xTimerStart(wifiReconnectTimer, 0);
        }
    }
}

// Timer callback — must not block; just wake the task
static void wifiReconnectTimerCb(TimerHandle_t) {
    if (s_wifiTaskHandle) xTaskNotifyGive(s_wifiTaskHandle);
}

// ---------------------------------------------------------------------------
// WiFi event handler — instant detection, no polling required
// ---------------------------------------------------------------------------

static void onWifiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (wifiStatus.connectionStatus == ConnState::Connected) {
                Serial.println("WiFi: connection lost (event)");
                wifiStatus = { ConnState::Disconnected, 0 };
                updateDisplayStatus();
                // Kick reconnect task after 10 s
                if (wifiReconnectTimer) xTimerStart(wifiReconnectTimer, 0);
            }
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            // The ESP32 driver reconnected autonomously (setAutoReconnect default)
            if (wifiStatus.connectionStatus != ConnState::Connected) {
                Serial.printf("WiFi: auto-recovered, IP=%s\n",
                              WiFi.localIP().toString().c_str());
                wifiStatus = { ConnState::Connected, rssiToQuality(WiFi.RSSI()) };
                updateDisplayStatus();
                // Stop any pending reconnect timer — we're already back
                if (wifiReconnectTimer) xTimerStop(wifiReconnectTimer, 0);
                onMqttAfterWifi();
            }
            break;

        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void initWifi() {
    // Set hostname before the WiFi stack initialises so the DHCP client
    // advertises the correct name from the very first connection attempt,
    // including after auto-reconnects where the connect task never runs.
    WiFi.setHostname("MiOpenIO");

    // Detect whether we have stored credentials; if not, portal is needed on first attempt
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    s_portalNeeded = (conf.sta.ssid[0] == '\0');

    WiFi.onEvent(onWifiEvent);

    wifiReconnectTimer = xTimerCreate(
        "wifiTimer",
        pdMS_TO_TICKS(10000),
        pdFALSE,          // one-shot; restarted explicitly when needed
        nullptr,
        wifiReconnectTimerCb
    );
    if (!wifiReconnectTimer) {
        Serial.println("WiFi: failed to create reconnect timer");
    }

    // Create the dedicated WiFi task so blocking calls never run in the timer task
    xTaskCreatePinnedToCore(
        wifiConnectTask,
        "wifiConnect",
        4096,
        nullptr,
        2,               // priority just above loop() (1), below radio ISR task (4)
        &s_wifiTaskHandle,
        tskNO_AFFINITY
    );

    // Periodically refresh the RSSI reading for the signal-strength display
    s_rssiTimer = xTimerCreate("rssiTimer", pdMS_TO_TICKS(5000), pdTRUE,
                               nullptr, rssiTimerCb);
    if (s_rssiTimer) xTimerStart(s_rssiTimer, 0);

    // Kick the initial connection attempt
    if (s_wifiTaskHandle) xTaskNotifyGive(s_wifiTaskHandle);
}

void checkWifiConnection() {
    // Still polled from loop() as a safety net for cases the event handler misses.
    // Detect both loss AND autonomous recovery.
    wl_status_t status = static_cast<wl_status_t>(WiFi.status());

    if (status != WL_CONNECTED &&
        wifiStatus.connectionStatus == ConnState::Connected) {
        // Missed disconnect event
        Serial.println("WiFi: loss detected by poll");
        wifiStatus = { ConnState::Disconnected, 0 };
        updateDisplayStatus();
        if (wifiReconnectTimer) xTimerStart(wifiReconnectTimer, 0);
    } else if (status == WL_CONNECTED &&
               wifiStatus.connectionStatus != ConnState::Connected) {
        // Autonomous recovery detected by poll (event handler already handles this,
        // but this is a belt-and-suspenders fallback)
        Serial.println("WiFi: recovery detected by poll");
        wifiStatus = { ConnState::Connected, rssiToQuality(WiFi.RSSI()) };
        updateDisplayStatus();
        if (wifiReconnectTimer) xTimerStop(wifiReconnectTimer, 0);
        onMqttAfterWifi();
    }
}
