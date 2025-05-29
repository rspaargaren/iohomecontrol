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

#ifndef OTHER_2W_DEVICE_H
#define OTHER_2W_DEVICE_H

#include <string>
#include <vector>
#include <iohcDevice.h>
#include <interact.h>

#define OTHER_2W_FILE  "/Other2W.json"

/*
    Singleton class with a full implementation of a COZYTOUCH/KIZBOX/CONEXOON controller
    The type of the controller can be managed changing related value within its profile file (Cozy2W.json)
    Type can be multiple, as it would be for KLI310, KLI312 and KLI313
    Also, the address and private key can be configured within the same json file.
*/
namespace IOHC {
    enum class Other2WButton {
        discovery,
        getName,
        custom,
        custom60,
        discover28,
        discover2A,
        fake0,
        ack,
        pairMode,
        checkCmd,
    };

    class iohcOtherDevice2W : public iohcDevice {
    public:
        static iohcOtherDevice2W *getInstance();
        ~iohcOtherDevice2W() override = default;

        // Put that in json
        address gateway/*[3]*/ = {0xba, 0x11, 0xad};
        address master_from/*[3]*/ = {0x47, 0x77, 0x06}; // It's the new heater kitchen Address From
        address master_to/*[3]*/ = {0x48, 0x79, 0x02}; // It's the new heater kitchen Address To
        address slave_from/*[3]*/ = {0x8C, 0xCB, 0x30}; // It's the new heater kitchen Address From
        address slave_to/*[3]*/ = {0x8C, 0xCB, 0x31}; // It's the new heater kitchen Address To

        Memorize memorizeOther2W; //2W only

        //            bool isFake(address nodeSrc, address nodeDst) override;
        void cmd(Other2WButton cmd, Tokens *data);
        bool load() override;
        bool save() override;
        void initializeValid();
        void scanDump();
        std::map<uint8_t, int> mapValid;
//        void scanDump() override {}

        static void forgePacket(iohcPacket *packet, const std::vector<uint8_t> &vector, size_t typn);

    private:
        iohcOtherDevice2W();
        static iohcOtherDevice2W *_iohcOtherDevice2W;

    protected:
        //            unsigned long relStamp;
        //            uint8_t source_originator[3] = {0};
        address _node{};
        address _dst{};
        std::string _type;
        //            uint16_t _sequence;
        //            uint8_t _key[16];
        //            std::vector<uint16_t> _type;
        //            uint8_t _manufacturer;

        //            IOHC::iohcPacket *packets2send[2]; //[25];
        // std::array<iohcPacket*, 25> packets2send{};
        std::vector<iohcPacket *> packets2send{};
        //            IOHC::iohcRadio *_radioInstance;
    };
}
#endif
