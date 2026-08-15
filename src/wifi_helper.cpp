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
#include <nvs_helpers.h>
#include <log_buffer.h>
#if defined(MQTT)
#include <mqtt_handler.h>
#endif
#if defined(SYSLOG)
#include <syslog_helper.h>
#endif
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <TickerUsESP32.h>
#include <tuple>

#define LOG_TAG "wifi"

const long PORTAL_TIMEOUT = 300000; // 5 minuten = 300.000 ms, used only as a fallback if NVS has no configured timeout
const uint32_t WIFI_NOTIFY_GOT_IP = BIT0;
const uint32_t WIFI_NOTIFY_DISCONNECTED = BIT1;
const uint32_t WIFI_NOTIFY_RECONNECT = BIT2;
static constexpr uint32_t WIFI_STACK_RESET_AFTER_MS = 120000;  // 2 min disconnected -> reset the WiFi stack
static constexpr uint32_t WIFI_RESTART_AFTER_MS = 1800000;     // 30 min disconnected -> restart the device

// below variables are thread safe because of the use of a single task that reads/modifies them (except for wifiStatus, but that one has atomic fields)
TimersUS::TickerUsESP32 wifiReconnectTimer {};
TimersUS::TickerUsESP32 rssiTimer {};
static uint32_t s_wifiDisconnectedSinceMs = 0;
static uint32_t s_lastWiFiStackResetMs = 0;
static uint16_t s_wifiReconnectAttempts = 0;
WiFiStatus wifiStatus = { ConnState::Disconnected, 0, 0 };

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
        wifiStatus.rssi = WiFi.RSSI();
        wifiStatus.signalStrengthPercent = rssiToQuality(wifiStatus.rssi);
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
        if (!mqttClient.connected() && mqttStatus != ConnState::Connecting) {
            connectToMqtt();
        }
#endif
}

static void handleWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        wifiStatus.connectionStatus = ConnState::Connected;
        wifiStatus.rssi = WiFi.RSSI();
        wifiStatus.signalStrengthPercent = rssiToQuality(wifiStatus.rssi);
        s_wifiDisconnectedSinceMs = 0;
        s_lastWiFiStackResetMs = 0;
        s_wifiReconnectAttempts = 0;

        if (WiFi.getMode() == WIFI_AP_STA) {
            ESP_LOGI(LOG_TAG, "Connected, disabling fallback AP mode");
            WiFi.mode(WIFI_STA);
        }

        wifiReconnectTimer.detach();
        rssiTimer.attach(5, rssiTimerCb);
        updateDisplayStatus();

        if (!mdnsStarted) {
            if (!MDNS.begin("miopenio")) {
                ESP_LOGE(LOG_TAG, "mDNS start failed");
            } else {
                mdnsStarted = true;
                ESP_LOGI(LOG_TAG, "mDNS started at http://miopenio.local");
            }
        }

        ensureWebServerStarted();
        onMqttAfterWifi();
#if defined(SYSLOG)
        resetSyslog();
        initSyslog();
#endif
    }
}

static void configureWifiDisconnected() {
    ESP_LOGW(LOG_TAG, "Connection lost (event)");
    wifiStatus.connectionStatus = ConnState::Disconnected;
    wifiStatus.signalStrengthPercent = 0;
    wifiStatus.rssi = 0;
    if (s_wifiDisconnectedSinceMs == 0) {
        s_wifiDisconnectedSinceMs = millis();
    }
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

String getConfiguredWiFiSSID() {
    return String(getConfiguredSSID().c_str());
}

void saveWiFiCredentials(const String &ssid, const String &password) {
    String passwordToSave = password;
    if (passwordToSave.length() == 0) {
        wifi_config_t conf {};
        if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
            passwordToSave = String(reinterpret_cast<const char*>(conf.sta.password));
        }
    }

    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), passwordToSave.c_str());
}

static bool readNetworkStringValue(const char *key, String &out) {
    std::string value;
    if (!nvs_read_string(key, value) || value.empty()) {
        return false;
    }
    out = String(value.c_str());
    return true;
}

static String readNetworkString(const char *key, const char *defaultValue = "") {
    String value;
    return readNetworkStringValue(key, value) ? value : String(defaultValue);
}

static bool parseIPAddressValue(const String &value, IPAddress &address) {
    return value.length() > 0 && address.fromString(value);
}

static String getSavedHostname() {
    String hostname = readNetworkString(NVS_KEY_NET_HOST, "MiOpenIO");
    hostname.trim();
    return hostname.length() > 0 ? hostname : String("MiOpenIO");
}

static void applySavedNetworkSettings() {
    const String hostname = getSavedHostname();
    WiFi.setHostname(hostname.c_str());

    bool dhcp = true;
    nvs_read_bool(NVS_KEY_NET_DHCP, dhcp);
    if (dhcp) {
        return;
    }

    IPAddress ip, gateway, mask, dns1, dns2;
    const bool hasIp = parseIPAddressValue(readNetworkString(NVS_KEY_NET_IP), ip);
    const bool hasGateway = parseIPAddressValue(readNetworkString(NVS_KEY_NET_GW), gateway);
    const bool hasMask = parseIPAddressValue(readNetworkString(NVS_KEY_NET_MASK), mask);
    parseIPAddressValue(readNetworkString(NVS_KEY_NET_DNS1), dns1);
    parseIPAddressValue(readNetworkString(NVS_KEY_NET_DNS2), dns2);

    if (hasIp && hasGateway && hasMask) {
        if (!WiFi.config(ip, gateway, mask, dns1, dns2)) {
            ESP_LOGE(LOG_TAG, "static network config failed, falling back to DHCP");
        }
    } else {
        ESP_LOGE(LOG_TAG, "static network config incomplete, falling back to DHCP");
    }
}

static bool readFallbackEnabled() {
    bool enabled = true;
    nvs_read_bool(NVS_KEY_FB_ENABLED, enabled);
    return enabled;
}

static uint16_t readFallbackU16(const char *key, uint16_t fallback) {
    uint16_t value = fallback;
    nvs_read_u16(key, value);
    return value;
}

static uint32_t readFallbackTimeoutMs() {
    const uint16_t timeoutSeconds = readFallbackU16(NVS_KEY_FB_TIMEOUT, 600);
    return static_cast<uint32_t>(timeoutSeconds) * 1000UL;
}

static void resetWiFiStack() {
    ESP_LOGW(LOG_TAG, "resetting WiFi stack after prolonged disconnect...");
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(250));
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.mode(WIFI_STA);
    applySavedNetworkSettings();
    applyAdvancedWiFiSettings();
    WiFi.begin();
    s_lastWiFiStackResetMs = millis();
}

static void runConfigPortal(const std::string& ssid, bool hasWifiConfiguration);

static void triggerWiFiReconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW(LOG_TAG, "Trigger WiFi reconnect...");
        applyAdvancedWiFiSettings();
        WiFi.mode(WIFI_STA);
        WiFi.begin();
        s_wifiReconnectAttempts++;

        const uint32_t now = millis();

        // After N failed reconnect attempts, open a fallback AP so the
        // device stays reachable for reconfiguration.
        const uint16_t runtimeFallbackRetries = readFallbackU16(NVS_KEY_FB_RUN, 3);
        if (readFallbackEnabled() && runtimeFallbackRetries > 0 &&
            s_wifiReconnectAttempts >= runtimeFallbackRetries) {
            ESP_LOGW(LOG_TAG, "opening fallback AP after reconnect retries");
            s_wifiReconnectAttempts = 0;
            runConfigPortal(getConfiguredSSID(), true);
            return;
        }

        if (s_wifiDisconnectedSinceMs != 0) {
            if (static_cast<int32_t>(now - s_wifiDisconnectedSinceMs) >= static_cast<int32_t>(WIFI_STACK_RESET_AFTER_MS) &&
                static_cast<int32_t>(now - s_lastWiFiStackResetMs) >= static_cast<int32_t>(WIFI_STACK_RESET_AFTER_MS)) {
                resetWiFiStack();
            }

            if (static_cast<int32_t>(now - s_wifiDisconnectedSinceMs) >= static_cast<int32_t>(WIFI_RESTART_AFTER_MS)) {
                ESP_LOGE(LOG_TAG, "disconnected too long, restarting device");
                esp_restart();
            }
        }
    }
}

static std::tuple<int, int> millisToMinutesAndSeconds(long millis) {
    auto secondsRemaining = millis / 1000;
    return { secondsRemaining / 60, secondsRemaining % 60 };
}

static void runConfigPortal(const std::string& ssid, bool hasWifiConfiguration) {
    if (hasWifiConfiguration) {
        ESP_LOGW(LOG_TAG, "Configured network not found, opening Config Portal...");
    } else {
        ESP_LOGW(LOG_TAG, "No WiFi network configured, opening Config Portal...");
    }

    WiFiManager wm;

    applyAdvancedWiFiSettings();
    wm.setConfigPortalBlocking(false);
    wm.setDisableConfigPortal(true); // allow config portal shutdown when previous configured wifi comes available.
    const uint32_t portalTimeoutMs = readFallbackTimeoutMs();
    if (portalTimeoutMs > 0) {
        wm.setConfigPortalTimeout(portalTimeoutMs / 1000);
    }
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

        const long millisRemaining = portalTimeoutMs == 0 ? 0 : static_cast<long>(portalTimeoutMs) - static_cast<long>(millis() - portalStartTime);
        if (portalTimeoutMs > 0) {
            auto remainingTime = millisToMinutesAndSeconds(millisRemaining);
            displayCustomMessage("Remaining time", format("%2dm %02ds", std::get<0>(remainingTime), std::get<1>(remainingTime)).c_str());
        } else {
            displayCustomMessage("Remaining time", "disabled");
        }

        const bool connected = wm.process(); // Required for async config portal handling

        vTaskDelay(pdMS_TO_TICKS(100));

        if (connected || WiFi.status() == WL_CONNECTED) {
            portalClosed = true;

            if (connected) {
                // workaround for bug in WiFiManager that causes the config portal webserver not to be shut down correctly (keeps port in use)
                esp_restart();
            }
        } else if (portalTimeoutMs > 0 && millisRemaining < 0) {
            ESP_LOGI(LOG_TAG, "Config portal timeout, closing portal...");
            portalClosed = true;

            if (hasWifiConfiguration) {
                ESP_LOGW(LOG_TAG, "Device keeps waiting for connection on network: %s. Restart to re-open config portal.", ssid.c_str());
            } else {
                ESP_LOGE(LOG_TAG, "Restart the device manually to re-open the config portal!");
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
        const uint16_t bootRetries = readFallbackU16(NVS_KEY_FB_BOOT, 3);
        for (uint16_t attempt = 0; attempt < bootRetries && status != WL_CONNECTED; ++attempt) {
            applyAdvancedWiFiSettings();
            WiFi.begin();
            ESP_LOGI(LOG_TAG, "Attempt connection to '%s' (%u/%u)...", ssid.c_str(), attempt + 1, bootRetries);
            status = (wl_status_t)WiFi.waitForConnectResult(10000);
        }
    }
    if (status != WL_CONNECTED) {
        // If there's no saved network, we always need the portal so the
        // device can be configured at all. If a network IS configured but
        // unreachable, only open the portal when fallback AP is enabled.
        if (!hasWifiConfiguration || readFallbackEnabled()) {
            runConfigPortal(ssid, hasWifiConfiguration);
        }
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
    // Set hostname (and any saved static IP config) before the WiFi stack
    // initialises so the DHCP client advertises the correct name from the
    // very first connection attempt, including after auto-reconnects where
    // the connect task never runs.
    applySavedNetworkSettings();

    xTaskCreatePinnedToCore(wifiWorker, "WiFi_Worker", 8192, NULL, 3, &wifiWorkerTaskHandle, 1);

    WiFi.onEvent(onWiFiEvent);
    WiFi.setAutoReconnect(true);

    ESP_LOGD(LOG_TAG, "MAC: %s", WiFi.macAddress().c_str());
}

void clearWifi() {
    WiFi.eraseAP();
    esp_restart();
}
