#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H
#include <user_config.h>

/* MQTT support can be enabled or disabled via the `MQTT` define in
 * `user_config.h`.  When disabled, this header becomes effectively empty so
 * other source files can include it unconditionally. */

#if defined(MQTT)

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

extern AsyncMqttClient mqttClient;
extern TimerHandle_t mqttReconnectTimer;
extern TimerHandle_t heartbeatTimer;
extern const char AVAILABILITY_TOPIC[];

void initMqtt();
void connectToMqtt();
static void publishIohcFrameDiscovery();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total);
void publishDiscovery(const std::string &id, const std::string &name, const std::string &key);
void publishTravelTimeDiscovery(const std::string &id, const std::string &name,
                                const std::string &key, uint32_t travelTime);
void handleMqttConnect();
void publishHeartbeat(TimerHandle_t timer);
void mqttFuncHandler(const char *cmd);
void publishCoverState(const std::string &id, const char *state);
void publishCoverPosition(const std::string &id, float position);
void removeDiscovery(const std::string &id);
static TaskHandle_t s_mqttPostConnectTask = nullptr;
static void mqttPostConnectTask(void*);
static void handleMqttConnectImpl();

#endif // MQTT

#endif // MQTT_HANDLER_H

