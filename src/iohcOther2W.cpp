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

#include <iohcDevice.h>
#include <iohcOther2W.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <iohcCryptoHelpers.h>
#include <string>
#include <iohcRadio.h>
#include <vector>
#include <numeric>

#include "iohcCozyDevice2W.h"

namespace IOHC {
    iohcOther2W *iohcOther2W::_iohcOtherDevice2W = nullptr;

    iohcOther2W::iohcOther2W() = default;

    iohcOther2W *iohcOther2W::getInstance() {
        if (!_iohcOtherDevice2W) {
            _iohcOtherDevice2W = new iohcOther2W();
            _iohcOtherDevice2W->load();
            _iohcOtherDevice2W->initializeValid();
        }
        return _iohcOtherDevice2W;
    }

    address fake_gateway = {0xba, 0x11, 0xad};

    void iohcOther2W::forgeAnyWPacket(iohcPacket *packet, const std::vector<uint8_t> &toSend, size_t typn = 0) {
        IOHC::relStamp = esp_timer_get_time();

        packet->payload.packet.header.cmd = 0x2A; //0x28; //typn;
        // Common Flags
        packet->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
        // packet->payload.packet.header.CtrlByte2.asByte = 0;
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
    }


    void iohcOther2W::cmd(Other2WButton cmd, Tokens *data) {
        if (!_radioInstance) {
            ets_printf("NO RADIO INSTANCE\n");
            _radioInstance = iohcRadio::getInstance();
        }
        packets2send.clear();
        iohcCozyDevice2W *cozyDevice2W = iohcCozyDevice2W::getInstance();

        // Emulates device button press
        switch (cmd) {
            case Other2WButton::discovery: {
                int bec = 0;

                for (int j = 0; j < 255; j++) {
                    packets2send.push_back(new iohcPacket);

                    std::string discovery = "d430477706ba11ad31"; //28"; //"2b578ebc37334d6e2f50a4dfa9";
                    std::vector<uint8_t> toSend = {};
                    packets2send.back()->buffer_length = hexStringToBytes(discovery, packets2send.back()->payload.buffer);
                    forgeAnyWPacket(packets2send.back(), toSend, bec);
                    packets2send.back()->payload.packet.header.cmd = 0x2e;
                    bec += 0x3B;
                    packets2send.back()->delayed = 75;
                    // packets2send.back()/*[j]*/->repeatTime = 225;
                }

                _radioInstance->send(packets2send);
                break;
            }

            case Other2WButton::getName: {

                std::vector<uint8_t> toSend = {}; // {0x0C, 0x60, 0x01, 0xFF};
                // int value = std::stol(data->at(1).c_str(), nullptr, 16);

                for (const auto &d: devices) {

                    auto packet = new iohcPacket;

                    forgeAnyWPacket(packet, toSend, 0);
                    packet->payload.packet.header.cmd = GET_NAME_0x50;

                    packet->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                    memcpy(packet->payload.packet.header.source, d._node, 3);
                    memcpy(packet->payload.packet.header.target, d._dst, 3);

                    packet->delayed = 125;

                    packets2send.push_back(packet);
                }

                IOHC::lastCmd = GET_NAME_0x50;
                cozyDevice2W->memorizeSend.memorizedCmd = GET_NAME_0x50;
                cozyDevice2W->memorizeSend.memorizedData = {};

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
                    forgeAnyWPacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = 0x00; //WRITE_PRIVATE_0x20;
                    memorizeOther2W.memorizedData = toSend;
                    memorizeOther2W.memorizedCmd = 0x00; //SEND_WRITE_PRIVATE_0x20;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, from/*gateway*/, 3);
                    memcpy(packets2send.back()->payload.packet.header.target, to_1, 3);

                    packets2send.back()->delayed = 150;
                }

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
                forgeAnyWPacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::WRITE_PRIVATE_0x20;
                memorizeOther2W.memorizedData = toSend;
                memorizeOther2W.memorizedCmd = iohcDevice::WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;

                memcpy(packets2send.back()->payload.packet.header.source, gateway/*master_from*/, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_to/*slave_to*/, 3);

                packets2send.back()->delayed = 250;
                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::discover28: {
                std::vector<uint8_t> toSend = {};
                // 0x00003f  b00000000 0000000000 111111   all
                // 0x00013f  b00000000 0000000100 111111   window 1 (Window opener)
                // 0x00007f  b00000000 0000000001 111111   blind 1 (Venetian Blind)
                // 0x0002bf  b00000000 0000001010 111111   blind 2 (Blind)
                // 0x0000bf  b00000000 0000000010 111111   shutter 1 (Roller Shutter)
                // 0x0000ff  b00000000 0000000011 111111   shutter 2 (Awning - External for window)
                // 0x00037f  b00000000 0000001101 111111   shutter 3 (Dual Shutter)
                address broadcast = {0x00, 0xFF, 0xFB}; //{0x02, 0x02, 0xFB}; //data->at(1).c_str();
                //            hexStringToBytes(dat, broadcast);


                size_t type = 0;
                for (int i = 0; i < 3; i++)
                for (type = 0; type < 64; type++) {
                    address command = {0x28, 0x2a, 0x2e};
                    packets2send.push_back(new iohcPacket);
                    forgeAnyWPacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = command[i]; //iohcDevice::SEND_DISCOVER_0x28;
                    memorizeOther2W.memorizedData = toSend;
                    memorizeOther2W.memorizedCmd = iohcDevice::DISCOVER_0x28;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;
                    address tahoma = {0xEF, 0x37, 0x12};

                    memcpy(packets2send.back()->payload.packet.header.source, tahoma/*gateway*/, 3);
                    // format b00000000 xxxxxxxx 111111
                    uint16_t bcast = (type<<6) + 0b111011;
                    broadcast[0] = (bcast >> 16) & 0xFF;  // Partie haute (sera 0, car bcast est sur 16 bits)
                    broadcast[1] = (bcast >> 8) & 0xFF;   //Octet du milieu
                    // broadcast[1] = 0xF0 | ( (bcast >> 8) & 0x0F ); 0x00Fxxx
                    broadcast[2] = bcast & 0xFF;          // Octet de poids faible

                    memcpy(packets2send.back()->payload.packet.header.target, broadcast, 3);

                    packets2send.back()->delayed = 75;
                }

                _radioInstance->send(packets2send);
                break;
            }
            case Other2WButton::discover2A: {
                // 00003b 5cd68f 2a  9332d618de2a0fa6250e2c7e
                // std::vector<uint8_t>  toSend = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12};

                address broadcast_1 = {0x00, 0xFF, 0xFB};

                address GW1 = {0x08, 0x42, 0xe3};
                address real = {0x5c, 0xd6, 0x8f}; // Discovery Session from 2W KLR200 (5cd68f)
                address tahoma = {0xEF, 0x37, 0x12};
                address somfy = {0xe0, 0x98, 0x48}; // Somfy Remote (E09848)
                address broadcast_3b = {0x00, 0x00, 0x3b};
                address broadcast_3f = {0x00, 0x00, 0x3f};
                // 24 types
                // 0x00003f  b00000000 0000000000 111111   all
                // 0x00013f  b00000000 0000000100 111111   window 1 (Window opener)
                // 0x00007f  b00000000 0000000001 111111   blind 1 (Venetian Blind)
                // 0x0002bf  b00000000 0000001010 111111   blind 2 (Blind)
                // 0x0000bf  b00000000 0000000010 111111   shutter 1 (Roller Shutter)
                // 0x0000ff  b00000000 0000000011 111111   shutter 2 (Awning - External for window)
                // 0x00037f  b00000000 0000001101 111111   shutter 3 (Dual Shutter)

                for (size_t i = 0; i < 6; i++) {
                    packets2send.push_back(new iohcPacket);
                    if (i == 5) {
                        std::vector<uint8_t> toSend = {};
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::DISCOVER_0x28;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3b, 3);
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 0;
                    }
                    if (i == 4) {
                        std::vector<uint8_t> toSend = {};
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::DISCOVER_0x28;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3f, 3);
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 0;
                    }
                    if (i == 3) {
                        std::vector<uint8_t> toSend = {
                            0x93, 0x32, 0xd6, 0x18, 0xde, 0x2a, 0x0f, 0xa6, 0x25, 0x0e, 0x2c, 0x7e
                        };
                        toSend = {0x0c, 0x3d, 0xd2, 0x09, 0x2d, 0x87, 0x20, 0x8d, 0x46, 0xb7, 0x92, 0x73};
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::DISCOVER_REMOTE_0x2A;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3b, 3);
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;

                    }
                    if (i == 2) {
                        std::vector<uint8_t> toSend = {
                            0x93, 0x32, 0xd6, 0x18, 0xde, 0x2a, 0x0f, 0xa6, 0x25, 0x0e, 0x2c, 0x7e
                        };
                        toSend = {0x0c, 0x3d, 0xd2, 0x09, 0x2d, 0x87, 0x20, 0x8d, 0x46, 0xb7, 0x92, 0x73};
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::DISCOVER_REMOTE_0x2A;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3f, 3);
                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    }
                    if (i == 1) {
                        std::vector<uint8_t> toSend = {0x00};
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::UNKNOWN_0x2E;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3b, 3);
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;

                    }
                    if (i == 0) {
                        std::vector<uint8_t> toSend = {0x00};
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = iohcDevice::UNKNOWN_0x2E;
                        memcpy(packets2send.back()->payload.packet.header.target, broadcast_3f, 3);
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;

                    }
                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;

                    packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 0;
                    // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Unk3 = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, somfy, 3);
                    packets2send.back()->repeat = 4;
                    packets2send.back()->repeatTime = 60;
                    // packets2send.back()->delayed = 75; // Give enough time for the answer
                    // packets2send.back()->frequency = CHANNEL3;
                }

                _radioInstance->send(packets2send);

                break;
            }
            case Other2WButton::fake0: {
                std::vector<uint8_t> toSend ={0x03, 0x00, 0x00}; //{0x03, 0xe7, 0x32, 0x00, 0x00, 0x00};

                size_t i = 0;
                for (const auto &d: devices) {
                    packets2send.push_back(new iohcPacket);
                    forgeAnyWPacket(packets2send.back(), toSend);

                    packets2send.back()->payload.packet.header.cmd = 0x03;
                    memorizeOther2W.memorizedData = toSend;
                    memorizeOther2W.memorizedCmd = 0x03;
                    IOHC::lastCmd = 0x03;
                    cozyDevice2W->memorizeSend.memorizedCmd = 0x03;
                    cozyDevice2W->memorizeSend.memorizedData = toSend;

                    packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                    // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                    // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Unk1 = 1;

                    memcpy(packets2send.back()->payload.packet.header.source, d._node, 3);
                    memcpy(packets2send.back()->payload.packet.header.target, d._dst, 3);

                    packets2send.back()->delayed = 120;
                }

                _radioInstance->send(packets2send);

                break;
            }
            case Other2WButton::ack: {
                std::vector<uint8_t> toSend = {};

                packets2send.push_back(new iohcPacket);
                forgeAnyWPacket(packets2send.back(), toSend);

                packets2send.back()->payload.packet.header.cmd = iohcDevice::KEY_TRANSFERT_ACK_0x33;
                memorizeOther2W.memorizedCmd = iohcDevice::KEY_TRANSFERT_ACK_0x33;
                memorizeOther2W.memorizedData = toSend;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;

                memcpy(packets2send.back()->payload.packet.header.source, gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, master_from, 3);

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
                // MY_GATEWAY 4d595f47415445574159
                std::vector<uint8_t> myName = {0x4d, 0x59, 0x5f, 0x47, 0x41, 0x54, 0x45, 0x57, 0x41, 0x59};

                address from = {0x08, 0x42, 0xe3}; //data->at(1).c_str(); //
                address to_0 = {0xda, 0x2e, 0xe6}; //
                address to_1 = {0x05, 0x4e, 0x17}; //{0x31, 0x58, 0x24}; //
                address to_2 = {0x9a, 0x50, 0x65};
                address to_3 = {0x31, 0x58, 0x24};
                //{0x47, 0x77, 0x06}, {0x48, 0x79, 0x02}, {0x8C, 0xCB, 0x30}, {0x8C, 0xCB, 0x31}
                address broad = {0x00, 0x0d, 0x3b}; //{0x00, 0xFF, 0xFB}; //

                uint8_t counter = 0;
//                packets2send.clear();
                for (const auto &[CmdSend, Answer]: mapValid) {
                    if (Answer == 0xFF || (Answer == 5 && CmdSend != 0x19)) {
                        counter++;

                        // 0x00,  0x01, 0x03, 0x0a, 0x0c, 0x19, 0x1e, 0x20, 0x23, 0x28, 0x2a(12), 0x2c, 0x2e, 0x31, 0x32(16), 0x36, 0x38(6), 0x39, 0x3c(6), 0x46(9), 0x48(9), 0x4a(18), 0x4b
                        // ,0x50, 0x52(16), 0x54, 0x56, 0x60(21), 0x64(2), 0x6e(9), 0x6f(9), 0x71, 0x73(3), 0x80, 0x82(21), 0x84, 0x86, 0x88, 0x8a(18), 0x8b(1), 0x8e, 0x90, 0x92(16), 0x94, 0x96(12), 0x98

                        if (CmdSend == 0x00 || CmdSend == 0x01 || CmdSend == 0x0B || CmdSend ==
                            0x0E || CmdSend == 0x23 || CmdSend == 0x2A || CmdSend == 0x1E)
                            toSend.assign(specacei, specacei + 6);
                        if (CmdSend == 0x8B || CmdSend == 0x19)
                            toSend.assign(special12, special12 + 1);
                        if (CmdSend == 0x04) toSend.assign(special12, special12 + 14);
                        uint8_t special03[] = {0x03, 0x00, 0x00};
                        if (CmdSend == 0x03 || CmdSend == 0x73)
                            toSend.assign(special03, special03 + 3);
                        uint8_t special0C[] = {0xD8, 0x00, 0x00, 0x00};
                        if (CmdSend == 0x0C)
                            toSend.assign(special0C, special0C + 4);
                        uint8_t special0D[] = {0x05, 0xaa, 0x0d, 0x00, 0x00}; // aa1c - aa0a
                        if (CmdSend == 0x0D)
                            toSend.assign(special0D, special0D + 5);
                        if (CmdSend == 0x64 || CmdSend == 0x14)
                            toSend.assign(special12, special12 + 2);
                        if (CmdSend == 0x2A || CmdSend == 0x96)
                            toSend.assign(special12, special12 + 12);
                        if (CmdSend == 0x38 || CmdSend == 0x3D)
                            toSend.assign(special12, special12 + 6);
                        if (CmdSend == 0x3C)
                            toSend = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                        if (CmdSend == 0x32 || CmdSend == 0x92)
                            toSend.assign(special12, special12 + 16);
                        if (CmdSend == 0x52) toSend = myName;
                        if (CmdSend == 0x46 || CmdSend == 0x48 || CmdSend == 0x6E || CmdSend == 0x6F)
                            toSend.assign(special12, special12 + 9);
                        if (CmdSend == 0x4A || CmdSend == 0x8A)
                            toSend.assign(special12, special12 + 18);
                        if (CmdSend == 0x60 || CmdSend == 0x82)
                            toSend.assign(special12, special12 + 21);

                        packets2send.push_back(new iohcPacket);
                        forgeAnyWPacket(packets2send.back(), toSend);
                        packets2send.back()->payload.packet.header.cmd = CmdSend;
                        // memorizeOther2W.memorizedCmd = packets2send.back()->payload.packet.header.cmd;
                        // memorizeOther2W.memorizedData = toSend;
                        memorizeOther2W.memorizedCmd = CmdSend;
                        IOHC::lastCmd = CmdSend;

                        packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                        packets2send.back()->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
                        // packets2send.back()->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
                        packets2send.back()->payload.packet.header.CtrlByte2.asStruct.Prio = 1;
                        address GW2 = {0x26, 0x94, 0x11};
                        address GW1 = {0x08, 0x42, 0xe3};
                        address BUREAU = {0x90, 0x4c, 0x09}; //{0x94, 0x78, 0x6E};
                        address SALONRUE = {0x05, 0x4E, 0x17};
                        address PORTAIL = {0x9A, 0x5C, 0xA0}; // TODO search the 2W address
                        address GARAGE = {0x41, 0x56, 0x84};
                        address tahoma = {0xEF, 0x37, 0x12};
                        address UK1 = {0x5B, 0xE4, 0xD0};
                        address UK2 = {0x94, 0x78, 0x6E}; //94786E

                        memcpy(packets2send.back()->payload.packet.header.source, GW2, 3);
                        // if (command.first == 0x14 || command.first == 0x19 || command.first == 0x1e || command.first == 0x2a || command.first == 0x34 || command.first == 0x4a) {
                        // memcpy(packets2send.back()->payload.packet.header.target, broad, 3);
                        // } else {
                        memcpy(packets2send.back()->payload.packet.header.target, UK2, 3);
                        // }

                        packets2send.back()->delayed = 145;
                    }
                    toSend.clear();
                }
                // ets_printf("valid %u\n", counter);

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
     EE 03 ??*/
    void iohcOther2W::initializeValid() {
        // size_t validKey = 0;
        // auto valid = std::vector<uint8_t>(255);
        // std::iota(valid.begin(), valid.end(), 0x00);

        const std::vector valid = {
            0x00, 0x01, 0x03, 0x0a, 0x0c, 0x19, 0x1e, 0x20, 0x23, 0x28, 0x2a, 0x2c, 0x38, 0x2e, 0x31, 0x32, 0x36, 0x39,
            0x46, 0x48, 0x4a, 0x4b,
            0x50, 0x52, 0x54, 0x56, 0x60, 0x64, 0x6e, 0x6f, 0x71, 0x73, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8b, 0x8e,
            0x90, 0x92, 0x94, 0x96, 0x98,
            // 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
            // 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
            //Not found in firmware 02 0e 25 30 34 3a 58 and send 0x3c at end ;)
            0x02, 0x0b, 0x0e, 0x14, 0x16, 0x25, 0x30, 0x34, 0x3a, 0x58, 0x3c
        };

        for (int key: valid) {
            mapValid[key] = 0xff;
        }
        // printf("ValidKey: %d\n", validKey);
        // printf("MapValid size: %zu\n", mapValid.size());
    }

    /**
    * @brief Dump the scan result to the console for debugging purposes.
    */
    void iohcOther2W::scanDump() {
        ets_printf("*********************** Scan result ***********************\n");

        uint8_t count = 0;

        for (auto &[Command, Answer]: mapValid) {
            if (Answer != 0x08) {
                if (Answer == 0x3C)
                    ets_printf("%2.2x=AUTH ", Command, Answer);
                else if (Answer == 0x80)
                    ets_printf("%2.2x=NRDY ", Command, Answer);
                else
                    ets_printf("%2.2x=%2.2x\t", Command, Answer);
                count++;
                if (count % 16 == 0) ets_printf("\n");
            }
        }
        if (count % 16 != 0) ets_printf("\n");

        ets_printf("%u toCheck \n", count);
    }

    /**
    * @brief Load Cozy 2W settings from file and store in _radioInstance.
    * @return True if successful false otherwise. This is a blocking call
    */
    bool iohcOther2W::load() {
        _radioInstance = iohcRadio::getInstance();
        // Load Cozy 2W device settings from file
        if (LittleFS.exists(OTHER_2W_FILE))
            ets_printf("Loading Cozy 2W devices settings from %s\n", OTHER_2W_FILE);
        else {
            ets_printf("*2W Cozy devices not available\n");
            return false;
        }

        fs::File f = LittleFS.open(OTHER_2W_FILE, "r", true);
        JsonDocument doc;
        deserializeJson(doc, f);
        f.close();

        for (JsonPair kv : doc.as<JsonObject>()) {
            auto deviceObj = kv.value().as<JsonObject>();

            // Key is dst (2W gateway associated with a simple 1W)
            std::string dstStr = kv.key().c_str();
            std::string nodeStr = deviceObj["node"].as<std::string>();

            Device device;
            hexStringToBytes(nodeStr, device._node);
            hexStringToBytes(dstStr, device._dst);

            device._type = deviceObj["type"].as<std::string>();
            device._description = deviceObj["description"].as<std::string>();

            auto assoStr = deviceObj["associated"].as<std::string>();
            hexStringToBytes(assoStr, device._associated);

            devices.push_back(device);
        }
        ets_printf("Loaded %d x 2W devices\n", devices.size());

        return true;
    }

    bool iohcOther2W::save() {
        fs::File f = LittleFS.open(OTHER_2W_FILE, "w");

        JsonDocument doc;
        for (const auto &d: devices) {
            // use dst as a key instead of node
            std::string key = bytesToHexString(d._dst, sizeof(address));
            auto jobj = doc[key.c_str()].to<JsonObject>();
            jobj["node"] = bytesToHexString(d._node, sizeof(address));
            jobj["type"] = d._type;
            jobj["description"] = d._description;

            jobj["associated"] = bytesToHexString(d._associated, sizeof(address));
        }
        // serializeJsonPretty(doc, f);
        serializeJson(doc, f);
        f.close();
        // ets_printf("Saved %d x 2W devices\n", devices.size());
        return true;
    }

}
