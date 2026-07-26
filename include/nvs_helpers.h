#pragma once
#include <nvs_flash.h>
#include <nvs.h>
#include <string>

#define NVS_NAMESPACE              "miopenio"
#define NVS_KEY_DISPLAY_ENABLED    "disp_en"
#define NVS_KEY_DISPLAY_DIM_AFTER  "disp_dim_s"
#define NVS_KEY_DISPLAY_OFF_AFTER  "disp_off_s"

// MQTT NVS keys
#define NVS_KEY_MQTT_SERVER        "mqtt_server"
#define NVS_KEY_MQTT_USER          "mqtt_user"
#define NVS_KEY_MQTT_PASSWORD      "mqtt_pass"
#define NVS_KEY_MQTT_PORT          "mqtt_port"
#define NVS_KEY_MQTT_CLIENT_ID     "mqtt_client"
#define NVS_KEY_MQTT_DISCOVERY     "mqtt_disc"

bool nvs_read_bool(const char* key, bool& value);
bool nvs_write_bool(const char* key, bool value);
bool nvs_read_int(const char* key, int& value);
bool nvs_write_int(const char* key, int value);
bool nvs_read_string(const char* key, std::string& value);
bool nvs_write_string(const char* key, const std::string& value);
bool nvs_read_u16(const char* key, uint16_t& value);
bool nvs_write_u16(const char* key, uint16_t value);
