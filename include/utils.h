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

#ifndef IOHC_UTILS_H
#define IOHC_UTILS_H

#include <unordered_map>
/* Various Part*/
#include <iohcPacket.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace IOHC {
#define to_hex_str(hex_val) (static_cast<std::stringstream const&>(std::stringstream() << /*"0x" <<*/ std::hex << static_cast<int>(hex_val))).str()
#define bytesToStr(val, width) (static_cast<std::stringstream const&>(std::stringstream() << std::hex << std::setw(width) << std::setfill('0') << val)).str()

inline std::string bitrow_to_hex_string(const uint8_t* bitrow, unsigned bit_len) {
        // Vérifier si bitrow est vide ou si bit_len est égal à 0
//    if (bitrow == nullptr || bit_len == 0) {
//        return "";
//    }
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned i = 0; i < (8 * bit_len + 7) / 8; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(bitrow[i]);
    }
    return ss.str();
    }

//#define to_hex_str(uint8_val) \
//    (static_cast<std::stringstream const&>(std::stringstream() << std::hex << std::setw(2) << std::setfill('0') << /*static_cast<int>*/(uint8_val))).str()
//std::string to_hex_str(uint8_t value) {
//    std::stringstream stream;
//    stream << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value);
//    return stream.str();
//}
    // int bitrow_snprint(uint8_t const *bitrow, unsigned bit_len, char *str, unsigned size){
    //     if (bit_len == 0 && size > 0) str[0] = '\0';
    //     int len = 0;
    //     for (unsigned i = 0; size > (unsigned)len && i < (bit_len + 7) / 8; ++i) {
    //         len += snprintf(str + len, size - len, "%02x", bitrow[i]);
    //     }
    //     return len;
    // }

// We make the code more readable
inline std::unordered_map<int, std::string> sCommandId = {
{0xFF,"DO_NOTHING"}, 
{0x00,"EXECUTE FUNCTION 0x00     "}, 
{0x01,"ACTIVATE MODE 0x01        "},
{0x03,"PRIVATE REQ 0x03          "},
{0x04,"PRIVATE ACK 0x04          "},
{0x19,"SET SENSOR VALUE 0x19     "},  
{0x20,"PRIVATE REQ 0x20          "},
{0x21,"PRIVATE ACK 0x21          "},
{0x28,"DISCOVER REQ 0x28         "},
{0x29,"DISCOVER ACK 0x29         "},
{0x2A,"DISCOVER REMOTE REQ 0x2A  "},
{0x2B,"DISCOVER REMOTE ACK 0x2B  "},
{0x2C,"DISCOVER ACTUATOR REQ 0x2C"},
{0x2D,"DISCOVER ACTUATOR ACK 0x2D"},
{0x2E,"UNKNOWN_0x2E              "},
{0x31,"ASK_CHALLENGE_0x31        "},
{0x32,"KEY TRANSFERT REQ 0x32    "},
{0x33,"KEY TRANSFERT ACK 0x33    "},
{0x36,"ADDRESS REQUEST 0x36      "},
{0x38,"LAUNCH KEY TRANSFERT 0x38 "},
{0x3C,"CHALLENGE REQ 0x3C        "},
{0x3D,"CHALLENGE ACK 0x3D        "},
{0x50,"GET NAME REQ 0x50         "},
{0x51,"GET NAME ACK 0x51         "},
{0xfe,"ERROR 0xFE                "}
};

inline std::unordered_map<int, std::string> sDevicesType = {
{0b0000000000, "All"},
{0b0000000001, "Venetian blind"},
{0b0000000010, "Roller shutter"},
{0b0000000011, "Awning (External for windows)"},
{0b0000000100, "Window opener"},
{0b0000000101, "Garage opener"},
{0b0000000110, "Light"},
{0b0000000111, "Gate opener"},
{0b0000001000, "Rolling Door Opener"},
{0b0000001001, "Lock"},
{0b0000001010, "Blind"},
{0b0000001011, "Unk"},
{0b0000001100, "Beacon"},
{0b0000001101, "Dual Shutter"},
{0b0000001110, "Heating Temperature Interface"},
{0b0000001111, "On / Off Switch"},
{0b0000010000, "Horizontal Awning"},
{0b0000010001, "External Venetian Blind"},
{0b0000010010, "Louvre Blind"},
{0b0000010011, "Curtain track"},
{0b0000010100, "Ventilation Point"},
{0b0000010101, "Exterior heating"},
{0b0000010110, "Heat pump (Not currently supported)"},
{0b0000010111, "Intrusion alarm"},
{0b0000011000, "Swinging Shutter"}
};
/*
Protection Level
0 b[000] = Personal/Human: Most secure level. Will disable all categories (Level 0 to 7).
"Since consequences of misusing this level can deeply impact the system behaviour, and therefore the io-homecontrol image, it is mandatory for the manufacturer that wants to use this level of priority to receive an agreement from io-homecontrol®."

1 b[001] = End Product/Environment = (House) Goods Protection: Local Sensors
User Level = User Control
2 b[010] = Level 1 - High: Controllers have a higher level of priority than others.
3 b[011] = Level 2 - Default: Default for (Remote) Controllers. Send immediate Command(s).
Comfort Level = Automatic Control
4 b[100] = Level 1 - TBD
5 b[101] = Level 2 - TBD
6 b[110] = Level 3 - SAAC: Stand Alone Automatic Controls
7 b[111] = Level 4 - TBD (Default Channel: KLF100)*/
inline std::unordered_map<int, std::string> sAceiLevel = {
    {0,"Prot Human"},
    {1,"Prot Sensor"},
    {2,"User Controller"},
    {3,"User Remote"},
    {4,"Auto 1"},
    {5,"Auto 2"},
    {6,"Auto SAAC"},
    {7,"Auto 4"},
};
/*
Specifies what or who fired the command.

0x00 = User: Local - User button press on Actuator
0x01 = User: User Remote Control causing Actuator Action
0x02 = Sensor: Rain
0x03 = Sensor: Timer
0x04 = Security: SCD Controlling Actuator
0x05 = UPS: Uninterruptible Power Supply Unit
0x06 = SFC: Smart Function Controller
0x07 = LSC: Lifestyle Scenario Controller
0x08 = SAAC: Stand Alone Automatic Controls
0x09 = Sensor: Wind
0x0A = Unknown
0x0B = Automatic Cycle/External Access/Load Shedding (Managers for requiring a particular electric load shed)
0x0C = Sensor: Local Light
0x0D = Sensor: Unspecified Enviroment Sensor (Used with commands of Unknown Sensor for Protection of End-Product or house goods)
0x10 = Myself: Used when Actuator decides to move by itself (Generated by Actuator)
0xC8 = Unknown
0xFE = Automatic Cycle
0xFF = Emergency: Used in context with Emergency or Security commands. This command originator should never be disabled.
Note Typically only USER or SAAC are used.
*/
inline std::unordered_map<int, std::string> sOriginator = {
};

inline int get_address_class(address address) {// char (*uint_8)[3]) {

        if (address[0] != 0) { return 13; } // bits 0..8

        if (address[1] || (address[2] & 0xC0) != 0) { // bits 8..18
            switch (address[2] & 0x3F) { // bits 18..24
                case 0x3B: return  7;
                case 0x3C: return  8;
                case 0x3D: return  9;
                case 0x3E: return 10;
                case 0x3F: return 11;
                default: return 12;
            }
        }
        else { switch (address[2] & 0x3F) { // bits 18..24
            case 0x00: return 0;
            default: return 1;
            case 0x3B: return 2;
            case 0x3C: return 3;
            case 0x3D: return 4;
            case 0x3E: return 5;
            case 0x3F: return 6; // broadcast
        }
        }
    }

}
#endif