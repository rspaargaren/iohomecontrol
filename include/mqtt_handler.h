//   Copyright (c) 2025. CRIDP https://github.com/cridp
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//           http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

/* MQTT support can be enabled or disabled via the `MQTT` define in
 * `user_config.h`.  When disabled, this header becomes effectively empty so
 * other source files can include it unconditionally. */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H
#include <user_config.h>

#if defined(MQTT)

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
void publishDiscovery(const std::string &id, const std::string &name, const std::string &key);
void handleMqttConnect();
void publishHeartbeat(TimerHandle_t timer);
void mqttFuncHandler(const char *cmd);
void publishCoverState(const std::string &id, const char *state);
void publishCoverPosition(const std::string &id, float position);

#endif // MQTT

#endif //MQTT_HANDLER_H