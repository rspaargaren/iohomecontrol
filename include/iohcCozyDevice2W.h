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

#ifndef COZY_2W_DEVICE_H
#define COZY_2W_DEVICE_H

#include <interact.h>
#include <string>
#include <iohcRadio.h>
#include <iohcDevice.h>
#include <map>
#include <vector>

#define COZY_2W_FILE  "/Cozy2W.json"

namespace IOHC {
    /// The `enum class DeviceButton` is defining an enumeration type with different button commands that can be used for a specific device.
    /// Each of the identifiers within the enumeration represents a specific button command that can be sent to the device.
    /// This enumeration provides a convenient way to refer to these commands using descriptive names rather than raw integer values.
    enum class DeviceButton {
        powerOn,
        setTemp,
        setMode,
        setPresence,
        setWindow,
        midnight,
        associate,
    };

    class iohcCozyDevice2W : public iohcDevice {
    public:
        static iohcCozyDevice2W *getInstance();
        ~iohcCozyDevice2W() override = default;

        Memorize memorizeSend; //2W only

        // Put that in json
        address gateway/*[3]*/ = {0xba, 0x11, 0xad};
        address master_from/*[3]*/ = {0x47, 0x77, 0x06}; // It's the new heater kitchen Address From
        address master_to/*[3]*/ = {0x48, 0x79, 0x02}; // It's the new heater kitchen Address To
        address slave_from/*[3]*/ = {0x8C, 0xCB, 0x30}; // It's the new heater kitchen Address From
        address slave_to/*[3]*/ = {0x8C, 0xCB, 0x31}; // It's the new heater kitchen Address To

        typedef std::array<uint8_t, 3> Address;
        std::vector<Address> addresses = {
            Address{0x48, 0x79, 0x02}, //Master_to
            Address{0x8C, 0xCB, 0x31}, //Slave_to
            // Address{0x47, 0x77, 0x06},
            // Address{0x8C, 0xCB, 0x30},
            // Address{0xba, 0x11, 0xad},
        };

        bool verbosity = true;

        bool isFake(address nodeSrc, address nodeDst) override;
        void cmd(DeviceButton cmd, Tokens *data);
        bool load() override;
        bool save() override;
        static void forgePacket(iohcPacket *packet, const std::vector<uint8_t> &vector);

    private:
        iohcCozyDevice2W();
        static iohcCozyDevice2W *_iohcCozyDevice2W;

    protected:
        struct device {
            address _node{};
            address _dst{};
            std::string _type;
            std::string _description;
        };

        std::vector<device> devices;
        std::vector<iohcPacket *> packets2send{};
    };
}
#endif
