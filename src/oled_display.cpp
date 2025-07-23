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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

bool initDisplay() {
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        return false;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    String ipStr = "INIT DISPLAY";
    display.println(ipStr);
    display.display();
    return true;
}

void displayIpAddress(IPAddress ip) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    String ipStr = "IP: " + ip.toString();
    display.println(ipStr);
    display.display();
}

void display1WAction(const uint8_t *remote, const char *action, const char *dir, const char *name) {
    std::string id = bytesToHexString(remote, 3);
    const auto *entry = IOHC::iohcRemoteMap::getInstance()->find(remote);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(dir);
    display.println(":");
    if (name) {
        display.println(name);
    } else if (entry) {
        display.println(entry->name.c_str());
    } else {
        display.print("ID: ");
        display.println(id.c_str());
    }
    display.print("Action: ");
    display.println(action);
    display.display();
}

void display1WPosition(const uint8_t *remote, float position, const char *name) {
    std::string id = bytesToHexString(remote, 3);
    const auto *entry = IOHC::iohcRemoteMap::getInstance()->find(remote);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Clear only the area used for the position line so existing text remains
    int y = 30;  // Position feedback shown below action information
    display.fillRect(0, y, SCREEN_WIDTH, 10, SSD1306_BLACK);
    display.setCursor(0, y);

    if (name) {
        display.print(name);
        display.print(" ");
    } else if (entry) {
        display.print(entry->name.c_str());
        display.print(" ");
    } else {
        display.print("ID: ");
        display.print(id.c_str());
        display.print(" ");
    }

    display.print("Pos: ");
    display.print(static_cast<int>(position));
    display.println("%");
    display.display();
}

void updateDisplayStatus() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("WiFi: ");
    switch (wifiStatus) {
        case ConnState::Connected:
            display.println("connected");
            break;
        case ConnState::Connecting:
            display.println("connecting");
            break;
        default:
            display.println("disconnected");
            break;
    }
    display.setCursor(0, 10);
    display.print("IP: ");
    if (wifiStatus == ConnState::Connected) {
        display.println(WiFi.localIP());
    } else {
        display.println("-");
    }
    display.setCursor(0, 20);
    display.print("MQTT: ");
    switch (mqttStatus) {
        case ConnState::Connected:
            display.println("connected");
            break;
        case ConnState::Connecting:
            display.println("connecting");
            break;
        default:
            display.println("disconnected");
            break;
    }
    display.display();
}

#endif
