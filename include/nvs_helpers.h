#ifndef NVS_HELPERS_H
#define NVS_HELPERS_H

#include <iohcPacket.h>
#include <string>

bool nvs_init();
bool nvs_read_sequence(const IOHC::address addr, uint16_t *sequence);
void nvs_write_sequence(const IOHC::address addr, uint16_t sequence);

bool nvs_read_string(const char *key, std::string &value);
void nvs_write_string(const char *key, const std::string &value);

#endif // NVS_HELPERS_H
