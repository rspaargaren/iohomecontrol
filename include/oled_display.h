#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <IPAddress.h>
#include <stdint.h>

#define OLED_ADDRESS 0x3c
#define OLED_SDA     4
#define OLED_SCL     15
#define OLED_RST     16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern Adafruit_SSD1306 display;

bool initDisplay();
void displayIpAddress(IPAddress ip);
void display1WAction(const uint8_t *remote, const char *action, const char *dir, const char *name = nullptr);
void updateDisplayStatus();

#endif // OLED_DISPLAY_H
