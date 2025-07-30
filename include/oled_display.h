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

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <board-config.h>
#include <user_config.h>

#if defined(SSD1306_DISPLAY)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <IPAddress.h>
#include <stdint.h>

#define OLED_ADDRESS 0x3c
#define OLED_SDA     I2C_SDA_PIN
#define OLED_SCL     I2C_SCL_PIN
#define OLED_RST     16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern Adafruit_SSD1306 display;

bool initDisplay();
void displayIpAddress(IPAddress ip);
void display1WAction(const uint8_t *remote, const char *action, const char *dir, const char *name = nullptr);
void updateDisplayStatus();
#else
inline bool initDisplay() { return true; }
inline void displayIpAddress(IPAddress) {}
inline void display1WAction(const uint8_t *, const char *, const char *, const char * = nullptr) {}
inline void updateDisplayStatus() {}
#endif

#endif //OLED_DISPLAY_H