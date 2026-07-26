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

void initMqtt();
void connectToMqtt();
void publishDiscovery(const std::string &id, const std::string &name, const std::string &key);
void publishTravelTimeDiscovery(const std::string &id, const std::string &name,
                                const std::string &key, uint32_t travelTime);
void handleMqttConnect();
void publishCoverState(const std::string &id, const char *state);
void publishCoverPosition(const std::string &id, float position);
void publishVersionInfo();
void removeDiscovery(const std::string &id);

#endif // MQTT

#if !defined(MQTT)
inline void publishVersionInfo() {}
#endif

#endif // MQTT_HANDLER_H
