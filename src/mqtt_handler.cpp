#include <mqtt_handler.h>

#if defined(MQTT)

#include <iohcRemote1W.h>
#include <iohcCryptoHelpers.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <interact.h>
#include <log_buffer.h>
#include <oled_display.h>
#include <cstring>
#include <cstdlib>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_helpers.h>
#include <atomic>

AsyncMqttClient mqttClient;
static const char AVAILABILITY_TOPIC[] = "iown/status";
static const char GATEWAY_ID[] = "MyOpenIO";
static TaskHandle_t s_mqttSchedulerTask = nullptr;
static std::atomic<bool> s_heartbeatEnabled{false};
static std::atomic<uint32_t> s_nextHeartbeatAtMs{0};
static uint32_t s_lastMqttConnectAttemptMs = 0;
static constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
static TaskHandle_t s_mqttPostConnectTask = nullptr;

static void mqttSchedulerTask(void*);
static void publishIohcFrameDiscovery();
static void onMqttConnect(bool sessionPresent);
static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
static void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total);
static void publishHeartbeat();
static void mqttFuncHandler(const char *cmd);
static void mqttPostConnectTask(void*);
static void handleMqttConnectImpl();

static void startHeartbeat() {
    s_heartbeatEnabled.store(true);
    s_nextHeartbeatAtMs.store(millis() + 60000UL);
}

static void stopHeartbeat() {
    s_heartbeatEnabled.store(false);
}

static void storeHardcodedConfigValue(const char* desc, const char* key, std::string &value) {
    if (!nvs_read_string(key, value)) {
        if (value.empty()) {
            Serial.printf("%s not set\n", desc);
        } else {
            nvs_write_string(key, value);
        }
    }
}

void initMqtt() {
    storeHardcodedConfigValue("MQTT server", NVS_KEY_MQTT_SERVER, mqtt_server);
    storeHardcodedConfigValue("MQTT user", NVS_KEY_MQTT_USER, mqtt_user);
    storeHardcodedConfigValue("MQTT password", NVS_KEY_MQTT_PASSWORD, mqtt_password);
    storeHardcodedConfigValue("MQTT discovery", NVS_KEY_MQTT_DISCOVERY, mqtt_discovery_topic);
    storeHardcodedConfigValue("MQTT client id", NVS_KEY_MQTT_CLIENT_ID, mqtt_client_id);

    if (!nvs_read_u16(NVS_KEY_MQTT_PORT, mqtt_port)) {
        nvs_write_u16(NVS_KEY_MQTT_PORT, mqtt_port);
    }

    mqttClient.setWill(AVAILABILITY_TOPIC, 0, true, "offline");
    mqttClient.setClientId(mqtt_client_id.c_str());
    mqttClient.setCredentials(mqtt_user.c_str(), mqtt_password.c_str());
    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);

    if (xTaskCreatePinnedToCore(mqttSchedulerTask, "mqttScheduler", 4096, nullptr,
                                1, &s_mqttSchedulerTask, tskNO_AFFINITY) != pdPASS) {
        Serial.println("Failed to create MQTT scheduler task");
        s_mqttSchedulerTask = nullptr;
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        connectToMqtt();
    }
}

static JsonDocument createDeviceObject(const std::string &id, const std::string &name, const std::string &key = "") {
    JsonDocument device;
    device["identifiers"] = id;
    device["name"] = name;
    device["manufacturer"] = "Somfy";
    device["model"] = "IO Blind Bridge";
    device["sw_version"] = "1.0.0";
    if (!key.empty()) {
        device["serial_number"] = key;
    }
    device["via_device"] = GATEWAY_ID;
    return device;
}

static void publishButtonDiscovery(const std::string &id, const std::string &name,
                                   const std::string &action, const std::string &key) {
    JsonDocument doc;
    doc["name"] = name + " " + action;
    doc["unique_id"] = id + "_" + action;
    doc["command_topic"] = "iown/" + id + "/" + action;
    doc["device"] = createDeviceObject(id, name, key);

    std::string payload;
    size_t len = serializeJson(doc, payload);

    std::string topic = mqtt_discovery_topic + "/button/" + id + "_" + action + "/config";
    mqttClient.publish(topic.c_str(), 0, true, payload.c_str(), len);
}

void publishTravelTimeDiscovery(const std::string &id, const std::string &name,
                                const std::string &key, uint32_t travelTime) {
    JsonDocument doc;
    doc["name"] = name + " travel time";
    doc["unique_id"] = id + "_travel_time";
    doc["command_topic"] = "iown/" + id + "/travel_time/set";
    doc["state_topic"] = "iown/" + id + "/travel_time";
    doc["unit_of_measurement"] = "s";
    doc["min"] = 0;
    doc["max"] = 60;
    doc["step"] = 1;
    doc["device"] = createDeviceObject(id, name, key);

    std::string payload;
    size_t len = serializeJson(doc, payload);

    std::string topic = mqtt_discovery_topic + "/number/" + id + "_travel_time/config";
    mqttClient.publish(topic.c_str(), 0, true, payload.c_str(), len);

    // publish current value
    std::string stateTopic = "iown/" + id + "/travel_time";
    std::string value = std::to_string(travelTime);
    mqttClient.publish(stateTopic.c_str(), 0, true, value.c_str());
}

void publishDiscovery(const std::string &id, const std::string &name, const std::string &key) {
    JsonDocument doc;
    doc["name"] = name;
    doc["unique_id"] = id;
    doc["command_topic"] = "iown/" + id + "/set";
    doc["state_topic"] = "iown/" + id + "/state";
    doc["position_topic"] = "iown/" + id + "/position";
    doc["set_position_topic"] = "iown/" + id + "/position/set";
    doc["availability_topic"] = AVAILABILITY_TOPIC;
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["payload_open"] = "OPEN";
    doc["payload_close"] = "CLOSE";
    doc["payload_stop"] = "STOP";
    doc["state_closed"] = "CLOSE";
    doc["state_open"] = "OPEN";
    doc["state_closing"] = "CLOSING";
    doc["state_opening"] = "OPENING";
    doc["state_stopped"] = "STOP";
    doc["device_class"] = "blind";
    doc["expire_after"] = 120;
    doc["optimistic"] = false;
    doc["retain"] = true;
    doc["qos"] = 0;
    doc["device"] = createDeviceObject(id, name, key);

    std::string payload;
    size_t len = serializeJson(doc, payload);

    std::string topic = mqtt_discovery_topic + "/cover/" + id + "/config";
    mqttClient.publish(topic.c_str(), 0, true, payload.c_str(), len);

    publishButtonDiscovery(id, name, "pair", key);
    publishButtonDiscovery(id, name, "add", key);
    publishButtonDiscovery(id, name, "remove", key);
}

void removeDiscovery(const std::string &id) {
    std::string topic = mqtt_discovery_topic + "/cover/" + id + "/config";
    mqttClient.publish(topic.c_str(), 0, true, "", 0);

    auto removeButton = [&](const std::string &action) {
        std::string t = mqtt_discovery_topic + "/button/" + id + "_" + action + "/config";
        mqttClient.publish(t.c_str(), 0, true, "", 0);
    };

    removeButton("pair");
    removeButton("add");
    removeButton("remove");

    std::string t = mqtt_discovery_topic + "/number/" + id + "_travel_time/config";
    mqttClient.publish(t.c_str(), 0, true, "", 0);
}

void publishHeartbeat() {
    mqttClient.publish(AVAILABILITY_TOPIC, 0, true, "online");
}

void publishCoverState(const std::string &id, const char *state) {
    std::string topic = "iown/" + id + "/state";
    mqttClient.publish(topic.c_str(), 0, true, state);
}

void publishCoverPosition(const std::string &id, float position) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%.0f", position);
    std::string topic = "iown/" + id + "/position";
    mqttClient.publish(topic.c_str(), 0, true, buf);
}

// ==== BELANGRIJK: scheduler die het zware werk in een eigen task zet ====
void handleMqttConnect() {
    if (mqttStatus != ConnState::Connected) return;
    if (s_mqttPostConnectTask) return; // al bezig
    xTaskCreatePinnedToCore(
        mqttPostConnectTask,
        "mqttPostConnect",
        4096,      // stack
        nullptr,
        1,         // prioriteit laag
        &s_mqttPostConnectTask,
        tskNO_AFFINITY
    );
}

static void mqttPostConnectTask(void* /*arg*/) {
    handleMqttConnectImpl();     // oude body van handleMqttConnect()
    s_mqttPostConnectTask = nullptr;
    vTaskDelete(nullptr);
}

static void handleMqttConnectImpl() {
    // Discovery van de ‘frame’ sensor eerst, zodat state pub direct een entity heeft
    publishIohcFrameDiscovery();
    const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
    for (const auto &r : remotes) {
        std::string id = bytesToHexString(r.node, sizeof(r.node));
        std::string key = bytesToHexString(r.key, sizeof(r.key));
        std::string name = r.name.empty() ? r.description : r.name;
        publishDiscovery(id, name, key);
        publishTravelTimeDiscovery(id, name, key, r.travelTime);
        //std::string t = "iown/" + id + "/set";
        //mqttClient.subscribe(t.c_str(), 0);
        //mqttClient.subscribe(("iown/" + id + "/pair").c_str(), 0);
        //mqttClient.subscribe(("iown/" + id + "/add").c_str(), 0);
        //mqttClient.subscribe(("iown/" + id + "/remove").c_str(), 0);
        //mqttClient.subscribe(("iown/" + id + "/travel_time/set").c_str(), 0);
        vTaskDelay(pdMS_TO_TICKS(200)); // throttle per device
    }
    startHeartbeat();
    publishHeartbeat();
}

void connectToMqtt() {
    if (mqttClient.connected() || mqttStatus == ConnState::Connecting) {
        return;  // Avoid parallel connection attempts
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping MQTT connection");
        return;
    }
    if (mqtt_server.empty()) {
        Serial.println("MQTT server not configured");
        return;
    }
    s_lastMqttConnectAttemptMs = millis();
    Serial.printf("Connecting to MQTT at %s:%u...\n", mqtt_server.c_str(), mqtt_port);
    addLogMessage(String("Connecting to MQTT at ") + mqtt_server.c_str() + ":" + String(mqtt_port));
    mqttStatus = ConnState::Connecting;
    updateDisplayStatus();
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    addLogMessage(String("Connected to MQTT at ") + mqtt_server.c_str() + ":" + String(mqtt_port));
    mqttStatus = ConnState::Connected;
    updateDisplayStatus();

    //mqttClient.subscribe("iown/powerOn", 0);
    //mqttClient.subscribe("iown/setPresence", 0);
    //mqttClient.subscribe("iown/setWindow", 0);
    //mqttClient.subscribe("iown/setTemp", 0);
    //mqttClient.subscribe("iown/setMode", 0);
    //mqttClient.subscribe("iown/midnight", 0);
    //mqttClient.subscribe("iown/associate", 0);
    //mqttClient.subscribe("iown/heatState", 0);
    // mqttClient.subscribe("iown/#", 0);  // DEBUG: later weer weghalen als alles werkt

    mqttClient.subscribe("iown/+/set", 0);
    mqttClient.subscribe("iown/+/position/set", 0);
    mqttClient.subscribe("iown/+/pair", 0);
    mqttClient.subscribe("iown/+/add", 0);
    mqttClient.subscribe("iown/+/remove", 0);
    mqttClient.subscribe("iown/+/travel_time/set", 0);

    //mqttClient.publish("iown/Frame", 0, false, R"({"cmd": "powerOn", "_data": "Gateway"})", 38);

    // Belangrijk: discovery/subscribes/heartbeat NU via worker task
    handleMqttConnect();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.print("Disconnected from MQTT. Reason: ");
    Serial.println(static_cast<uint8_t>(reason));
    addLogMessage(String("Disconnected from MQTT (reason ") + String(static_cast<uint8_t>(reason)) + ")");
    mqttStatus = ConnState::Disconnected;
    updateDisplayStatus();
    stopHeartbeat();
}

static void mqttSchedulerTask(void*) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        const uint32_t now = millis();

        if (mqttStatus == ConnState::Disconnected && WiFi.status() == WL_CONNECTED &&
            static_cast<int32_t>(now - s_lastMqttConnectAttemptMs) >= static_cast<int32_t>(MQTT_RECONNECT_INTERVAL_MS)) {
            connectToMqtt();
        }

        if (s_heartbeatEnabled.load() &&
            static_cast<int32_t>(now - s_nextHeartbeatAtMs.load()) >= 0) {
            s_nextHeartbeatAtMs.store(now + 60000UL);
            if (mqttStatus == ConnState::Connected && mqttClient.connected()) {
                publishHeartbeat();
            }
        }
    }
}

static void publishIohcFrameDiscovery() {
    JsonDocument configDoc;
    configDoc["name"] = "IOHC Frame";
    configDoc["state_topic"] = mqtt_discovery_topic + "/sensor/iohc_frame/state";
    configDoc["unique_id"] = "iohc_frame";
    configDoc["json_attributes_topic"] = mqtt_discovery_topic + "/sensor/iohc_frame/state";
    configDoc["device"] = createDeviceObject(GATEWAY_ID, "My Open IO Gateway");

    std::string cfg;
    size_t cfgLen = serializeJson(configDoc, cfg);
    mqttClient.publish((mqtt_discovery_topic + "/sensor/iohc_frame/config").c_str(),
                       0, true, cfg.c_str(), cfgLen);
}


void mqttFuncHandler(const char *cmd) {
    constexpr char delim = ' ';
    Tokens segments;
    tokenize(cmd + 5, delim, segments);
    Serial.printf("Search for %s\t", segments[0].c_str());
    for (uint8_t idx = 0; idx <= lastEntry; ++idx) {
        if (_cmdHandler[idx] == nullptr) continue;
        if (segments[0].find(_cmdHandler[idx]->cmd) != std::string::npos) {
            Serial.printf(" %s %s (%s)\n", _cmdHandler[idx]->cmd,
                          segments.size() > 1 ? segments[1].c_str() : "No param",
                          _cmdHandler[idx]->description);
            _cmdHandler[idx]->handler(&segments);
            return;
        }
    }
    Serial.printf("*> MQTT Unknown %s <*\n", segments[0].c_str());
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total) {
    if (!topic || !payload || len == 0) return;

    // Safe copy of payload
    char buf[len + 1];
    memcpy(buf, payload, len);
    buf[len] = '\0';

    Serial.printf("Received MQTT %s %s %d\n", topic, buf, len);

    std::string topicStr(topic);
    std::string payloadStr(buf);

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/travel_time/set", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/travel_time/set", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            uint32_t tt = strtoul(payloadStr.c_str(), nullptr, 10);
            if (tt > 0) {
                IOHC::iohcRemote1W::getInstance()->setTravelTime(it->description, tt);
                std::string stateTopic = "iown/" + id + "/travel_time";
                std::string val = std::to_string(tt);
                mqttClient.publish(stateTopic.c_str(), 0, true, val.c_str());
            }
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        }
        return;
    }

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/position/set", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/position/set", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            int openVal = atoi(payloadStr.c_str());
            openVal = std::clamp(openVal, 0, 100);
            int closeVal = 100 - openVal;
            Tokens t;
            t.push_back(std::to_string(closeVal));
            t.push_back(it->description);
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Absolute, &t);
            std::string stateTopic = "iown/" + id + "/state";
            const char *state = (openVal >= 99) ? "OPEN" : (openVal <= 1 ? "CLOSE" : "STOP");
            mqttClient.publish(stateTopic.c_str(), 0, true, state);
            std::string posTopic = "iown/" + id + "/position";
            std::string openStr = std::to_string(openVal);
            mqttClient.publish(posTopic.c_str(), 0, true, openStr.c_str());
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        }
        return;
    }

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/absolute/set", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/absolute/set", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            Tokens t;
            t.push_back(payloadStr);
            t.push_back(it->description);
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Absolute, &t);
            std::string stateTopic = "iown/" + id + "/state";
            int val = atoi(payloadStr.c_str());
            int openVal = 100 - std::clamp(val, 0, 100);
            const char *state = (openVal >= 99) ? "OPEN" : (openVal <= 1 ? "CLOSE" : "STOP");
            mqttClient.publish(stateTopic.c_str(), 0, true, state);
            std::string posTopic = "iown/" + id + "/position";
            std::string openStr = std::to_string(openVal);
            mqttClient.publish(posTopic.c_str(), 0, true, openStr.c_str());
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        }
        return;
    }

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/set", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/set", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            Tokens t;
            std::transform(payloadStr.begin(), payloadStr.end(), payloadStr.begin(), ::tolower);
            t.push_back(payloadStr);
            t.push_back(it->description);
            std::string stateTopic = "iown/" + id + "/state";

            if (payloadStr == "open") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Open, &t);
                mqttClient.publish(stateTopic.c_str(), 0, true, "OPEN");
            } else if (payloadStr == "close") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Close, &t);
                mqttClient.publish(stateTopic.c_str(), 0, true, "CLOSE");
            } else if (payloadStr == "stop") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Stop, &t);
                mqttClient.publish(stateTopic.c_str(), 0, true, "STOP");
            } else if (payloadStr == "vent") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Vent, &t);
            } else if (payloadStr == "force") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::ForceOpen, &t);
            } else {
                Serial.printf("*> MQTT Unknown %s <*\n", payloadStr.c_str());
            }
            // Clear retained set message
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        } else {
            Serial.printf("*> MQTT Unknown device %s <*\n", id.c_str());
        }
        return;
    }

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/pair", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/pair", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            Tokens t;
            t.push_back("pair");
            t.push_back(it->description);
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Pair, &t);
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        }
        return;
    }

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/add", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/add", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            Tokens t;
            t.push_back("add");
            t.push_back(it->description);
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Add, &t);
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        }
        return;
    }

    if (topicStr.rfind("iown/", 0) == 0 && topicStr.find("/remove", 5) != std::string::npos) {
        std::string id = topicStr.substr(5, topicStr.find("/remove", 5) - 5);
        std::transform(id.begin(), id.end(), id.begin(), ::tolower);
        const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const auto &r) {
            return bytesToHexString(r.node, sizeof(r.node)) == id;
        });
        if (it != remotes.end()) {
            Tokens t;
            t.push_back("remove");
            t.push_back(it->description);
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Remove, &t);
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        }
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, buf) != DeserializationError::Ok) {
        Serial.println(F("Failed to parse JSON"));
        return;
    }

    const char *data = doc["_data"];
    size_t bufferSize = strlen(topic) + (data ? strlen(data) : 0) + 7;
    char message[bufferSize];
    if (!data)
        snprintf(message, sizeof(message), "MQTT %s", topic);
    else
        snprintf(message, sizeof(message), "MQTT %s %s", topic, data);
    mqttFuncHandler(message);
}
#endif // MQTT
