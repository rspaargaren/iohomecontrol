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

#include <cstdio>
#include <cstring>
#include <esp_attr.h>
#include <iohcPacket.h>
#include <iomanip>
#include <sstream>
#include <utils.h>
#include <rom/ets_sys.h>

namespace IOHC {

    void IRAM_ATTR iohcPacket::decode(bool verbosity) {

        if (packetStamp - relStamp > 500000L) {
            ets_printf("\n");
            relStamp = packetStamp; // - this->relStamp;
        }
        char _dir[3] = {};

        if(this->payload.packet.header.CtrlByte1.asStruct.Protocol) {
            _dir[0] = '>';
        }

        else if (this->payload.packet.header.CtrlByte1.asStruct.StartFrame && !this->payload.packet.header.CtrlByte1.asStruct.EndFrame) _dir[0] = '>';
        else if (!this->payload.packet.header.CtrlByte1.asStruct.StartFrame && this->payload.packet.header.CtrlByte1.asStruct.EndFrame) _dir[0] = '<';
        else _dir[0] = ' ';

        ets_printf("(%2.2u) %1xW S %s E %s ", this->payload.packet.header.CtrlByte1.asStruct.MsgLen,
               this->payload.packet.header.CtrlByte1.asStruct.Protocol ? 1 : 2,
               this->payload.packet.header.CtrlByte1.asStruct.StartFrame ? "1" : "0",
               this->payload.packet.header.CtrlByte1.asStruct.EndFrame ? "1" : "0");

        if (this->payload.packet.header.CtrlByte2.asStruct.LPM) ets_printf("[LPM]");
        if (this->payload.packet.header.CtrlByte2.asStruct.Beacon) ets_printf("[B]");
        if (this->payload.packet.header.CtrlByte2.asStruct.Routed) ets_printf("[R]");
        if (this->payload.packet.header.CtrlByte2.asStruct.Prio) ets_printf("[PRIO]");
        if (this->payload.packet.header.CtrlByte2.asStruct.Unk2) ets_printf("[U2]");
        if (this->payload.packet.header.CtrlByte2.asStruct.Unk3) ets_printf("[U3]");
        if (this->payload.packet.header.CtrlByte2.asStruct.Version)
            ets_printf( "[V]%u", this->payload.packet.header.CtrlByte2.asStruct.Version);

        //            const char *commandName = commands[msg_cmd_id].c_str();
        //            ets_print(commandName);

        ets_printf("\tFROM %2.2X%2.2X%2.2X TO %2.2X%2.2X%2.2X CMD %2.2X",
               this->payload.packet.header.source[0], this->payload.packet.header.source[1],
               this->payload.packet.header.source[2],
               this->payload.packet.header.target[0], this->payload.packet.header.target[1],
               this->payload.packet.header.target[2],
               this->payload.packet.header.cmd);
        
        // if (verbosity) printf(" +%03.3f F%03.3f, %03.1fdBm %f %f\t", static_cast<float>(packetStamp - relStamp)/1000.0, static_cast<float>(this->frequency)/1000000.0, this->rssi, this->lna, this->afc);
        // if (verbosity) printf(" +%03.3f F%03.3f, %03.1fdBm %f\t", static_cast<float>(packetStamp - relStamp)/1000.0, static_cast<float>(this->frequency)/1000000.0, this->rssi, this->afc);
        // if (verbosity) printf(" +%03.3f\t%03.1fdBm\t", static_cast<float>(packetStamp - relStamp)/1000.0,  this->rssi);
        // if (verbosity) printf(" +%03.3f F%03.3f\t", static_cast<float>(packetStamp - relStamp)/1000.0, static_cast<float>(this->frequency)/1000000.0);
        if (verbosity) ets_printf(" +%03d\t", static_cast<int>((packetStamp - relStamp)/1000.0));
        ets_printf(" %s ", _dir);

        uint8_t dataLen = this->buffer_length - 9;
        ets_printf(" DATA(%2.2u) ", dataLen);

        // 1W fields
        if (this->payload.packet.header.CtrlByte1.asStruct.Protocol) {
            std::string msg_data = bitrow_to_hex_string(this->payload.buffer + 9, dataLen);
            ets_printf(" %s", msg_data.c_str());

            switch (this->payload.packet.header.cmd) {
                case 0x30: {
                    ets_printf("\tMANU %X DATA %X ", this->payload.packet.msg.p0x30.man_id, this->payload.packet.msg.p0x30.data);
                    ets_printf("\tKEY %s SEQ %s ", bitrow_to_hex_string(this->payload.packet.msg.p0x30.enc_key, 16).c_str(),
                           bitrow_to_hex_string(this->payload.packet.msg.p0x30.sequence, 2).c_str());
                    break;
                }
                case 0x2E:
                case 0x39: {
                    ets_printf("\tDATA %X ", this->payload.packet.msg.p0x2e.data);
                    ets_printf("\tSEQ %s MAC %s ", bitrow_to_hex_string(this->payload.packet.msg.p0x2e.sequence, 2).c_str(),
                           bitrow_to_hex_string(this->payload.packet.msg.p0x2e.hmac, 6).c_str());
                    break;
                }
                // case 0x28:
                case 0x00:
                case 0x01: {
                    // auto main = static_cast<unsigned>((this->payload.packet.msg.p0x00.main[0] << 8) | this->payload.packet.msg.p0x00.main[1]);
                    // printf("Org %X Acei %X Main %X fp1 %X fp2 %X ", this->payload.packet.msg.p0x00.origin, this->payload.packet.msg.p0x00.acei, main, this->payload.packet.msg.p0x00.fp1, this->payload.packet.msg.p0x00.fp2);
                    // ets_printf("tSEQ %s Hmac %s", bitrow_to_hex_string(this->payload.packet.msg.p0x01_13.sequence, 2).c_str(), bitrow_to_hex_string(this->payload.packet.msg.p0x01_13.hmac, 6).c_str());
                    //                    int msg_seq_nr = 0;
                    //                    msg_seq_nr = (this->payload.buffer[9 + data_length] << 8) | this->payload.buffer[9 + data_length/* + 1*/];
                    //                    ets_printf("\tSEQ %3.2X", msg_seq_nr);
                    //                    std::string msg_mac = bitrow_to_hex_string(this->payload.buffer + 9 + data_length + 2, 6);
                    //                    ets_printf(" MAC %s ", msg_mac.c_str());

                    if (dataLen == 13) {
                        ets_printf("\tSEQ %s MAC %s ",
                               bitrow_to_hex_string(this->payload.packet.msg.p0x01_13.sequence, 2).c_str(),
                               bitrow_to_hex_string(this->payload.packet.msg.p0x01_13.hmac, 6).c_str());

                        auto main = static_cast<unsigned>((this->payload.packet.msg.p0x01_13.main) /*[0] << 8) | this->payload.packet.msg.p0x01_13.main[1]*/);
                        ets_printf(" Orig %X Acei %X Main %X fp1 %X fp2 %X ", this->payload.packet.msg.p0x01_13.origin,
                               this->payload.packet.msg.p0x01_13.acei.asByte, main,
                               this->payload.packet.msg.p0x01_13.fp1,
                               this->payload.packet.msg.p0x01_13.fp2);

                        auto acei = this->payload.packet.msg.p0x01_13.acei;
                        ets_printf("Acei %u %u %u %u ", acei.asStruct.level, acei.asStruct.service, acei.asStruct.extended, acei.asStruct.isvalid);
                    }
                    if (dataLen == 14) {
                        ets_printf("\tSEQ %s MAC %s ",
                               bitrow_to_hex_string(this->payload.packet.msg.p0x00_14.sequence, 2).c_str(),
                               bitrow_to_hex_string(this->payload.packet.msg.p0x00_14.hmac, 6).c_str());

                        auto main = static_cast<unsigned>((this->payload.packet.msg.p0x00_14.main[0] << 8) /* | this->payload.packet.msg.p0x00_14.main[1]*/);
                        ets_printf(" Orig %X Acei %X Main %X fp1 %X fp2 %X ", this->payload.packet.msg.p0x00_14.origin,
                               this->payload.packet.msg.p0x00_14.acei.asByte, main,
                               this->payload.packet.msg.p0x00_14.fp1,
                               this->payload.packet.msg.p0x00_14.fp2);

                        auto acei = this->payload.packet.msg.p0x00_14.acei;
                        ets_printf("Acei %u %u %u %u ", acei.asStruct.level, acei.asStruct.service, acei.asStruct.extended, acei.asStruct.isvalid);
                    }
                    if (dataLen == 16) {
                        ets_printf("\tSEQ %s MAC %s ",
                               bitrow_to_hex_string(this->payload.packet.msg.p0x00_16.sequence, 2).c_str(),
                               bitrow_to_hex_string(this->payload.packet.msg.p0x00_16.hmac, 6).c_str());

                        auto main = static_cast<unsigned>((this->payload.packet.msg.p0x00_16.main[0] << 8) | this->payload.packet.msg.p0x00_16.main[1]);
                        auto data = static_cast<unsigned>((this->payload.packet.msg.p0x00_16.data[0] << 8) | this->payload.packet.msg.p0x00_16.data[1]);
                        ets_printf(" Orig %X Acei %X Main %4X fp1 %X fp2 %X Data %4X", this->payload.packet.msg.p0x00_16.origin,
                               this->payload.packet.msg.p0x00_16.acei.asByte, main,
                               this->payload.packet.msg.p0x00_16.fp1,
                               this->payload.packet.msg.p0x00_16.fp2, data);

                        auto acei = this->payload.packet.msg.p0x00_16.acei;
                        ets_printf(" Acei %u %u %u %u ", acei.asStruct.level, acei.asStruct.service, acei.asStruct.extended, acei.asStruct.isvalid);
                    }
                    break;
                }
                case 0x20: {
                    if (dataLen == 13) {
                        ets_printf("\tSEQ %s MAC %s ",
                               bitrow_to_hex_string(this->payload.packet.msg.p0x20_13.sequence, 2).c_str(),
                               bitrow_to_hex_string(this->payload.packet.msg.p0x20_13.hmac, 6).c_str());

                        auto main = static_cast<unsigned>((this->payload.packet.msg.p0x20_13.main[0] << 8) | this->payload.packet.msg.p0x20_13.main[1]);
                        ets_printf(" Manu %X Acei %X Main %X fp1 %X ", this->payload.packet.msg.p0x20_13.origin,
                               this->payload.packet.msg.p0x20_13.acei.asByte, main,
                               this->payload.packet.msg.p0x20_13.fp1
                               );

                        // auto acei = this->payload.packet.msg.p0x20_13.acei;
                        // printf(" Acei %u %u %u %u ", acei.asStruct.level, acei.asStruct.service, acei.asStruct.extended, acei.asStruct.isvalid);
                    }
                    if (dataLen == 15) {
                        ets_printf("\tSEQ %s MAC %s ",
                               bitrow_to_hex_string(this->payload.packet.msg.p0x20_15.sequence, 2).c_str(),
                               bitrow_to_hex_string(this->payload.packet.msg.p0x20_15.hmac, 6).c_str());

                        auto main = static_cast<unsigned>(  (this->payload.packet.msg.p0x20_15.main[0] << 8) | this->payload.packet.msg.p0x20_15.main[1]);
                        ets_printf(" Manu %X Acei %X Main %X fp1 %X fp2 %X fp3 %X ", this->payload.packet.msg.p0x20_15.origin,
                               this->payload.packet.msg.p0x20_15.acei.asByte, main,
                               this->payload.packet.msg.p0x20_15.fp1,
                               this->payload.packet.msg.p0x20_15.fp2,
                               this->payload.packet.msg.p0x20_15.fp3);

                        // auto acei = this->payload.packet.msg.p0x20_15.acei;
                        // printf(" Acei %u %u %u %u ", acei.asStruct.level, acei.asStruct.service, acei.asStruct.extended, acei.asStruct.isvalid);
                    }
                    if (dataLen == 16) {
                        ets_printf("\tSEQ %s MAC %s ",
                               bitrow_to_hex_string(this->payload.packet.msg.p0x20_16.sequence, 2).c_str(),
                               bitrow_to_hex_string(this->payload.packet.msg.p0x20_16.hmac, 6).c_str());

                        auto main = static_cast<unsigned>((this->payload.packet.msg.p0x20_16.main[0] << 8) | this->payload.packet.msg.p0x20_16.main[1]);
                        auto data = static_cast<unsigned>((this->payload.packet.msg.p0x20_16.data[0] << 8) | this->payload.packet.msg.p0x20_16.data[1]);
                        ets_printf(" Manu %X Acei %X Main %4X fp1 %X fp2 %X Data %4X", this->payload.packet.msg.p0x20_16.origin,
                               this->payload.packet.msg.p0x20_16.acei.asByte, main,
                               this->payload.packet.msg.p0x20_16.fp1,
                               this->payload.packet.msg.p0x20_16.fp2, data);

                        // auto acei = this->payload.packet.msg.p0x20_16.acei;
                        // printf(" Acei %u %u %u %u ", acei.asStruct.level, acei.asStruct.service, acei.asStruct.extended, acei.asStruct.isvalid);
                    }
                    break;
                }

                default: {
                    ets_printf("Undecoded %s", bitrow_to_hex_string(this->payload.buffer + 9, dataLen).c_str());

                    // int msg_seq_nr = 0;
                    // msg_seq_nr = (this->payload.buffer[9 + data_length] << 8) | this->payload.buffer[9 + data_length + 1];
                    // ets_printf("\tSEQ %3.2X", msg_seq_nr);

                    // std::string msg_mac = bitrow_to_hex_string(this->payload.buffer + 9 + data_length + 2, 6);
                    // ets_printf(" MAC %s ", msg_mac.c_str());

                    // ets_printf("\tSEQ %s MAC %s ", bitrow_to_hex_string(this->payload.packet.msg.p0x00.sequence, 2).c_str(), bitrow_to_hex_string(this->payload.packet.msg.p0x00.hmac, 6).c_str());
                }
            }
            uint16_t broadcast = ((this->payload.packet.header.target[1]) << 2) | ( (this->payload.packet.header.target[2] >> 6) & 0x03);
            const char* typeName = sDevicesType[broadcast].c_str();
            ets_printf(" Type %s ", typeName);
        } else {
            // 2W fields
            if (dataLen != 0) {
                std::string msg_data = bitrow_to_hex_string(this->payload.buffer + 9, dataLen);
                ets_printf(" %s", msg_data.c_str());
                /*Private Atlantic/Sauter/Thermor*/
                if (this->payload.packet.header.cmd == 0x20) {}
            }
        }

        ets_printf("\n");

        relStamp = packetStamp;
    }

    std::string iohcPacket::decodeToString(bool verbosity) {
        std::ostringstream ss;
        char dir = ' ';

        if(this->payload.packet.header.CtrlByte1.asStruct.Protocol) dir = '>';
        else if (this->payload.packet.header.CtrlByte1.asStruct.StartFrame && !this->payload.packet.header.CtrlByte1.asStruct.EndFrame) dir = '>';
        else if (!this->payload.packet.header.CtrlByte1.asStruct.StartFrame && this->payload.packet.header.CtrlByte1.asStruct.EndFrame) dir = '<';

        ss << "(" << std::setw(2) << std::setfill('0') << std::dec
           << static_cast<int>(this->payload.packet.header.CtrlByte1.asStruct.MsgLen) << ") ";
        ss << (this->payload.packet.header.CtrlByte1.asStruct.Protocol ? "1W" : "2W") << " ";
        ss << "FROM " << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(this->payload.packet.header.source[0])
           << static_cast<int>(this->payload.packet.header.source[1])
           << static_cast<int>(this->payload.packet.header.source[2])
           << " TO "
           << static_cast<int>(this->payload.packet.header.target[0])
           << static_cast<int>(this->payload.packet.header.target[1])
           << static_cast<int>(this->payload.packet.header.target[2])
           << " CMD " << static_cast<int>(this->payload.packet.header.cmd);

        uint8_t dataLen = this->buffer_length - 9;
        ss << " DATA(" << std::dec << static_cast<int>(dataLen) << ") ";
        if (dataLen)
            ss << bitrow_to_hex_string(this->payload.buffer + 9, dataLen);
        ss << " " << dir;
        return ss.str();
    }
}
