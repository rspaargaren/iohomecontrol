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

#ifndef IOHC_PACKET_H
#define IOHC_PACKET_H

#include <vector>
#include <string>

#include <board-config.h>
#include <cstring>

#if defined(RADIO_SX127X)
#include <SX1276Helpers.h>
#endif

#define RESET_AFTER_LAST_MSG_US         15000
#define MAX_FRAME_LEN                   32
#define IOHC_INBOUND_MAX_PACKETS        255     // Maximum Inbound packets buffer
#define IOHC_OUTBOUND_MAX_PACKETS       20      // Maximum Outbound packets

namespace IOHC {
    using address = uint8_t[3];

    struct CB1 {
        uint8_t MsgLen: 5; //1
        uint8_t Protocol: 1; //2
        uint8_t StartFrame: 1; //3
        uint8_t EndFrame: 1; //4
    };

    struct CB2 {
        uint8_t Version: 2;
        uint8_t Prio: 1;
        uint8_t Unk2: 1;
        uint8_t Unk3: 1;
        uint8_t LPM: 1;
        uint8_t Routed: 1;
        uint8_t Beacon: 1;
    };

    union CtrlByte1Union {
        uint8_t asByte;
        CB1 asStruct;
    };

    union CtrlByte2Union {
        uint8_t asByte;
        CB2 asStruct;
    };

    /// Common frame header
    struct _header {
        CtrlByte1Union CtrlByte1; //1
        CtrlByte2Union CtrlByte2; //1
        address target; //3
        address source; //3
        uint8_t cmd; //1
    };

    struct Acei {
        uint8_t isvalid: 1;
        uint8_t extended: 2;
        uint8_t service: 2;
        uint8_t level: 3;
    };

    union AceiUnion {
        uint8_t asByte;
        Acei asStruct;
    };

    //61 0110 0001
    //83 1000 0011
    inline void setAcei(AceiUnion&acei, uint8_t value) {
        acei.asByte = value;
        // acei.level = (value >> 5) & 0x07;
        // acei.service = (value >> 3) & 0x03;
        // acei.extended = (value >> 1) & 0x03;
        // acei.isvalid = value & 0x01;
    }

    struct _p0x00_16 {
        uint8_t origin;
        AceiUnion acei;
        uint8_t main[2];
        uint8_t fp1;
        uint8_t fp2;
        uint8_t data[2];
        uint8_t sequence[2];
        uint8_t hmac[6];
    };

    struct _p0x00_14 {
        uint8_t origin;
        AceiUnion acei;
        uint8_t main[2];
        uint8_t fp1;
        uint8_t fp2;
        uint8_t sequence[2];
        uint8_t hmac[6];
    };

    struct _p0x01_13 {
        uint8_t origin;
        AceiUnion acei;
        uint8_t main; //[2];
        uint8_t fp1;
        uint8_t fp2;
        uint8_t sequence[2];
        uint8_t hmac[6];
    };

    struct _p0x20_15 {
        uint8_t origin;
        AceiUnion acei;
        uint8_t main[2];
        uint8_t fp1;
        uint8_t fp2;
        uint8_t fp3;
        uint8_t sequence[2];
        uint8_t hmac[6];
    };

    struct _p0x20_13 {
        uint8_t origin;
        AceiUnion acei;
        uint8_t main[2];
        uint8_t fp1;
        uint8_t sequence[2];
        uint8_t hmac[6];
    };

    struct _p0x20_16 {
        uint8_t origin;
        AceiUnion acei;
        uint8_t main[2];
        uint8_t fp1;
        uint8_t fp2;
        uint8_t data[2];
        uint8_t sequence[2];
        uint8_t hmac[6];
    };

    struct _p0x2b {
        uint8_t actuator[2];
        address backbone;
        uint8_t manufacturer;
        uint8_t info;
        uint8_t tstamp[2];
    };
/// 1) 1W PAIR
    struct _p0x2e {
        uint8_t data;
        uint8_t sequence[2];
        uint8_t hmac[6];
    };
/// 3) 1W ADD**
    struct _p0x30 {
        uint8_t enc_key[16];
        uint8_t man_id;
        uint8_t data;
        uint8_t sequence[2];
    };

    union _msg {
        _p0x01_13 p0x01_13;
        _p0x00_14 p0x00_14;
        _p0x20_15 p0x20_15;
        _p0x20_13 p0x20_13;
        _p0x00_16 p0x00_16;
        _p0x20_16 p0x20_16;
        _p0x2b p0x29;
        _p0x2b p0x2b;
        _p0x2e p0x2e;
        _p0x30 p0x30;
        _p0x2e p0x39; // same format of 2e
    };

    struct _packet {
        _header header;
        _msg msg;
    };

    typedef union payload {
        uint8_t buffer[MAX_FRAME_LEN];
        _packet packet;
    } Payload;

    /* keep the size of variable lenght of data */
    typedef struct {
        uint8_t memorizedCmd;
        std::vector<uint8_t> memorizedData;
    } Memorize;

    // Frequencies
    inline unsigned long packetStamp = 0L;
    inline unsigned long relStamp = 0L;
    inline size_t lastSendCmd = 0xFF;
    /**
    Class implementing the IOHC packet received/sent, including some additional members useful to track/control Radio parameters and timings
    */
    class iohcPacket {
    public:
        iohcPacket() = default;
        ~iohcPacket() = default;

        Payload payload{};
        uint8_t buffer_length = 0;
        uint32_t frequency = CHANNEL2; // Both 1W & 2W
        unsigned long repeatTime = 0L;
        uint8_t repeat = 0;
        bool lock = false;
        unsigned long delayed = 0;

        double afc{}; // AFC freq correction applied
        uint8_t snr{}; // in dB
        float rssi{}; // -RSSI*2 of last packet received
        uint8_t lna{}; // LNA attenuation in dB

        void decode(bool verbosity = false);
        std::string decodeToString(bool verbosity = false);

        bool operator==(const iohcPacket &other) const {
            return (memcmp(this->payload.buffer, other.payload.buffer, 16) == 0);
        }
    };
}
#endif
