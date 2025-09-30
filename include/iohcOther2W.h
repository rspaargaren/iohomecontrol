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

#include <algorithm>
#include <string>
#include <vector>
#include <iohcDevice.h>
#include <iohcSystemTable.h>
#include <tokens.h>

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

    class iohcOther2W : public iohcDevice {

    public:
        static iohcOther2W *getInstance();
        ~iohcOther2W() override = default;

        static std::string extractAndNormalizeName(const uint8_t* buffer, int offset, int length) {
            std::string result;
            result.reserve(length);

            for (int i = 0; i < length; i++) {
                uint8_t byte = buffer[offset + i];

                if (byte == 0) {
                    continue;
                } else if (byte >= 32 && byte <= 126) {
                    result.push_back(std::toupper(static_cast<char>(byte)));
                } else {
                    result.push_back('_');
                }
            }

            return result.empty() ? "UNKNOWN" : result;
        }

        // Put that in JSON
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

        // bool save() override;
        void initializeValid();
        void scanDump();
        std::unordered_map<uint8_t, int> mapValid;
//        void scanDump() override {}

        static void forgeAnyWPacket(iohcPacket *packet, const std::vector<uint8_t> &vector, size_t typn);


        std::vector<Device> devices;
        std::string getDescription(const Device &device)    {
            const auto it = std::ranges::find(devices, device);
            if ( it != devices.end())
                return it->_description;
            return "";
        }
        bool addOrUpdateDevice(const Device &device) {
            const auto it = std::ranges::find(devices, device);
            if (it != devices.end()) {
                bool modified = false;
                if (it->_description != device._description && device._description != "NONAME") {
                    it->_description = device._description;
                    modified = true;
                }
                // Si c'est la gateway, on n'enregistre pas, on ne souhaite que la telecommande 1W associée
                if (memcmp(device._associated, device._node, sizeof(address)) != 0) {
                    memcpy(it->_associated, device._associated, sizeof(address));
                    modified = true;
                }
                if (it->_type != device._type) {
                    it->_type = device._type;
                    modified = true;
                }
                return modified;
            }
            devices.push_back(device);
            return true;
        }

    private:
        iohcOther2W();
        static iohcOther2W *_iohcOtherDevice2W;

    protected:

        std::vector<iohcPacket *> packets2send{};

    };
}
#endif
