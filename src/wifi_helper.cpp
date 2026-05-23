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
#include <ESPmDNS.h>
#include <TickerUsESP32.h>
#include <tuple>

const long PORTAL_TIMEOUT = 300000; // 5 minuten = 300.000 ms
const uint32_t WIFI_NOTIFY_GOT_IP = BIT0;
const uint32_t WIFI_NOTIFY_DISCONNECTED = BIT1;
const uint32_t WIFI_NOTIFY_RECONNECT = BIT2;

// below variables are thread safe because of the use of a single task that reads/modifies them (except for wifiStatus, but that one has atomic fields)
TimersUS::TickerUsESP32 wifiReconnectTimer {};
TimersUS::TickerUsESP32 rssiTimer {};
WiFiStatus wifiStatus = { ConnState::Disconnected, 0 };

TaskHandle_t wifiWorkerTaskHandle = NULL;
bool mdnsStarted = false;
bool webServerStarted = false;

// Replicate WiFiManager::getRSSIasQuality() without constructing a WiFiManager object.
static int rssiToQuality(int rssi) {
    if (rssi <= -100) return 0;
    if (rssi >= -50)  return 100;
    return 2 * (rssi + 100);
}

static void notifyWiFiWorker(uint32_t bits) {
    if (wifiWorkerTaskHandle != NULL) {
        xTaskNotify(wifiWorkerTaskHandle, bits, eSetBits);
    }
}

static void rssiTimerCb() {
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus.signalStrengthPercent = rssiToQuality(WiFi.RSSI());
    }
}

static void wifiReconnectTimerCb() {
    notifyWiFiWorker(WIFI_NOTIFY_RECONNECT);
}

static void ensureWebServerStarted() {
    if (!webServerStarted) {
        setupWebServer();
        webServerStarted = true;
    }
}

static void onMqttAfterWifi() {
#if defined(MQTT)
        // Establish MQTT connection if needed and MQTT client is initialized
        if (mqttReconnectTimer && !mqttClient.connected() &&
            mqttStatus != ConnState::Connecting) {
            connectToMqtt();
        }
#endif
}

static void handleWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus.connectionStatus = ConnState::Connected;
        wifiStatus.signalStrengthPercent = rssiToQuality(WiFi.RSSI());

        wifiReconnectTimer.detach();
        rssiTimer.attach(5, rssiTimerCb);
        updateDisplayStatus();

        if (!mdnsStarted) {
            if (!MDNS.begin("miopenio")) {
                Serial.println("WiFi: mDNS start failed");
            } else {
                mdnsStarted = true;
                Serial.println("WiFi: mDNS started at http://miopenio.local");
            }
        }

        ensureWebServerStarted();
        onMqttAfterWifi();
    }
}

static void configureWifiDisconnected() {
    Serial.println("WiFi: connection lost (event)");
    wifiStatus.connectionStatus = ConnState::Disconnected;
    wifiStatus.signalStrengthPercent = 0;
    rssiTimer.detach();
    wifiReconnectTimer.attach(10, wifiReconnectTimerCb);
    mdnsStarted = false;
    updateDisplayStatus();
}

static void handleWifiDisconnected() {
    if (wifiStatus.connectionStatus == ConnState::Connected) {
        configureWifiDisconnected();
    }
}

static void applyAdvancedWiFiSettings() {
    wifi_config_t config;
    if (esp_wifi_get_config(WIFI_IF_STA, &config) == ESP_OK) {
        config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;   
#ifdef REQUIRE_MINIMUM_WPA2_PSK  
        // This is necessary to prevent the device from Evil Twin attacks, where an attacker creates an additional network with the same
        // SSID as the one selected. WPA2_PSK will detect that and even prevent sending the password. 

        // Enable minimal WPA2_PSK level (also allows WPA3 or other more secure modes)
        config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK; 
#endif // REQUIRE_MINIMUM_WPA2_PSK
        esp_wifi_set_config(WIFI_IF_STA, &config);
    }
}

static std::string getConfiguredSSID() {
    wifi_config_t conf {};
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
        return {};
    }

    return std::string(reinterpret_cast<const char*>(conf.sta.ssid));
}

static void triggerWiFiReconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi: Trigger WiFi reconnect...");
        applyAdvancedWiFiSettings(); 
        WiFi.mode(WIFI_STA); 
        WiFi.begin();
    }
}

static std::tuple<int, int> millisToMinutesAndSeconds(long millis) {
    auto secondsRemaining = millis / 1000;
    return { secondsRemaining / 60, secondsRemaining % 60 };
}

static void runConfigPortal(const std::string& ssid, bool hasWifiConfiguration) {
    if (hasWifiConfiguration) {
        Serial.println("WiFi: Configured network not found, opening Config Portal...");
    } else {
        Serial.println("WiFi: No WiFi network configured, opening Config Portal...");
    }

    WiFiManager wm;

    applyAdvancedWiFiSettings();
    wm.setConfigPortalBlocking(false);
    wm.setDisableConfigPortal(true); // allow config portal shutdown when previous configured wifi comes available.
    wm.setConfigPortalTimeout(PORTAL_TIMEOUT / 1000);
    wm.autoConnect("iohc-setup");

    const unsigned long portalStartTime = millis();
    bool portalClosed = false;
    while (!portalClosed) {
        // Keep telling this info to keep it visible on the display
        if (hasWifiConfiguration) {
            displayCustomMessage("WiFi not found.", ssid.c_str());
        } else {
            displayCustomMessage("WiFi not configured.");
        }
        displayCustomMessage("Custom WiFi AP", "iohc-setup");

        const long millisRemaining = PORTAL_TIMEOUT - static_cast<long>(millis() - portalStartTime);
        auto remainingTime = millisToMinutesAndSeconds(millisRemaining);
        displayCustomMessage("Remaining time", format("%2dm %02ds", std::get<0>(remainingTime), std::get<1>(remainingTime)).c_str());

        const bool connected = wm.process(); // Required for async config portal handling

        vTaskDelay(pdMS_TO_TICKS(100));

        if (connected || WiFi.status() == WL_CONNECTED) {
            portalClosed = true;

            if (connected) {
                // workaround for bug in WiFiManager that causes the config portal webserver not to be shut down correctly (keeps port in use)
                esp_restart();
            }
        } else if (millisRemaining < 0) {
            Serial.println("WiFi: Config portal timeout, closing portal...");
            portalClosed = true;

            if (hasWifiConfiguration) {
                Serial.printf("WiFi: Device keeps waiting for connection on network: %s. Restart to re-open config portal.\n", ssid.c_str());
            } else {
                Serial.println("WiFi: Restart the device manually to re-open the config portal!");
            }
        }
    }

    clearDisplayMessages();
}

static void wifiWorker(void * pvParameters) {
    wl_status_t status = WL_DISCONNECTED;

    WiFi.mode(WIFI_STA);

    const std::string ssid = getConfiguredSSID();
    const bool hasWifiConfiguration = !ssid.empty();
    if (hasWifiConfiguration) {
        applyAdvancedWiFiSettings();
        WiFi.begin();

        Serial.printf("WiFi: Attempt connection to '%s', try for max 30 seconds...\n", ssid.c_str());
        status = (wl_status_t)WiFi.waitForConnectResult(30000);
    }
    if (status != WL_CONNECTED) {
        runConfigPortal(ssid, hasWifiConfiguration);
    }

    if (WiFi.status() != WL_CONNECTED) {
        configureWifiDisconnected();
    }

    uint32_t events = 0;
    while (true) {
        xTaskNotifyWait(0, UINT32_MAX, &events, portMAX_DELAY);

        if ((events & WIFI_NOTIFY_DISCONNECTED) != 0) {
            handleWifiDisconnected();
        }

        if ((events & WIFI_NOTIFY_RECONNECT) != 0) {
            triggerWiFiReconnect();
        }

        if ((events & WIFI_NOTIFY_GOT_IP) != 0 &&
            wifiStatus.connectionStatus != ConnState::Connected) {
            handleWifiConnected();
        }
    }
}

static void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            notifyWiFiWorker(WIFI_NOTIFY_GOT_IP);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            notifyWiFiWorker(WIFI_NOTIFY_DISCONNECTED);
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

    xTaskCreatePinnedToCore(wifiWorker, "WiFi_Worker", 8192, NULL, 3, &wifiWorkerTaskHandle, 1);

    WiFi.onEvent(onWiFiEvent);
    WiFi.setAutoReconnect(true);

    Serial.printf("WiFi MAC: %s\n", WiFi.macAddress().c_str());
}
