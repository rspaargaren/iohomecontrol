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

#include <board-config.h>
#include <user_config.h>

#include <crypto2Wutils.h>
#include <iohcCryptoHelpers.h>
#include <iohcRadio.h>

#include <iohcSystemTable.h>
#include <fileSystemHelpers.h>
#include <ArduinoJson.h>
#include <iohcRemote1W.h>
#include <iohcCozyDevice2W.h>
#include <iohcOtherDevice2W.h>
#include <interact.h>

#include <web_server_handler.h>
#include "LittleFS.h"
#include <WiFi.h> // Assuming WiFi is used and initialized elsewhere or will be here.


#include <oled_display.h>


extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

void txUserBuffer(Tokens *cmd);
void testKey();
void scanDump();
bool publishMsg(IOHC::iohcPacket *iohc);
bool msgRcvd(IOHC::iohcPacket *iohc);
bool msgArchive(IOHC::iohcPacket *iohc);


uint8_t keyCap[16] = {};
//uint8_t source_originator[3] = {0};

IOHC::iohcRadio *radioInstance;
IOHC::iohcPacket *radioPackets[IOHC_INBOUND_MAX_PACKETS];

std::vector<IOHC::iohcPacket *> packets2send{};
std::vector<IOHC::iohcPacket *> packets2send_tmp{};

uint8_t nextPacket = 0;

IOHC::iohcSystemTable *sysTable;
IOHC::iohcRemote1W *remote1W;
IOHC::iohcCozyDevice2W *cozyDevice2W;
IOHC::iohcOtherDevice2W *otherDevice2W;

uint32_t frequencies[] = FREQS2SCAN;

using namespace IOHC;

void setup() {
    Serial.begin(115200);
    

    initDisplay();


    //Heltec.begin(true /*DisplayEnable*/, false /*LoRaEnable*/, true /*SerialEnable*/);
    //Heltec.display->clear();
    //Heltec.display->drawString(0, 0, "Booting...");
    //Heltec.display->display();

    pinMode(RX_LED, OUTPUT); // Blink this LED
    digitalWrite(RX_LED, 1);

    // Mount LittleFS filesystem
#if defined(ESP32)
    // LittleFS.begin(); // Original call, replaced by new init below
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        // Handle error appropriately, maybe by halting or indicating failure
        return; 
    }
    Serial.println("LittleFS mounted successfully");
#endif

    // --- WiFi Setup ---
    // Credentials are defined in include/user_config.h

    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    Serial.print("Connecting to WiFi...");
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) { // Retry for 15 seconds
        delay(500);
        Serial.print(".");
        retries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected to WiFi. IP Address: ");
        Serial.println(WiFi.localIP());

        displayIpAddress(WiFi.localIP());

        // --- End WiFi Setup ---

        // Call setupWebServer() only if WiFi connected
        setupWebServer();
    } else {
        Serial.println("Failed to connect to WiFi. Web server not started.");
    }

    radioInstance = IOHC::iohcRadio::getInstance();
    radioInstance->start(MAX_FREQS, frequencies, 0, msgRcvd, publishMsg); //msgArchive); //, msgRcvd);

    sysTable = IOHC::iohcSystemTable::getInstance();

    remote1W = IOHC::iohcRemote1W::getInstance();
    cozyDevice2W = IOHC::iohcCozyDevice2W::getInstance();
    otherDevice2W = IOHC::iohcOtherDevice2W::getInstance();

    //   AES_init_ctx(&ctx, transfert_key); // PreInit AES for cozy (1W use original version) TODO

    Cmd::createCommands();

//    esp_timer_dump(stdout);

    printf("Startup completed. type help to see what you can do!\n");
    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
}

/**
 * The function `forgePacket` modifies a given `iohcPacket` structure with specific values and
 * settings.
 * 
 * @param packet The `packet` parameter is a pointer to an `iohcPacket` struct.
 * @param toSend The `vector` parameter in the `forgePacket` function is a `std::vector<uint8_t>` type,
 * which is a standard C++ container that stores a sequence of elements of type `uint8_t` (unsigned
 * 8-bit integer). In this function, the size of the `
 */
/**
* @brief Creates a iohcPacket with the given data to send. 
* @param packet * The packet you want to forge
* @param toSend The data that will be added to the packet
*/
void IRAM_ATTR forgePacket(iohcPacket* packet, const std::vector<uint8_t> &toSend) {
    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
    IOHC::packetStamp = esp_timer_get_time();

    // Common Flags
    // 8 if protocol version is 0 else 10
    packet->payload.packet.header.CtrlByte1.asStruct.MsgLen = sizeof(_header) - 1;
    packet->payload.packet.header.CtrlByte1.asStruct.Protocol = 0;
    packet->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
    packet->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
    packet->payload.packet.header.CtrlByte1.asByte += toSend.size();
    memcpy(packet->payload.buffer + 9, toSend.data(), toSend.size());
    packet->buffer_length = toSend.size() + 9;

    packet->payload.packet.header.CtrlByte2.asByte = 0;

    packet->frequency = CHANNEL2;
    packet->repeatTime = 25;
    packet->repeat = 0;
    packet->lock = false;
}

bool msgRcvd(IOHC::iohcPacket *iohc) {
    JsonDocument doc;
    doc["type"] = "Unk";
    switch (iohc->payload.packet.header.cmd) {
        case iohcDevice::RECEIVED_DISCOVER_0x28: {
            printf("2W Pairing Asked\n");
            if (!Cmd::pairMode) break;

            // 0x0b OverKiz 0x0c Atlantic
            std::vector<uint8_t> toSend = {0xff, 0xc0, 0xba, 0x11, 0xad, 0x0b, 0xcc, 0x00, 0x00};

            packets2send.clear();
            auto* packet = new iohcPacket;
            forgePacket(packet, toSend);

            packet->payload.packet.header.cmd = IOHC::iohcDevice::SEND_DISCOVER_ANSWER_0x29;
 
            /* Swap */
            memcpy(packet->payload.packet.header.source, cozyDevice2W->gateway, 3);
            memcpy(packet->payload.packet.header.target, iohc->payload.packet.header.source, 3);

            packet->delayed = 250;
            packet->repeat = 0;

            packets2send.push_back(packet);
            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
            radioInstance->send(packets2send);
            break;
        }
        case iohcDevice::RECEIVED_DISCOVER_ANSWER_0x29: {
            printf("2W Device want to be paired\n");
            if (!Cmd::pairMode) break;

            std::vector<uint8_t> deviceAsked;
            deviceAsked.assign(iohc->payload.buffer + 9, iohc->payload.buffer + 18);
            for (unsigned char i: deviceAsked) {
                printf("%02X ", i);
            }
            printf("\n");

            // printf("Sending 0x38 \n");
            printf("Sending SEND_DISCOVER_ACTUATOR_0x2C \n");

            // std::vector<uint8_t> toSend = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; // 38
            std::vector<uint8_t> toSend = {}; // SEND_DISCOVER_ACTUATOR_0x2C

            packets2send.clear();
            packets2send.push_back(new IOHC::iohcPacket);
            forgePacket(packets2send.back(), toSend);

            // packets2send.back()->payload.packet.header.cmd = 0x38;
            packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_ACTUATOR_0x2C;
            // cozyDevice2W->memorizeSend.memorizedData = toSend;
            // cozyDevice2W->memorizeSend.memorizedCmd = SEND_DISCOVER_ACTUATOR_0x2C;

            /* Swap */
            memcpy(packets2send.back()->payload.packet.header.source, iohc->payload.packet.header.target, 3);
            memcpy(packets2send.back()->payload.packet.header.target, iohc->payload.packet.header.source, 3);

            packets2send.back()->repeat = 1;

            radioInstance->send(packets2send);
            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
            break;
        }
        case iohcDevice::RECEIVED_DISCOVER_REMOTE_ANSWER_0x2B: {
            sysTable->addObject(iohc->payload.packet.header.source, iohc->payload.packet.msg.p0x2b.backbone,
                                iohc->payload.packet.msg.p0x2b.actuator, iohc->payload.packet.msg.p0x2b.manufacturer,
                                iohc->payload.packet.msg.p0x2b.info);
            break;
        }
        case iohcDevice::RECEIVED_DISCOVER_ACTUATOR_0x2C: {
            printf("2W Actuator Ack Asked\n");
            if (!Cmd::pairMode) break;

            std::vector<uint8_t> toSend = {};

            packets2send.clear();
            packets2send.push_back(new IOHC::iohcPacket);
            forgePacket(packets2send.back(), toSend);

            packets2send.back()->payload.packet.header.cmd = IOHC::iohcDevice::SEND_DISCOVER_ACTUATOR_ACK_0x2D;

            /* Swap */
            memcpy(packets2send.back()->payload.packet.header.source, iohc->payload.packet.header.target, 3);
            memcpy(packets2send.back()->payload.packet.header.target, iohc->payload.packet.header.source, 3);

            packets2send.back()->delayed = 250;
            packets2send.back()->repeat = 0;

            radioInstance->send(packets2send);
            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
            break;
        }
        case iohcDevice::RECEIVED_LAUNCH_KEY_TRANSFERT_0x38: {
            printf("2W Key Transfert Asked after Command %2.2X\n", iohc->payload.packet.header.cmd);
            if (!Cmd::pairMode) break;

            std::vector<uint8_t> key_transfert;
            key_transfert.assign(iohc->payload.buffer + 9, iohc->payload.buffer + 15);

            for (unsigned char i: key_transfert) {
                printf("%02X ", i);
            }
            printf("\n");
            std::vector<uint8_t> data = {IOHC::iohcDevice::SEND_ASK_CHALLENGE_0x31}; //0x38
            unsigned char initial_value[16];
            constructInitialValue(data, initial_value, data.size(), key_transfert, nullptr);
            Serial.printf("2) Initial value used for key encryption: ");
            for (unsigned char i: initial_value) {
                printf("%02X ", i);
            }
            printf("\n");

            AES_init_ctx(&ctx, transfert_key);
            uint8_t encrypted_key[16];
            AES_ECB_encrypt(&ctx, initial_value);
            //  XORing transfert_key
            for (int i = 0; i < 16; i++) {
                encrypted_key[i] = initial_value[i] ^ transfert_key[i];
            }
            printf("2) Encrypted 2-way key to be sent with SEND_KEY_TRANSFERT_0x32: ");
            for (unsigned char i: encrypted_key) {
                printf("%02X ", i);
            }
            printf("\n");
            std::vector<uint8_t> toSend;
            toSend.assign(encrypted_key, encrypted_key + 16);

            packets2send.clear();
            packets2send.push_back(new IOHC::iohcPacket);
            forgePacket(packets2send.back(), toSend);

            packets2send.back()->payload.packet.header.cmd = IOHC::iohcDevice::SEND_KEY_TRANSFERT_0x32;
            cozyDevice2W->memorizeSend.memorizedCmd = IOHC::iohcDevice::SEND_KEY_TRANSFERT_0x32;

            /* Swap */
            memcpy(packets2send.back()->payload.packet.header.source, iohc->payload.packet.header.target, 3);
            memcpy(packets2send.back()->payload.packet.header.target, iohc->payload.packet.header.source, 3);

            packets2send.back()->repeat = 0;

            radioInstance->send(packets2send);
            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
            break;
        }
        case iohcDevice::RECEIVED_WRITE_PRIVATE_0x20:  {
            cozyDevice2W->memorizeSend.memorizedCmd = iohc->payload.packet.header.cmd;
            IOHC::lastSendCmd = iohc->payload.packet.header.cmd;
            break;
        }
        case iohcDevice::RECEIVED_PRIVATE_ACK_0x21: {
            // Answer of 0x20, publish the confirmed command
            // doc["type"] = "Cozy";
            // doc["from"] = bytesToHexString(iohc->payload.packet.header.target, 3);
            // doc["to"] = bytesToHexString(iohc->payload.packet.header.source, 3);
            // doc["cmd"] = to_hex_str(iohc->payload.packet.header.cmd).c_str();
            // doc["_data"] = bytesToHexString(iohc->payload.buffer + 9, iohc->buffer_length - 9);
            // std::string message;
            // size_t messageSize = serializeJson(doc, message);
            // mqttClient.publish("iown/Frame", 0, false, message.c_str(), messageSize);
            break;
        }
        case iohcDevice::RECEIVED_CHALLENGE_REQUEST_0x3C: {
            // Answer only to our gateway, not to others devices
            if (cozyDevice2W->isFake(iohc->payload.packet.header.source, iohc->payload.packet.header.target)) {
                // (true) { //

                doc["type"] = "Gateway";
//                if (!cozyDevice2W->isFake(iohc->payload.packet.header.source, iohc->payload.packet.header.target)) {
                    //                        AES_init_ctx(&ctx, setgo); // PreInit AES for other2W (1W use original version) TODO
//                }
                //                    else
                AES_init_ctx(&ctx, transfert_key);

                // IVdata is the challenge with commandId put on start
                std::vector<uint8_t> challengeAsked;
                //                    challengeAsked.assign(iohc->payload.packet.msg.variableData.data, iohc->payload.packet.msg.variableData.data + iohc->payload.packet.msg.variableData.size);
                challengeAsked.assign(iohc->payload.buffer + 9, iohc->payload.buffer + 15);
                printf("Challenge asked after LastSend Command %2.2X\n", IOHC::lastSendCmd);
                printf("Challenge asked after Memorized Command %2.2X\n", cozyDevice2W->memorizeSend.memorizedCmd);

                if (Cmd::scanMode) {
                    otherDevice2W->mapValid[IOHC::lastSendCmd] = iohcDevice::RECEIVED_CHALLENGE_REQUEST_0x3C;
                    break;
                }

                std::vector<uint8_t> IVdata = cozyDevice2W->memorizeSend.memorizedData;
                IVdata.insert(IVdata.begin(), cozyDevice2W->memorizeSend.memorizedCmd);

                packets2send.clear();
                packets2send.push_back(new IOHC::iohcPacket);

                packets2send.back()->payload.packet.header.cmd = IOHC::iohcDevice::SEND_CHALLENGE_ANSWER_0x3D;

                unsigned char initial_value[16];
                constructInitialValue(IVdata, initial_value, IVdata.size(), challengeAsked, nullptr);
                AES_ECB_encrypt(&ctx, initial_value);
                uint8_t dataLen = 6;

                if (cozyDevice2W->memorizeSend.memorizedCmd == IOHC::iohcDevice::RECEIVED_ASK_CHALLENGE_0x31) {
                    packets2send.back()->payload.packet.header.cmd = IOHC::iohcDevice::SEND_KEY_TRANSFERT_0x32;
                    dataLen = 16;
                    IVdata = {IOHC::iohcDevice::RECEIVED_ASK_CHALLENGE_0x31};
                    constructInitialValue(IVdata, initial_value, 1, challengeAsked, nullptr);
                    AES_ECB_encrypt(&ctx, initial_value);
                    for (int i = 0; i < dataLen; i++)
                        initial_value[i] = initial_value[i] ^ transfert_key[i];
                    cozyDevice2W->memorizeSend.memorizedCmd = IOHC::iohcDevice::SEND_KEY_TRANSFERT_0x32;
                    cozyDevice2W->memorizeSend.memorizedData.assign(initial_value, initial_value + 16);
                }

                std::vector<uint8_t> toSend;
                toSend.assign(initial_value, initial_value + dataLen);
                forgePacket(packets2send.back(), toSend);

                /* Swap */
                memcpy(packets2send.back()->payload.packet.header.source, iohc->payload.packet.header.target, 3);
                memcpy(packets2send.back()->payload.packet.header.target, iohc->payload.packet.header.source, 3);

                packets2send.back()->repeatTime = 6;
                packets2send.back()->repeat = 1;

                radioInstance->send(packets2send);

                // Serial.print("IV used for key encryption: ");
                // for (int i = 0; i < 16; i++)
                //     Serial.printf("%02X ", initial_value[i]);
                // Serial.println();
                printf("Challenge response %2.2X: ", packets2send[0]->payload.packet.header.cmd);
                for (int i = 0; i < dataLen; i++)
                    printf("%02X ", initial_value[i]);
                printf("\n");

                //                sysTable->addObject(iohc);
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
            }
            break;
        }
        case 0X00:
        case 0x01:
        case 0x03:
        case 0x19: {
            if (iohc->payload.packet.header.CtrlByte1.asStruct.Protocol == 1 && iohc->payload.packet.header.cmd == 0x00) {
                doc["type"] = "1W";
                uint16_t main = (iohc->payload.packet.msg.p0x00_14.main[0] << 8) | iohc->payload.packet.msg.p0x00_14.main[1];
                const char *action = "unknown";
                switch (main) {
                    case 0x0000: action = "OPEN"; break;
                    case 0xC800: action = "CLOSE"; break;
                    case 0xD200: action = "STOP"; break;
                    case 0xD803: action = "VENT"; break;
                    case 0x6400: action = "FORCE"; break;
                    default: break;
                }
                doc["action"] = action;
                display1WAction(iohc->payload.packet.header.source, action);
            } else {
                doc["type"] = "Other";
                otherDevice2W->memorizeOther2W.memorizedCmd = iohc->payload.packet.header.cmd;
                cozyDevice2W->memorizeSend.memorizedCmd = iohc->payload.packet.header.cmd;
            }
            break;
        }
        case iohcDevice::RECEIVED_GET_NAME_0x50: {
            if (cozyDevice2W->isFake(iohc->payload.packet.header.source, iohc->payload.packet.header.target)) {
            // MY_GATEWAY 4d595f47415445574159
            std::vector<uint8_t> toSend = {0x4d, 0x59, 0x5f, 0x47, 0x41, 0x54, 0x45, 0x57, 0x41, 0x59};
            toSend.resize(16);
            
            packets2send.clear();
            packets2send.push_back(new IOHC::iohcPacket);
            forgePacket(packets2send.back(), toSend);

            packets2send.back()->payload.packet.header.cmd = 0x51;

            /* Swap */
            memcpy(packets2send.back()->payload.packet.header.source, cozyDevice2W->gateway, 3);
            memcpy(packets2send.back()->payload.packet.header.target, iohc->payload.packet.header.source, 3);

            packets2send.back()->delayed = 50;
            packets2send.back()->repeat = 0;

            radioInstance->send(packets2send);
            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
            }
            break;
        }
        case 0x51: {
            std::vector<uint8_t> nameReceived;
            nameReceived.assign(iohc->payload.buffer + 9, iohc->payload.buffer + 25);
            //            std::string asciiName;

            for (char byte: nameReceived) {
                //    asciiName += std::toupper(byte);
                printf("%c", std::toupper(byte));
            }
            //            printf("%s\n", asciiName.c_str());
            printf("\n");
            break;
        }
        case 0x04:
        case 0x0D:
        case iohcDevice::RECEIVED_DISCOVER_ACTUATOR_ACK_0x2D:
        case 0x4B:
        case 0x55:
        case 0x57:
        case 0x59: {
            if (Cmd::scanMode) {
                otherDevice2W->memorizeOther2W = {};
                // printf(" Answer %X Cmd %X ", iohc->payload.packet.header.cmd, IOHC::lastSendCmd);
                otherDevice2W->mapValid[IOHC::lastSendCmd] = iohc->payload.packet.header.cmd;
            }
        break;
    }
        case iohcDevice::RECEIVED_STATUS_0xFE: {
            if (Cmd::scanMode) {
                otherDevice2W->memorizeOther2W = {};
                // printf(" Unknown %X Cmd %X ", iohc->payload.buffer[9], IOHC::lastSendCmd);
                otherDevice2W->mapValid[IOHC::lastSendCmd] = iohc->payload.buffer[9];
            }
            break;
        }
        case 0x30: {
            for (uint8_t idx = 0; idx < 16; idx++)
                keyCap[idx] = iohc->payload.packet.msg.p0x30.enc_key[idx];

            iohcCrypto::encrypt_1W_key((const uint8_t *) iohc->payload.packet.header.source, (uint8_t *) keyCap);
            printf("CLEAR KEY: ");
            for (unsigned char idx: keyCap)
                printf("%2.2X", idx);
            printf("\n");
            break;
        }
        case 0X2E: {
            printf("1W Learning mode\n");
            break;
        }
        case 0x39: {
            if (keyCap[0] == 0) break;
            uint8_t hmac[16];
            std::vector<uint8_t> frame(&iohc->payload.packet.header.cmd, &iohc->payload.packet.header.cmd + 2);
            // frame = {0x39, 0x00}; //
            iohcCrypto::create_1W_hmac(hmac, iohc->payload.packet.msg.p0x39.sequence, keyCap, frame);
            printf("MAC: ");
            for (uint8_t idx = 0; idx < 6; idx++)
                printf("%2.2X", hmac[idx]);
            printf("\n");
            break;
        }
        case iohcDevice::RECEIVED_CHALLENGE_ANSWER_0x3D:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0X05: break;
        default:
            printf("Received Unknown command %02X ", iohc->payload.packet.header.cmd);
            return false;
            break;
    }

    publishMsg(iohc);
    return true;
}

/**
 * The function creates a JSON message from an `iohcPacket` object and publishes it using
 * MQTT if enabled.
 * 
 * @param iohc The `iohc` parameter is a pointer to an object of type `IOHC::iohcPacket`. The function
 * `publishMsg` takes this pointer as input and processes the data within the `iohc` object to create a
 * JSON message and publish it using MQTT if the conditions are met.
 * 
 * @return The function `publishMsg` is returning `false`.
 */
bool publishMsg(IOHC::iohcPacket *iohc) {
    JsonDocument doc;

    doc["type"] = "Cozy";
    doc["from"] = bytesToHexString(iohc->payload.packet.header.target, 3);
    doc["to"] = bytesToHexString(iohc->payload.packet.header.source, 3);
    doc["cmd"] = to_hex_str(iohc->payload.packet.header.cmd).c_str();
    doc["_data"] = bytesToHexString(iohc->payload.buffer + 9, iohc->buffer_length - 9);

    if (iohc->payload.packet.header.CtrlByte1.asStruct.Protocol == 1 &&
        iohc->payload.packet.header.cmd == 0x00) {
        uint16_t main =
                (iohc->payload.packet.msg.p0x00_14.main[0] << 8) |
                iohc->payload.packet.msg.p0x00_14.main[1];
        const char *action = "unknown";
        switch (main) {
            case 0x0000: action = "open"; break;
            case 0xC800: action = "close"; break;
            case 0xD200: action = "stop"; break;
            case 0xD803: action = "vent"; break;
            case 0x6400: action = "force"; break;
            default: break;
        }
        doc["type"] = "1W";
        doc["action"] = action;
    }

    std::string message;
    size_t messageSize = serializeJson(doc, message);
#if defined(MQTT)
    mqttClient.publish("iown/Frame", 1, false, message.c_str(), messageSize);
    mqttClient.publish("homeassistant/sensor/iohc_frame/state", 0, false, message.c_str(), messageSize);
#endif
    return false;
}

/**
 * @deprecated
 * The function copies data from one `iohcPacket` object to another and stores it in an
 * array, returning true if successful and false if there are not enough buffers available.
 * 
 * @param iohc The `iohc` parameter in the `msgArchive` function is a pointer to an object of type
 * `IOHC::iohcPacket`. This object contains information such as buffer length, frequency, RSSI
 * (Received Signal Strength Indication), and payload data. The function `msgArchive` is
 * 
 * @return The function `msgArchive` returns a boolean value - `true` if the operation is successful
 * and `false` if there is a failure condition detected during the execution of the function.
 */
bool msgArchive(IOHC::iohcPacket *iohc) {
    radioPackets[nextPacket] = new IOHC::iohcPacket; 
    if (!radioPackets[nextPacket]) {
        Serial.printf("*** Malloc failed!\n");
        return false;
    }

    radioPackets[nextPacket]->buffer_length = iohc->buffer_length;
    radioPackets[nextPacket]->frequency = iohc->frequency;
    //    radioPackets[nextPacket]->stamp = iohc->stamp;
    radioPackets[nextPacket]->rssi = iohc->rssi;

    for (uint8_t i = 0; i < iohc->buffer_length; i++)
        radioPackets[nextPacket]->payload.buffer[i] = iohc->payload.buffer[i];

    nextPacket += 1;
    Serial.printf("-> %d\r", nextPacket);
    if (nextPacket >= IOHC_INBOUND_MAX_PACKETS) {
        nextPacket = IOHC_INBOUND_MAX_PACKETS - 1;
        Serial.printf("*** Not enough buffers available. Please erase current ones\n");
        return false;
    }

    return true;
}

/**
 * @deprecated
 * The function `txUserBuffer` sends a packet using a radio instance based on the input command and
 * frequency.
 * 
 * @param cmd The `cmd` parameter is a pointer to a `Tokens` object. It seems like the `Tokens` class
 * has a method `size()` that returns the size of the object, and an `at()` method that retrieves a
 * specific element at a given index. The function `txUserBuffer`
 * 
 * @return In the provided code snippet, the `txUserBuffer` function returns `void`, which means it
 * does not return any value. Instead, it performs certain operations and then exits the function
 * without returning any specific value.
 */
void txUserBuffer(Tokens *cmd) {
    if (cmd->size() < 2) {
        Serial.printf("No packet to be sent!\n");
        return;
    }
    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
    if (!packets2send[0])
        packets2send[0] = new IOHC::iohcPacket;

    if (cmd->size() == 3)
        packets2send[0]->frequency = frequencies[atoi(cmd->at(2).c_str()) - 1];
    else
        packets2send[0]->frequency = 0;

    packets2send[0]->buffer_length = hexStringToBytes(cmd->at(1), packets2send[0]->payload.buffer);
    packets2send[0]->repeatTime = 35;
    packets2send[0]->repeat = 1;
    packets2send[1] = nullptr;

    radioInstance->send(packets2send);
    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
}

void loop() {
    // loopWebServer(); // For ESPAsyncWebServer, this is typically not needed.
}
