#pragma once
#include <nvs_flash.h>
#include <nvs.h>
#include <string>

#define NVS_NAMESPACE              "miopenio"
#define NVS_KEY_DISPLAY_ENABLED    "disp_en"
#define NVS_KEY_DISPLAY_DIM_AFTER  "disp_dim_s"
#define NVS_KEY_DISPLAY_OFF_AFTER  "disp_off_s"

bool nvs_read_bool(const char* key, bool& value);
bool nvs_write_bool(const char* key, bool value);
bool nvs_read_int(const char* key, int& value);
bool nvs_write_int(const char* key, int value);
