/*
   Copyright (c) 2024. CRIDP https://github.com/cridp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <user_config.h>
#if defined(SSD1306_DISPLAY)
#include <oled_display.h>
#include <iohcCryptoHelpers.h>
#include <iohcRemoteMap.h>
#include <interact.h>
#include <wifi_helper.h>
#include <WiFi.h>
#include <display_helpers.h>
#include <chrono>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
DisplayBuffer displayBuffer;
TimerHandle_t displayUpdateTimer;
std::chrono::time_point<std::chrono::system_clock> startTime;
std::chrono::time_point<std::chrono::system_clock> timeSinceNoData;
void handleTimerTick(TimerHandle_t);

const int MILLIS_BETWEEN_DISPLAY_UPDATE_SLOW = 5000;
const int MILLIS_BETWEEN_DISPLAY_UPDATE_FAST = 100;
const int SECONDS_BEFORE_SCREENSAVER = 60;

const uint8_t PROGMEM miopenioLogo[] =
{
    B00000010, B00000000,
    B00001101, B10000000,
    B00110000, B01100000,
    B11000000, B00011000,
    B01001011, B10010000,
    B01001010, B10010000,
    B01001010, B10010000,
    B01001011, B10010000,
    B01000000, B00010000,
    B01111111, B11110000,
}; // House with IO in it

const uint8_t PROGMEM wifiIcons[4][7] = {
    {
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B11011011,
    }, // 3 empty bars
    {
        B00000000,
        B00000000,
        B00000000,
        B00000000,
        B11000000,
        B11000000,
        B11011011,
    }, // first bar filled
    {
        B00000000,
        B00000000,
        B00011000,
        B00011000,
        B11011000,
        B11011000,
        B11011011,
    }, // first two filled
    {
        B00000011,
        B00000011,
        B00011011,
        B00011011,
        B11011011,
        B11011011,
        B11011011,
    }, // all three filled
};

const uint8_t PROGMEM mqttIcons[3][2*7] = {
     {
        B00111000, B11100000,
        B01100000, B00110000,
        B11000000, B00011000,
        B01100000, B00110000,
        B00111000, B11100000,
    }, // empty chain ends
    {
        B00111000, B11100000,
        B01100000, B00110000,
        B11001111, B10011000,
        B01100000, B00110000,
        B00111000, B11100000,
    }, // filled chain ends
    {
        B00011100, B00111000,
        B01100011, B00001100,
        B11000000, B00001100,
        B11000011, B00011000,
        B01110000, B11100000,
    }, // broken chain ends
};

int mqttStatusToIconIndex() {
    switch (mqttStatus) {
    case ConnState::Connecting:
        return 0;
    case ConnState::Connected:
        return 1;
    case ConnState::Disconnected:
        return 2;
    };
    return 2;
}

const bool fast = true;
const bool slow = false;
bool timerIsFast = false;
void setTimerSpeed(bool needsFast) {
    if (needsFast != timerIsFast) {
        xTimerChangePeriod(displayUpdateTimer, pdMS_TO_TICKS(needsFast ? MILLIS_BETWEEN_DISPLAY_UPDATE_FAST : MILLIS_BETWEEN_DISPLAY_UPDATE_SLOW), 0);

        timerIsFast = needsFast;
    }
}

bool initDisplay() {
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        return false;
    }

    startTime = std::chrono::system_clock::now();
    timeSinceNoData = startTime;
    displayUpdateTimer = xTimerCreate(
        "displayTimer",
        pdMS_TO_TICKS(MILLIS_BETWEEN_DISPLAY_UPDATE_FAST),
        pdTRUE,
        nullptr,
        handleTimerTick
    );
    if (displayUpdateTimer) {
        xTimerStart(displayUpdateTimer, 0);
    } else {
        Serial.println("Failed to create display update timer");
    }

    return true;
}

int getSecondsSince(std::chrono::time_point<std::chrono::system_clock> &time) {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - time).count();
}

int getSecondsSinceStart() {
    return getSecondsSince(startTime);
}

int getSecondsSinceNoData() {
    return getSecondsSince(timeSinceNoData);
}

template<typename ... Args>
std::string format(const std::string &format, Args ... args)
{
    char buf[250];
    snprintf(buf, 250, format.c_str(), args ...);
    return std::string(buf);
}

const char* getRemoteName(const uint8_t *remote, const char *name) {
    if (name) return name;
    
    const auto *entry = IOHC::iohcRemoteMap::getInstance()->find(remote);
    if (entry) return entry->name.c_str();

    return bytesToHexString(remote, 3).c_str();
}

void display1WAction(const uint8_t *remote, const char *action, const char *dir, const char *name) {
    displayBuffer.addLine(format("%s: %s", dir, getRemoteName(remote, name)), action);

    setTimerSpeed(fast);
}

void display1WPosition(const uint8_t *remote, float position, const char *name) {
    displayBuffer.addLine(getRemoteName(remote, name), format("%d%%", static_cast<int>(position)));

    setTimerSpeed(fast);
}

void updateDisplayStatus() {
    setTimerSpeed(fast);
}

void drawLogo(int x, int y) {
    // miopenio logo is 16x10
    display.drawBitmap(x+1, y+1, miopenioLogo, 16, 10, SSD1306_WHITE);
    display.setCursor(x+20, y+4);
    display.print("MiOpen.IO");
}

void drawHeader() {
    drawLogo(0, 0);

#if defined(MQTT)
    // mqtt icon is 16x5 (including 3 pixels space, so adding 1 extra for a reasonable space)
    const auto mqttIcon = mqttIcons[mqttStatusToIconIndex()];
    display.drawBitmap(127-8-1-16, 5, mqttIcon, 16, 5, SSD1306_WHITE);
#endif // MQTT

    // wifi icon is 8x7
    const auto wifiIcon = wifiIcons[min(wifiStatus.signalStrengthPercent, 99) / 25];
    display.drawBitmap(127-8, 3, wifiIcon, 8, 7, SSD1306_WHITE);
}

void drawFooter() {
    if (wifiStatus.connectionStatus == ConnState::Connected) {
        display.setCursor(1, 56);
        // every 10 seconds alternate between url and ip
        if (getSecondsSinceStart() / 10 % 2 == 0) {
            display.println("http://miopenio.local");
        } else {
            display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        }
    }
}

bool drawContents() {
    const int width = SCREEN_WIDTH / 6; // char width is 5 + 1 pixel space
    const int height = (SCREEN_HEIGHT - 20 - 8) / 8; // char height is 7 + 1 pixel space and 20 pixels (12 pixels + an empty line) for the header + 8 for the footer

    const auto lines = displayBuffer.getTextToDisplay(width, height);
    for(auto &line : lines) {
        display.println(line.c_str());
    }
    return lines.size() > 0;
}

void handleTimerTick(TimerHandle_t) {
    display.clearDisplay();

    timeSinceNoData = timerIsFast ? std::chrono::system_clock::now() : timeSinceNoData;
    const auto secondsSinceNoData = getSecondsSinceNoData();

    if (timerIsFast || secondsSinceNoData < SECONDS_BEFORE_SCREENSAVER) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        drawHeader();

        display.setCursor(0, 20);
        const bool hasData = drawContents();
        if (!hasData) {
            setTimerSpeed(slow);
        }

        drawFooter();
    } else {
        // draw logo at random position to avoid burn-in
        const int x = 50.0 * std::rand() / RAND_MAX; // number between 0 and 50
        const int y = 48.0 * std::rand() / RAND_MAX; // number between 0 and 48
        drawLogo(x, y);
    }

    display.display();
}

#endif
