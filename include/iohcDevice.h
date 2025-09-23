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

#ifndef IOHC_DEVICE_H
#define IOHC_DEVICE_H

#include <vector>
#include <iohcPacket.h>
#include <iohcRadio.h>
#include <map>

namespace IOHC {
    /* Base classe for all devices type */
    class iohcDevice {
    public:
        // static iohcDevice *getInstance();
        virtual ~iohcDevice() = default;
        virtual bool isFake(address, address);
        virtual bool isHome(address, address);

//        std::map<uint8_t, int> mapValid;
//        virtual void initializeValid();
//        virtual void scanDump();

        bool verbosity{};
        virtual bool load() = 0;
        virtual bool save() = 0;
//        virtual void scanDump() = 0;

        enum commandId {
            DO_NOTHING = 0xFF,
            RECEIVED_WRITE_PRIVATE_0x20 = 0x20, SEND_WRITE_PRIVATE_0x20 = 0x20,
            RECEIVED_PRIVATE_ACK_0x21 = 0x21, SEND_PRIVATE_ACK_0x21 = 0x21,
            RECEIVED_DISCOVER_0x28 = 0x28, SEND_DISCOVER_0x28 = 0x28,
            RECEIVED_DISCOVER_ANSWER_0x29 = 0x29, SEND_DISCOVER_ANSWER_0x29 = 0x29,
            RECEIVED_DISCOVER_REMOTE_0x2A = 0x2A, SEND_DISCOVER_REMOTE_0x2A = 0x2A,
            RECEIVED_DISCOVER_REMOTE_ANSWER_0x2B = 0x2B, SEND_DISCOVER_REMOTE_ANSWER_0x2B = 0x2B,
            RECEIVED_DISCOVER_ACTUATOR_0x2C = 0x2C, SEND_DISCOVER_ACTUATOR_0x2C = 0x2C,
            RECEIVED_DISCOVER_ACTUATOR_ACK_0x2D = 0x2D, SEND_DISCOVER_ACTUATOR_ACK_0x2D = 0x2D,
            RECEIVED_UNKNOWN_0x2E = 0x2E, SEND_UNKNOWN_0x2E = 0x2E,
            RECEIVED_ASK_CHALLENGE_0x31 = 0x31, SEND_ASK_CHALLENGE_0x31 = 0x31,
            RECEIVED_KEY_TRANSFERT_0x32 = 0x32, SEND_KEY_TRANSFERT_0x32 = 0x32,
            RECEIVED_KEY_TRANSFERT_ACK_0x33 = 0x33, SEND_KEY_TRANSFERT_ACK_0x33 = 0x33,
            RECEIVED_ADDRESS_REQUEST_0x36 = 0x36, SEND_ADDRESS_REQUEST_0x36 = 0x36,
            RECEIVED_LAUNCH_KEY_TRANSFERT_0x38 = 0x38, SEND_LAUNCH_KEY_TRANSFERT_0x38 = 0x38,
            RECEIVED_CHALLENGE_REQUEST_0x3C = 0x3c, SEND_CHALLENGE_REQUEST_0x3C = 0x3c,
            RECEIVED_CHALLENGE_ANSWER_0x3D = 0x3d, SEND_CHALLENGE_ANSWER_0x3D = 0x3d,
            RECEIVED_GET_NAME_0x50 = 0x50, SEND_GET_NAME_0x50 = 0x50,
            RECEIVED_GET_NAME_ANSWER_0x51 = 0x51, SEND_GET_NAME_ANSWER_0x51 = 0x51,
            RECEIVED_STATUS_0xFE = 0xfe, SEND_STATUS_0xFE = 0xfe,
        };

        typedef struct _dev {
            address _node{};
            address _dst{};
            std::string _type;
            std::string _description;
            address _associated{};

            _dev() = default;

            _dev(address source, address target) {
                memcpy(_node, source, sizeof(address));
                memcpy(_dst, target, sizeof(address));
                _type = "";
                _description = "";
            }

            friend bool operator==(const _dev &lhs, const _dev &rhs) {
                return memcmp(lhs._node, rhs._node, sizeof(address)) == 0 && memcmp(lhs._dst, rhs._dst, sizeof(address)) == 0;
            }

            _dev(const _dev& other) {
                memcpy(_node, other._node, sizeof(address));
                memcpy(_dst, other._dst, sizeof(address));
                _description = other._description;
                memcpy(_associated, other._associated, sizeof(address));
            }
            _dev& operator=(const _dev& other) {
                if (this != &other) {
                    memcpy(_node, other._node, sizeof(address));
                    memcpy(_dst, other._dst, sizeof(address));
                    _description = other._description;
                    memcpy(_associated, other._associated, sizeof(address));
                }
                return *this;
            }
        } Device;

    protected:
        bool Fake = false;
        bool Home = false;

        std::vector<iohcPacket *> packets2send{};
        iohcRadio *_radioInstance{};

    private:
        // static iohcDevice *_iohcDevice;
    };
}
#endif
