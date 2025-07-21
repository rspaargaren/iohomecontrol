#ifndef NVS_HELPERS_H
#define NVS_HELPERS_H

#include <iohcPacket.h>

bool nvs_init();
bool nvs_read_sequence(const IOHC::address addr, uint16_t *sequence);
void nvs_write_sequence(const IOHC::address addr, uint16_t sequence);

#endif // NVS_HELPERS_H
