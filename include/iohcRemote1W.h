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

#ifndef IOHC_1W_DEVICE_H
#define IOHC_1W_DEVICE_H

#include <interact.h>
#include <iohcDevice.h>
#include <vector>

#define IOHC_1W_REMOTE  "/1W.json"

/*
    Singleton class with a full implementation of a VELUX KLIxxx controller
    The type of the controller can be managed changing related value within its profile file (1W.json)
    Type can be multiple, as it would be for KLI310, KLI312 and KLI313
    Also, the address and private key can be configured within the same json file.
*/
namespace IOHC {
    enum class RemoteButton {
        Pair,
        Add,
        Remove,
        Open,
        Close,
        Stop,
        Vent,
        ForceOpen,
        Mode1, Mode2, Mode3, Mode4
    };

    class iohcRemote1W : public iohcDevice {
    public:
        static iohcRemote1W* getInstance();
        ~iohcRemote1W() override = default;

        void cmd(RemoteButton cmd, Tokens* data);
        bool load() override;
        bool save() override;
//        void scanDump() override { }

        static void forgePacket(iohcPacket* packet, uint16_t typn);

    private:
        iohcRemote1W();

        static iohcRemote1W* _iohcRemote1W;

    protected:
        int8_t target[3];
        struct remote {
            address node{};
            uint16_t sequence{};
            uint8_t key[16]{};
            std::vector<uint8_t> type{};
            uint8_t manufacturer{};
            std::string description;
        };

        std::vector<remote> remotes;

        std::vector<iohcPacket *> packets2send{};

    };
}
#endif
