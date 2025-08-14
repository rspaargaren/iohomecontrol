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

#include <iohcOtherDevice2W.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <iohcCryptoHelpers.h>
#include <string>
#include <iohcRadio.h>
#include <vector>
#include <iohcDevice.h>
#include <numeric>

namespace IOHC {
    iohcOtherDevice2W *iohcOtherDevice2W::_iohcOtherDevice2W = nullptr;

    iohcOtherDevice2W::iohcOtherDevice2W() = default;

    iohcOtherDevice2W *iohcOtherDevice2W::getInstance() {
        if (!_iohcOtherDevice2W) {
            _iohcOtherDevice2W = new iohcOtherDevice2W();
            _iohcOtherDevice2W->load();
            _iohcOtherDevice2W->initializeValid();
        }
        return _iohcOtherDevice2W;
    }

    address fake_gateway = {0xba, 0x11, 0xad};

    void iohcOtherDevice2W::forgePacket(iohcPacket *packet, const std::vector<uint8_t> &toSend, size_t typn = 0) {
        digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
        IOHC::relStamp = esp_timer_get_time();

        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen = sizeof(_header) - 1;

        packet->payload.packet.header.cmd = 0x2A; //0x28; //typn;
        // Common Flags
        packet->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
        packet->payload.packet.header.CtrlByte2.asByte = 0;
        packet->payload.packet.header.CtrlByte2.asStruct.Prio = 1;
        packet->payload.packet.header.CtrlByte2.asStruct.LPM = 0;

        // Broadcast Target
        u_int16_t bcast = typn;
        //            uint16_t bcast = /*(_type.at(typn)<<6)*/(typn)<<6 + 0b111111;
        packet->payload.packet.header.target[0] = 0x00;
        packet->payload.packet.header.target[1] = bcast >> 8;
        packet->payload.packet.header.target[2] = bcast & 0x00ff;

        packet->payload.packet.header.CtrlByte1.asByte += toSend.size();
        memcpy(packet->payload.buffer + 9, toSend.data(), toSend.size());
        packet->buffer_length = toSend.size() + 9;

        packet->frequency = CHANNEL2;
        packet->repeatTime = 50;
        packet->repeat = 0;
        packet->lock = false;
    }

    void iohcOtherDevice2W::cmd(Other2WButton cmd, Tokens *data) {
        if (!_radioInstance) {
            Serial.println("NO RADIO INSTANCE");
            _radioInstance = iohcRadio::getInstance();
        }
        packets2send.clear();

        // Emulates device button press
        switch (cmd) {
            case Other2WButton::discovery: {
                int bec = 0;

                for (int j = 0; j < 255; j++) {
                    packets2send.push_back(new iohcPacket);

                    std::string discovery = "d430477706ba11ad31"; //28"; //"2b578ebc37334d6e2f50a4dfa9";
                    std::vector<uint8_t> toSend = {};
                    packets2send.back()->buffer_length = hexStringToBytes(
                        discovery, packets2send.back()/*[j]*/->payload.buffer);
                    forgePacket(packets2send.back()/*[j]*//*->payload.buffer[4]*/, toSend, bec);
                    bec += 0x01;
                    packets2send.back()->repeatTime = 250; // Slow down discovery loop
                }

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::getName: {

                std::vector<uint8_t> toSend = {}; // {0x0C, 0x60, 0x01, 0xFF};

                // const char* dat = data->at(1).c_str();

                int value = std::stol(data->at(1).c_str(), nullptr, 16);
                address target;
                target[0] = static_cast<uint8_t>(value >> 16);
                target[1] = static_cast<uint8_t>(value >> 8);
                target[2] = static_cast<uint8_t>(value);
                // uint8_t target[3] = {0x08, 0x42, 0xE3};
                // toSend[3] = custom;

                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend, 0);
                packets2send.back()->payload.packet.header.cmd = SEND_GET_NAME_0x50;
                // memorizeSend.memorizedData = toSend;
                // memorizeSend.memorizedCmd = SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
//                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, fake_gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, target, 3);

//                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());
//                packets2send.back()->buffer_length = toSend.size() + 9;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                // packets2send.back()->delayed = 501;
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::custom: {
                std::vector<uint8_t> toSend = {0x01, 0x47, 0xc8, 0x00, 0x00, 0x00};
                //{0x03, 0x65, 0xd4, 0x00, 0x00, 0x00}; //{0x0C, 0x60, 0x01, 0xFF, 0xFF};
                //const char* dat = data->at(1).c_str();
                for (int acei = 0; acei < 256; acei++) {
                    //               int custom = std::stoi(data->at(1));
                    AceiUnion ACEI{};
                    ACEI.asByte = acei;
                    // Only other ACEI are valids, other give answer: 0xFE 0x58
                    if (!ACEI.asStruct.isvalid || ACEI.asStruct.service != 0) continue;

                    toSend[1] = acei; //custom;
                    // toSend[2] = acei;

                    address from = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //
                    address to = {0xda, 0x2e, 0xe6}; //
                    address to_1 = {0x05, 0x4e, 0x17}; //{0x31, 0x58, 0x24}; //

//                    packets2send.clear();
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = 0x00; //SEND_WRITE_PRIVATE_0x20;
                    memorizeOther2W.memorizedData = toSend;
                    memorizeOther2W.memorizedCmd = 0x00; //SEND_WRITE_PRIVATE_0x20;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, from/*gateway*/, 3);
                    memcpy(packets2send.back()->payload.packet.header.target, to_1, 3);

                    packets2send.back()->delayed = 250; // Give enough time for the answer
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::custom60: {
                std::vector<uint8_t> toSend = {0x0C, 0x60, 0x01, 0xFF};
                // Accepted command {0x0C, 0x61, 0x01, 0xFF, FF};
                const char *dat = data->at(1).c_str();

                //                for (int custom = 0; custom < 256; custom++) {
                int custom = std::stoi(data->at(1));

                toSend[3] = custom; //custom;

//                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;
                memorizeOther2W.memorizedData = toSend;
                memorizeOther2W.memorizedCmd = iohcDevice::SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                memcpy(packets2send.back()->payload.packet.header.source, gateway/*master_from*/, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_to/*slave_to*/, 3);

                packets2send.back()->delayed = 250;
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::discover28: {
                std::vector<uint8_t> toSend = {};

                //                uint8_t broadcast[3];
                address broadcast = {0x00, 0xFF, 0xFB}; //{0x02, 0x02, 0xFB}; //data->at(1).c_str();
                //            hexStringToBytes(dat, broadcast);
//                packets2send.clear();
                size_t i = 0;
                for (i = 0; i < 10; i++) {
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send[i], toSend);

                    packets2send[i]->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_0x28;
                    memorizeOther2W.memorizedData = toSend;
                    memorizeOther2W.memorizedCmd = iohcDevice::SEND_DISCOVER_0x28;

                    packets2send[i]->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                    packets2send[i]->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                    memcpy(packets2send[i]->payload.packet.header.source, gateway, 3);
                    memcpy(packets2send[i]->payload.packet.header.target, broadcast, 3);

                    packets2send[i]->delayed = 250; // Give enough time for the answer
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::discover2A: {
                // 00003b 5cd68f 2a  9332d618de2a0fa6250e2c7e
                //                std::vector<uint8_t>  toSend = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12};

                // uint8_t broadcast[3];
                // const char* dat = data->at(1).c_str();
                // hexStringToBytes(dat, broadcast);
                //                uint8_t broadcast[3];
                address broadcast_1 = {0x00, 0xFF, 0xFB}; //data->at(1).c_str();
                address broadcast_2 = {0x00, 0x0d, 0x3b};

                address from = {0x08, 0x42, 0xe3};
                address real = {0x5c, 0xd6, 0x8f};
                address broadcast_3b = {0x00, 0x00, 0x3b};
                address broadcast_3f = {0x00, 0x00, 0x3f};

//                packets2send.clear();
                for (size_t i = 0; i < 30; i++) {
                    packets2send.push_back(new iohcPacket);

                    if (i > 20) {
                        std::vector<uint8_t> toSend = {0x00};
                        forgePacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_UNKNOWN_0x2E;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3f, 3);
                    }
                    if (i <= 20) {
                        std::vector<uint8_t> toSend = {};
                        forgePacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_0x28;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3b, 3);
                    }
                    if (i <= 10) {
                        std::vector<uint8_t> toSend = {
                            0x93, 0x32, 0xd6, 0x18, 0xde, 0x2a, 0x0f, 0xa6, 0x25, 0x0e, 0x2c, 0x7e
                        };
                        forgePacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_REMOTE_0x2A;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3b, 3);
                    }
                    //                    packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_DISCOVER_REMOTE_0x2A;

                    //                    memorizeSend.memorizedData = toSend; //.assign(toSend, toSend + 12);
                    //                    memorizeSend.memorizedCmd = iohcDevice::SEND_DISCOVER_REMOTE_0x2A;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, /*from*/real/*gateway*/, 3);

                    packets2send.back()->delayed = 250; // Give enough time for the answer
                    packets2send.back()->repeatTime = 250; // Slow down discover loop
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                _radioInstance->send(packets2send);

                break;
            }
            case Other2WButton::fake0: {
                // 09:54:36.226 > (14) 2W S 1 E 0  FROM 0842E3 TO 14E00E CMD 00, F868.950 s+0.000   >  DATA(06)  03 e7 00 00 00 00
                //                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                /*std::vector<uint8_t>*/
                std::vector<uint8_t> toSend = {0x03, 0xe7, 0x32, 0x00, 0x00, 0x00};
                //{0x03, 0x00, 0x00}; //  Not good for Cmd 0x01 Answer FE 0x10

                address gateway = {0xba, 0x11, 0xad}; //{0x08, 0x42, 0xe3};
                address from = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //

                // Those are the local reals discovered device, can be huge, and must but put in 2W.json
                address guessed[15] = {
                    {0x2D, 0xBE, 0x8D}, {0xDA, 0x2E, 0xE6}, {0x31, 0x58, 0x24}, {0x20, 0xE5, 0x2E}, {0x14, 0xe0, 0x0e},
                    {0x05, 0x4E, 0x17}, {0x1C, 0x68, 0x58}, {0x90, 0x4c, 0x09}, {0xfe, 0x90, 0xee}, {0x41, 0x56, 0x84},
                    {0x08, 0x42, 0xe3},
                    {0x47, 0x77, 0x06}, {0x48, 0x79, 0x02}, {0x8C, 0xCB, 0x30}, {0x8C, 0xCB, 0x31}
                };

//                packets2send.clear();
                size_t i = 0;
                for (i = 0; i < 15; i++) {
                    packets2send.push_back(new iohcPacket);
                    forgePacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = 0x00;
                    memorizeOther2W.memorizedData = toSend;
                    memorizeOther2W.memorizedCmd = 0x00;
                    IOHC::lastSendCmd = 0x00;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Unk1 = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, from/*gateway*/, 3);
                    memcpy(packets2send.back()->payload.packet.header.target, guessed[i], 3);

                    packets2send.back()->delayed = 250; // Give enough time for the answer
                    packets2send.back()->repeatTime = 250; // Slow down discover loop
                }
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);

                break;
            }
            case Other2WButton::ack: {
                std::vector<uint8_t> toSend = {};

//                packets2send.clear();
                packets2send.push_back(new iohcPacket);
                forgePacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::SEND_KEY_TRANSFERT_ACK_0x33;
                memorizeOther2W.memorizedCmd = iohcDevice::SEND_KEY_TRANSFERT_ACK_0x33;
                memorizeOther2W.memorizedData = toSend;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_from, 3);

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::checkCmd: {
                std::vector<uint8_t> toSend;
                // = {}; //{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; //, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12};
                uint8_t special12[] = {
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                    0x17, 0x18, 0x19, 0x20, 0x21
                };
                uint8_t specacei[] = {0x01, 0xe7, 0x00, 0x00, 0x00, 0x00};
                // address broadcast;
                // const char* dat = data->at(1).c_str();
                // hexStringToBytes(dat, broadcast);

                address from = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //
                address to_0 = {0xda, 0x2e, 0xe6}; //
                address to_1 = {0x05, 0x4e, 0x17}; //{0x31, 0x58, 0x24}; //
                address to_2 = {0x9a, 0x50, 0x65};
                address to_3 = {0x31, 0x58, 0x24};
                //{0x47, 0x77, 0x06}, {0x48, 0x79, 0x02}, {0x8C, 0xCB, 0x30}, {0x8C, 0xCB, 0x31}
                address broad = {0x00, 0x0d, 0x3b}; //{0x00, 0xFF, 0xFB}; //

                uint8_t counter = 0;
//                packets2send.clear();
                for (const auto &command: mapValid) {
                    if (command.second == 0 || (command.second == 5 && command.first != 0x19)) {
                        counter++;

                        // 0x00,  0x01, 0x03, 0x0a, 0x0c, 0x19, 0x1e, 0x20, 0x23, 0x28, 0x2a(12), 0x2c, 0x2e, 0x31, 0x32(16), 0x36, 0x38(6), 0x39, 0x3c(6), 0x46(9), 0x48(9), 0x4a(18), 0x4b
                        // ,0x50, 0x52(16), 0x54, 0x56, 0x60(21), 0x64(2), 0x6e(9), 0x6f(9), 0x71, 0x73(3), 0x80, 0x82(21), 0x84, 0x86, 0x88, 0x8a(18), 0x8b(1), 0x8e, 0x90, 0x92(16), 0x94, 0x96(12), 0x98

                        if (command.first == 0x00 || command.first == 0x01 || command.first == 0x0B || command.first ==
                            0x0E || command.first == 0x23 || command.first == 0x2A || command.first == 0x1E)
                            toSend.assign(specacei, specacei + 6);
                        if (command.first == 0x8B || command.first == 0x19)
                            toSend.assign(special12, special12 + 1);
                        if (command.first == 0x04) toSend.assign(special12, special12 + 14);
                        uint8_t special03[] = {0x03, 0x00, 0x00};
                        if (command.first == 0x03 || command.first == 0x73)
                            toSend.assign(special03, special03 + 3);
                        uint8_t special0C[] = {0xD8, 0x00, 0x00, 0x00};
                        if (command.first == 0x0C)
                            toSend.assign(special0C, special0C + 4);
                        uint8_t special0D[] = {0x05, 0xaa, 0x0d, 0x00, 0x00};
                        if (command.first == 0x0D)
                            toSend.assign(special0D, special0D + 5);
                        if (command.first == 0x64 || command.first == 0x14)
                            toSend.assign(special12, special12 + 2);
                        if (command.first == 0x2A || command.first == 0x96)
                            toSend.assign(special12, special12 + 12);
                        if (command.first == 0x38 || command.first == 0x3C || command.first == 0x3D)
                            toSend.assign(special12, special12 + 6);
                        if (command.first == 0x32 || command.first == 0x52 || command.first == 0x92)
                            toSend.assign(special12, special12 + 16);
                        if (command.first == 0x46 || command.first == 0x48 || command.first == 0x6E || command.first == 0x6F)
                            toSend.assign(special12, special12 + 9);
                        if (command.first == 0x4A || command.first == 0x8A)
                            toSend.assign(special12, special12 + 18);
                        if (command.first == 0x60 || command.first == 0x82)
                            toSend.assign(special12, special12 + 21);

                        packets2send.push_back(new iohcPacket);
                        forgePacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = command.first;
                        memorizeOther2W.memorizedCmd = packets2send.back()->payload.packet.header.cmd;

                        packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                        packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                        memcpy(packets2send.back()->payload.packet.header.source, gateway/*from*//*gateway*/, 3);
                        // if (command.first == 0x14 || command.first == 0x19 || command.first == 0x1e || command.first == 0x2a || command.first == 0x34 || command.first == 0x4a) {
                        // memcpy(packets2send.back()->payload.packet.header.target, broad, 3);
                        // } else {
                        memcpy(packets2send.back()->payload.packet.header.target, master_to, 3);
                        // }

                        //packets2send.back()->delayed = 245;
                        packets2send.back()->repeatTime = 250; // Slow down discover loop
                        packets2send.back()->delayed = 250;   // Give enough time for the answer
                    }
                    toSend.clear();
                }
                Serial.printf("valid %u\n", counter);
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                _radioInstance->send(packets2send);

                break;
            }
            default: break;
        } // switch (cmd)
        //        save(); // Save Other associated devices
    }

    /* Initialise all valids commands for scanMode(checkCmd), other arent implemented in 2W devices
     00 04 - 01 04 - 03 04 - 0a 0D - 0c 0D - 19 1a - 1e fe - 20 21 - 23 24 - 28 29 - 2a(12) 2b - 2c 2d - 2e 2f - 31 3c - 32(16) 33 - 36 37 - 38(6) 32 - 39 fe - 3c(6) 3d - 46(9) 47 - 48(9) 49 - 4a(18) 4b
     50 51 - 52(16) 53 - 54 55 - 56 57 - 60(21) .. - 64(2) 65 - 6e(9) fe - 6f(9) .. - 71 72 - 73(3) .. - 80 81 - 82(21) .. - 84  85 - 86 87 - 88 89 - 8a(18) 8c - 8b(1) 8c - 8e .. - 90 91 - 92(16) 93 - 94 95 - 96(12) 97 - 98 99
    */
    void iohcOtherDevice2W::initializeValid() {
        size_t validKey = 0;
        auto valid = std::vector<uint8_t>(255);
        std::iota(valid.begin(), valid.end(), 0);

        valid = {
            0x00, 0x01, 0x03, 0x0a, 0x0c, 0x19, 0x1e, 0x20, 0x23, 0x28, 0x2a, 0x2c, 0x2e, 0x31, 0x32, 0x36, 0x38, 0x39,
            0x3c, 0x46, 0x48, 0x4a, 0x4b,
            0x50, 0x52, 0x54, 0x56, 0x60, 0x64, 0x6e, 0x6f, 0x71, 0x73, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8b, 0x8e,
            0x90, 0x92, 0x94, 0x96, 0x98,
            //Not in firmware 02 0e 25 30 34 3a 3d 58
            0x02, 0x0b, 0x0e, 0x14, 0x16, 0x25, 0x30, 0x34, 0x3a, 0x3d, 0x58
        };

        for (int key: valid) {
            // validKey++;
            mapValid[key] = 0;
        }
        // printf("ValidKey: %d\n", validKey);
        // printf("MapValid size: %zu\n", mapValid.size());
    }

    /**
    * @brief Dump the scan result to the console for debugging purposes.
    */
    void iohcOtherDevice2W::scanDump() {
        printf("*********************** Scan result ***********************\n");

        uint8_t count = 0;

        for (auto &it: mapValid) {
            // Prints the first two bytes of the second.
            // Prints the token and argument.
            if (it.second != 0x08) {
                // Prints the first and second of the token.
                // Prints the argument string representation of the argument.
                if (it.second == 0x3C)
                    printf("%2.2x=AUTH ", it.first, it.second);
                    // Prints the string representation of the argument.
                    // Prints the string representation of the argument.
                else if (it.second == 0x80)
                    printf("%2.2x=NRDY ", it.first, it.second);
                else
                    printf("%2.2x=%2.2x\t", it.first, it.second);
                count++;
                // Prints the number of bytes to the console.
                // Prints the number of bytes to the console.
                if (count % 16 == 0) printf("\n");
            }
        }

        // Prints the number of bytes to the console.
        if (count % 16 != 0) printf("\n");

        printf("%u toCheck \n", count);
    }

    bool iohcOtherDevice2W::load() {
        _radioInstance = iohcRadio::getInstance();
        if (LittleFS.exists(OTHER_2W_FILE))
            Serial.printf("Loading Other 2W devices settings from %s\n", OTHER_2W_FILE);
        else {
            Serial.printf("*2W Other devices not available\n");
            return false;
        }

        fs::File f = LittleFS.open(OTHER_2W_FILE, "r", true);
        JsonDocument doc;
        deserializeJson(doc, f);
        f.close();

        // Iterate through the JSON object
        for (JsonPair kv: doc.as<JsonObject>()) {
            hexStringToBytes(kv.key().c_str(), _node);
            auto jobj = kv.value().as<JsonObject>();
            //     hexStringToBytes(jobj["key"].as<const char*>(), _key);
            hexStringToBytes(jobj["dst"].as<const char *>(), _dst);
            //     uint8_t btmp[2];
            //     hexStringToBytes(jobj["sequence"].as<const char*>(), btmp);

            //            /*hexStringToBytes*/(jobj["type"].as<const char*>(), _type);

            //     _sequence = (btmp[0]<<8)+btmp[1];
            //     JsonArray jarr = jobj["type"];

            //     // Iterate through the JSON array
            //     for (uint8_t i=0; i<jarr.size(); i++)
            //         _type.insert(_type.begin()+i, jarr[i].as<uint16_t>());

            //     _manufacturer = jobj["manufacturer_id"].as<uint8_t>();
        }

        return true;
    }

    /**
     * @brief
     *
     * @return true
     * @return false
     */
    bool iohcOtherDevice2W::save() {
        fs::File f = LittleFS.open(OTHER_2W_FILE, "w");

        //DynamicJsonDocument doc(256);
        JsonDocument doc;

        std::string key = bytesToHexString(_node, sizeof(_node));
        JsonObject jobj = doc[key.c_str()].to<JsonObject>();
        jobj["dst"] = _dst;
        //        uint8_t btmp[2];
        //        btmp[1] = _sequence & 0x00ff;
        //        btmp[0] = _sequence >> 8;
        //        jobj["sequence"] = bytesToHexString(btmp, sizeof(btmp));

        //        jobj["_type"] = _type;

        //        JsonArray jarr = jobj.createNestedArray("type");
        //        for (uint8_t i=0; i<_type.size(); i++)
        //            if (_type[i])
        //                jarr.add(_type.at(i));
        //            else
        //                break;
        //        jobj["manufacturer_id"] = _manufacturer;

        serializeJsonPretty/*serializeJson*/(doc, f);
        f.close();

        return true;
    }
}
