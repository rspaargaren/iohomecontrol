#ifndef NVS_HELPERS_H
#define NVS_HELPERS_H

#include <iohcPacket.h>
#include <string>

// NVS keys for persisted MQTT configuration. Keys must be <=15 characters.
static constexpr char NVS_KEY_MQTT_SERVER[] = "mqtt_server";
static constexpr char NVS_KEY_MQTT_USER[] = "mqtt_user";
static constexpr char NVS_KEY_MQTT_PASSWORD[] = "mqtt_password";
static constexpr char NVS_KEY_MQTT_DISCOVERY[] = "mqtt_disc_topic";


bool nvs_init();
bool nvs_read_sequence(const IOHC::address addr, uint16_t *sequence);
void nvs_write_sequence(const IOHC::address addr, uint16_t sequence);

bool nvs_read_string(const char *key, std::string &value);
void nvs_write_string(const char *key, const std::string &value);

#endif // NVS_HELPERS_H
