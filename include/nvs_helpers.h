#ifndef NVS_HELPERS_H
#define NVS_HELPERS_H

#include <iohcPacket.h>
#include <string>

// NVS keys for persisted MQTT configuration. Keys must be <=15 characters.
static constexpr char NVS_KEY_MQTT_SERVER[] = "mqtt_server";
static constexpr char NVS_KEY_MQTT_USER[] = "mqtt_user";
static constexpr char NVS_KEY_MQTT_PASSWORD[] = "mqtt_password";
static constexpr char NVS_KEY_MQTT_DISCOVERY[] = "mqtt_disc_topic";
static constexpr char NVS_KEY_MQTT_CLIENT_ID[] = "mqtt_client_id";
static constexpr char NVS_KEY_MQTT_PORT[] = "mqtt_port";
static constexpr char NVS_KEY_SYSLOG_ENABLED[] = "syslog_enabled";
static constexpr char NVS_KEY_SYSLOG_SERVER[] = "syslog_server";
static constexpr char NVS_KEY_SYSLOG_PORT[] = "syslog_port";


bool nvs_init();
bool nvs_read_sequence(const IOHC::address addr, uint16_t *sequence);
void nvs_write_sequence(const IOHC::address addr, uint16_t sequence);

bool nvs_read_string(const char *key, std::string &value);
void nvs_write_string(const char *key, const std::string &value);
bool nvs_read_u16(const char *key, uint16_t &value);
void nvs_write_u16(const char *key, uint16_t value);
bool nvs_read_bool(const char *key, bool &value);
void nvs_write_bool(const char *key, bool value);

#endif // NVS_HELPERS_H
