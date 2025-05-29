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

#ifndef IOHC_OBJECT_H
#define IOHC_OBJECT_H

#include <string>
#include <iohcPacket.h>
#include <iohcCryptoHelpers.h>

#define MAX_MANUFACTURER    13

namespace IOHC {

    struct iohcObject_t {
        address     node;
        uint8_t     actuator[2];
        uint8_t     flags;
        uint8_t     io_manufacturer;
        address     backbone;
    };

    class iohcObject {
        public:
            iohcObject();
            ~iohcObject();
            iohcObject(const address node, const address backbone, const uint8_t actuator[2], uint8_t manufacturer, uint8_t flags);
            explicit iohcObject(std::string serialized);

            address *getNode();
            address *getBackbone();
            std::tuple<uint16_t, uint8_t> getTypeSub();
            std::string serialize();
            void dump1W();
            void dump2W();
        protected:

        private:
            iohcObject_t object{};
            char man_id[MAX_MANUFACTURER][15] = {"VELUX", "Somfy", "Honeywell", "Hörmann", "ASSA ABLOY", "Niko", "WINDOW MASTER", "Renson", "CIAT", "Secuyou", "OVERKIZ", "Atlantic Group", "Other"};
    };
}
#endif