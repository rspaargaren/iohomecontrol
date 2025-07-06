#include <mqtt_handler.h>

#include <iohcRemote1W.h>
#include <iohcCryptoHelpers.h>

//#if defined(MQTT)
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <interact.h>


AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t heartbeatTimer;
const char AVAILABILITY_TOPIC[] = "iown/status";

void publishDiscovery(const std::string &id, const std::string &name) {
    JsonDocument doc;
    doc["name"] = name;
    doc["unique_id"] = id;
    doc["command_topic"] = "iown/" + id + "/set";
    doc["state_topic"] = "iown/" + id + "/state";
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

    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"] = "MyOpenIO";
    device["name"] = "My Open IO Gateway";
    device["manufacturer"] = "Somfy";
    device["model"] = "IO Blind Bridge";
    device["sw_version"] = "1.0.0";

    std::string payload;
    size_t len = serializeJson(doc, payload);

    std::string topic = "homeassistant/cover/" + id + "/config";
    mqttClient.publish(topic.c_str(), 0, true, payload.c_str(), len);
}

void publishHeartbeat(TimerHandle_t) {
    mqttClient.publish(AVAILABILITY_TOPIC, 0, true, "online");
}

void handleMqttConnect() {
    const auto &remotes = IOHC::iohcRemote1W::getInstance()->getRemotes();
    for (const auto &r : remotes) {
        std::string id = bytesToHexString(r.node, sizeof(r.node));
        publishDiscovery(id, r.description);
        std::string t = "iown/" + id + "/set";
        mqttClient.subscribe(t.c_str(), 0);
    }
    if (!heartbeatTimer)
        heartbeatTimer = xTimerCreate("hb", pdMS_TO_TICKS(60000), pdTRUE, nullptr, publishHeartbeat);
    xTimerStart(heartbeatTimer, 0);
    publishHeartbeat(nullptr);
}

void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttStatus = ConnState::Connecting;
    updateDisplayStatus();
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    mqttStatus = ConnState::Connected;
    updateDisplayStatus();

    mqttClient.subscribe("iown/powerOn", 0);
    mqttClient.subscribe("iown/setPresence", 0);
    mqttClient.subscribe("iown/setWindow", 0);
    mqttClient.subscribe("iown/setTemp", 0);
    mqttClient.subscribe("iown/setMode", 0);
    mqttClient.subscribe("iown/midnight", 0);
    mqttClient.subscribe("iown/associate", 0);
    mqttClient.subscribe("iown/heatState", 0);

    mqttClient.publish("iown/Frame", 0, false, R"({"cmd": "powerOn", "_data": "Gateway"})", 38);
    {
        JsonDocument configDoc;
        configDoc["name"] = "IOHC Frame";
        configDoc["state_topic"] = "homeassistant/sensor/iohc_frame/state";
        configDoc["unique_id"] = "iohc_frame";
        configDoc["json_attributes_topic"] = "homeassistant/sensor/iohc_frame/state";
        JsonObject device = configDoc["device"].to<JsonObject>();
        device["identifiers"] = "iogateway";
        device["name"] = "MyOpenIO";
        std::string cfg;
        size_t cfgLen = serializeJson(configDoc, cfg);
        mqttClient.publish("homeassistant/sensor/iohc_frame/config", 0, true, cfg.c_str(), cfgLen);
    }
    handleMqttConnect();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason) {
    Serial.println("Disconnected from MQTT.");
    mqttStatus = ConnState::Disconnected;
    updateDisplayStatus();
    xTimerStart(mqttReconnectTimer, 0);
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
    if (topic[0] == '\0') return;
    payload[len] = '\0';
    Serial.printf("Received MQTT %s %s %d\n", topic, payload, len);

    std::string topicStr(topic);
    std::string payloadStr(payload);

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
                mqttClient.publish(stateTopic.c_str(), 0, true, "open");
            } else if (payloadStr == "close") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Close, &t);
                mqttClient.publish(stateTopic.c_str(), 0, true, "closed");
            } else if (payloadStr == "stop") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Stop, &t);
            } else if (payloadStr == "vent") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Vent, &t);
            } else if (payloadStr == "force") {
                IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::ForceOpen, &t);
            } else {
                Serial.printf("*> MQTT Unknown %s <*\n", payloadStr.c_str());
            }
            // Clear retained set message to avoid re-trigger on reconnect
            mqttClient.publish(topicStr.c_str(), 0, true, "", 0);
        } else {
            Serial.printf("*> MQTT Unknown device %s <*\n", id.c_str());
        }
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        Serial.println(F("Failed to parse JSON"));
        return;
    }

    const char *data = doc["_data"];
    size_t bufferSize = strlen(topic) + strlen(data) + 7;
    char message[bufferSize];
    if (!data)
        snprintf(message, sizeof(message), "MQTT %s", topic);
    else
        snprintf(message, sizeof(message), "MQTT %s %s", topic, data);
    mqttFuncHandler(message);
}

//#endif // MQTT

