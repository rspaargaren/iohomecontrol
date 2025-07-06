#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

//#include <interact.h>

//#if defined(MQTT)

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

extern AsyncMqttClient mqttClient;
extern TimerHandle_t mqttReconnectTimer;
extern TimerHandle_t heartbeatTimer;
extern const char AVAILABILITY_TOPIC[];

void initMqtt();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char *topic, char *payload,
                   AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total);
void publishDiscovery(const std::string &id, const std::string &name);
void handleMqttConnect();
void publishHeartbeat(TimerHandle_t timer);
void mqttFuncHandler(const char *cmd);

//#endif // MQTT

#endif // MQTT_HANDLER_H

