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
#define OLED_RST     DISPLAY_OLED_RST_PIN
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern Adafruit_SSD1306 display;

bool initDisplay();
void display1WAction(const uint8_t *remote, const char *action, const char *dir, const char *name = nullptr);
void display1WPosition(const uint8_t *remote, float position, const char *name = nullptr);
void displayCustomMessage(const char* message, const char* status = nullptr);
void clearDisplayMessages();
void updateDisplayStatus();
bool isDisplayEnabled();
void setDisplayEnabled(bool enabled);
#else
inline bool initDisplay() { return true; }
inline void display1WAction(const uint8_t *, const char *, const char *, const char * = nullptr) {}
inline void display1WPosition(const uint8_t *, float, const char * = nullptr) {}
inline void updateDisplayStatus() {}
inline void displayCustomMessage(const char*, const char* = nullptr) {}
inline void clearDisplayMessages() {}
inline bool isDisplayEnabled() { return false; }
inline void setDisplayEnabled(bool) {}
#endif


template<typename ... Args>
std::string format(const std::string &format, Args ... args)
{
    char buf[250];
    snprintf(buf, 250, format.c_str(), args ...);
    return std::string(buf);
}


#endif // OLED_DISPLAY_H
