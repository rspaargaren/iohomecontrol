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
#include <nvs_helpers.h>
#include <atomic>
#include <chrono>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
DisplayBuffer displayBuffer;
SemaphoreHandle_t displayBufferMutex = xSemaphoreCreateMutex();
TaskHandle_t displayTaskHandle = nullptr;
std::chrono::time_point<std::chrono::system_clock> startTime;
std::atomic<int64_t> lastDataTime = 0;
std::atomic<bool> displayEnabled = true;

// Dim/off timeouts (seconds). 0 = disabled.
std::atomic<int> displayDimAfter  = 30;
std::atomic<int> displayOffAfter  = 120;

void displayTask(void *);

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
};

const uint8_t PROGMEM wifiIcons[4][7] = {
    { B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B11011011 },
    { B00000000, B00000000, B00000000, B00000000, B11000000, B11000000, B11011011 },
    { B00000000, B00000000, B00011000, B00011000, B11011000, B11011000, B11011011 },
    { B00000011, B00000011, B00011011, B00011011, B11011011, B11011011, B11011011 },
};

const uint8_t PROGMEM mqttIcons[3][2*7] = {
    { B00111000, B11100000, B01100000, B00110000, B11000000, B00011000, B01100000, B00110000, B00111000, B11100000 },
    { B00111000, B11100000, B01100000, B00110000, B11001111, B10011000, B01100000, B00110000, B00111000, B11100000 },
    { B00011100, B00111000, B01100011, B00001100, B11000000, B00001100, B11000011, B00011000, B01110000, B11100000 },
};

int mqttStatusToIconIndex() {
    switch (mqttStatus) {
    case ConnState::Connecting:    return 0;
    case ConnState::Connected:     return 1;
    case ConnState::Disconnected:  return 2;
    }
    return 2;
}

const bool fast = true;
const bool slow = false;
std::atomic<bool> timerIsFast = false;

static void notifyDisplayTask() {
    if (displayTaskHandle != nullptr) xTaskNotifyGive(displayTaskHandle);
}

void setTimerSpeed(bool needsFast) {
    if (needsFast != timerIsFast) {
        timerIsFast = needsFast;
        notifyDisplayTask();
    }
}

// ── dim / off getters & setters ─────────────────────────────────────────────
int getDisplayDimAfter()  { return displayDimAfter; }
int getDisplayOffAfter()  { return displayOffAfter; }

void setDisplayDimAfter(int seconds) {
    if (seconds == displayDimAfter) return;
    displayDimAfter = seconds;
    nvs_write_int(NVS_KEY_DISPLAY_DIM_AFTER, seconds);
    notifyDisplayTask();
}

void setDisplayOffAfter(int seconds) {
    if (seconds == displayOffAfter) return;
    displayOffAfter = seconds;
    nvs_write_int(NVS_KEY_DISPLAY_OFF_AFTER, seconds);
    notifyDisplayTask();
}

bool initDisplay() {
    bool enabled = true;
    if (nvs_read_bool(NVS_KEY_DISPLAY_ENABLED, enabled)) displayEnabled = enabled;

    int v;
    if (nvs_read_int(NVS_KEY_DISPLAY_DIM_AFTER, v)) displayDimAfter = v;
    if (nvs_read_int(NVS_KEY_DISPLAY_OFF_AFTER, v)) displayOffAfter = v;

    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) return false;

    if (xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, nullptr, 1,
                                &displayTaskHandle, tskNO_AFFINITY) != pdPASS) {
        Serial.println("Failed to create display task");
        return false;
    }

    startTime    = std::chrono::system_clock::now();
    lastDataTime = esp_timer_get_time();
    xTaskNotifyGive(displayTaskHandle);
    return true;
}

int getSecondsSince(std::chrono::time_point<std::chrono::system_clock> &time) {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - time).count();
}
int getSecondsSinceStart()  { return getSecondsSince(startTime); }
int getSecondsSinceNoData() { return (esp_timer_get_time() - lastDataTime) / 1000000LL; }

const char* getRemoteName(const uint8_t *remote, const char *name) {
    if (name) return name;
    const auto *entry = IOHC::iohcRemoteMap::getInstance()->find(remote);
    if (entry) return entry->name.c_str();
    return bytesToHexString(remote, 3).c_str();
}

void display1WAction(const uint8_t *remote, const char *action, const char *dir, const char *name) {
    displayCustomMessage(format("%s: %s", dir, getRemoteName(remote, name)).c_str(), action);
}
void display1WPosition(const uint8_t *remote, float position, const char *name) {
    displayCustomMessage(getRemoteName(remote, name), format("%d%%", static_cast<int>(position)).c_str());
}

// Reset inactiviteitstimer en herstel display (dim→bright, off→on)
static void wakeDisplay() {
    lastDataTime.store(esp_timer_get_time());
    notifyDisplayTask();
}

void displayCustomMessage(const char* message, const char* status) {
    if (!displayEnabled) return;

    xSemaphoreTake(displayBufferMutex, portMAX_DELAY);
    displayBuffer.addLine(message, status ? status : "");
    xSemaphoreGive(displayBufferMutex);

    setTimerSpeed(fast);
    wakeDisplay();
}

void clearDisplayMessages() {
    xSemaphoreTake(displayBufferMutex, portMAX_DELAY);
    displayBuffer.clear();
    xSemaphoreGive(displayBufferMutex);

    lastDataTime.store(esp_timer_get_time());
    setTimerSpeed(slow);
}

void updateDisplayStatus() {
    if (!displayEnabled) return;
    setTimerSpeed(fast);
    wakeDisplay();
}

bool isDisplayEnabled() { return displayEnabled; }

void setDisplayEnabled(bool enabled) {
    if (enabled == displayEnabled) return;
    displayEnabled = enabled;
    nvs_write_bool(NVS_KEY_DISPLAY_ENABLED, enabled);
    if (!enabled) {
        xSemaphoreTake(displayBufferMutex, portMAX_DELAY);
        displayBuffer.clear();
        xSemaphoreGive(displayBufferMutex);
        lastDataTime = esp_timer_get_time();
        setTimerSpeed(slow);
    } else {
        lastDataTime = esp_timer_get_time();
        setTimerSpeed(fast);
    }
    notifyDisplayTask();
}

void drawLogo(int x, int y) {
    display.drawBitmap(x+1, y+1, miopenioLogo, 16, 10, SSD1306_WHITE);
    display.setCursor(x+20, y+4);
    display.print("MiOpen.IO");
}

void drawHeader() {
    drawLogo(0, 0);
#if defined(MQTT)
    const auto mqttIcon = mqttIcons[mqttStatusToIconIndex()];
    display.drawBitmap(127-8-1-16, 5, mqttIcon, 16, 5, SSD1306_WHITE);
#endif
    const auto wifiIcon = wifiIcons[min(wifiStatus.signalStrengthPercent.load(), 99) / 25];
    display.drawBitmap(127-8, 3, wifiIcon, 8, 7, SSD1306_WHITE);
}

void drawFooter() {
    if (wifiStatus.connectionStatus == ConnState::Connected) {
        display.setCursor(1, 56);
        if (getSecondsSinceStart() / 10 % 2 == 0) {
            display.println("http://miopenio.local");
        } else {
            display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        }
    }
}

bool drawContents() {
    const int width  = SCREEN_WIDTH / 6;
    const int height = (SCREEN_HEIGHT - 20 - 8) / 8;
    xSemaphoreTake(displayBufferMutex, portMAX_DELAY);
    const auto lines = displayBuffer.getTextToDisplay(width, height);
    xSemaphoreGive(displayBufferMutex);
    for (auto &line : lines) display.println(line.c_str());
    return lines.size() > 0;
}

void drawData() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    drawHeader();
    display.setCursor(0, 20);
    const bool hasData = drawContents();
    if (!hasData) setTimerSpeed(slow);
    drawFooter();
}

void drawLogo() {
    const int x = 50.0 * std::rand() / RAND_MAX;
    const int y = 48.0 * std::rand() / RAND_MAX;
    drawLogo(x, y);
}

// ── 3-fase state machine ─────────────────────────────────────────────────────
// ACTIVE : volledig aan, normale helderheid
// DIM    : display.dim(true)  — hardware contrast verlaagd
// OFF    : SSD1306_DISPLAYOFF — pixels volledig uit

enum class OledState { ACTIVE, DIM, OFF };

void displayTask(void *) {
    OledState state = OledState::ACTIVE;

    while (true) {
        bool showData = timerIsFast;
        const TickType_t waitTicks = pdMS_TO_TICKS(showData
            ? MILLIS_BETWEEN_DISPLAY_UPDATE_FAST
            : MILLIS_BETWEEN_DISPLAY_UPDATE_SLOW);
        ulTaskNotifyTake(pdTRUE, waitTicks);

        // ── display volledig uitgeschakeld via toggle ────────────────────────
        if (!displayEnabled) {
            if (state != OledState::OFF) {
                display.clearDisplay();
                display.display();
                display.ssd1306_command(SSD1306_DISPLAYOFF);
                state = OledState::OFF;
            }
            continue;
        }

        const int idle     = getSecondsSinceNoData();
        const int dimAfter = displayDimAfter.load();
        const int offAfter = displayOffAfter.load();

        // ── bepaal gewenste staat op basis van inactiviteit ──────────────────
        OledState desired;
        if (offAfter > 0 && idle >= offAfter) {
            desired = OledState::OFF;
        } else if (dimAfter > 0 && idle >= dimAfter) {
            desired = OledState::DIM;
        } else {
            desired = OledState::ACTIVE;
        }

        // ── overgang toepassen ───────────────────────────────────────────────
        if (desired == OledState::OFF && state != OledState::OFF) {
            display.clearDisplay();
            display.display();
            display.ssd1306_command(SSD1306_DISPLAYOFF);
            state = OledState::OFF;
            continue;
        }

        if (desired != OledState::OFF && state == OledState::OFF) {
            display.ssd1306_command(SSD1306_DISPLAYON);
            display.dim(false);
            state = OledState::ACTIVE;
        }

        if (desired == OledState::DIM && state == OledState::ACTIVE) {
            display.dim(true);
            state = OledState::DIM;
        } else if (desired == OledState::ACTIVE && state == OledState::DIM) {
            display.dim(false);
            state = OledState::ACTIVE;
        }

        // ── teken ────────────────────────────────────────────────────────────
        display.clearDisplay();
        if (showData || idle < SECONDS_BEFORE_SCREENSAVER) {
            drawData();
        } else {
            drawLogo();
        }
        display.display();
    }
}

#endif
