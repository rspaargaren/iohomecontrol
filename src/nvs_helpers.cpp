#include <Preferences.h>
#include "nvs_helpers.h"

static Preferences prefs;
static bool initialized = false;

bool nvs_init() {
    if (!initialized) {
        initialized = prefs.begin("seq", false);
    }
    return initialized;
}

bool nvs_read_sequence(const IOHC::address addr, uint16_t *sequence) {
    if (!nvs_init()) return false;
    char key[7];
    sprintf(key, "%02x%02x%02x", addr[0], addr[1], addr[2]);
    if (!prefs.isKey(key)) return false;
    *sequence = prefs.getUShort(key, *sequence);
    return true;
}

void nvs_write_sequence(const IOHC::address addr, uint16_t sequence) {
    if (!nvs_init()) return;
    char key[7];
    sprintf(key, "%02x%02x%02x", addr[0], addr[1], addr[2]);
    prefs.putUShort(key, sequence);
}

bool nvs_read_string(const char *key, std::string &value) {
    if (!nvs_init()) return false;
    if (!prefs.isKey(key)) return false;
    value = std::string(prefs.getString(key, "").c_str());
    return true;
}

void nvs_write_string(const char *key, const std::string &value) {
    if (!nvs_init()) return;
    prefs.putString(key, value.c_str());
}
