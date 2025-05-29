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

#include <cstring>
#include <iohcObject.h>

#include <utility>

namespace IOHC {
    iohcObject::iohcObject() = default;
    iohcObject::~iohcObject() = default;

    iohcObject::iohcObject(const address node, const address backbone, const uint8_t actuator[2], uint8_t manufacturer, uint8_t flags) {
        for (uint8_t i=0; i<3; i++) {
            object.node[i] = node[i];
            object.backbone[i] = backbone[i];
            if (i<2)
                object.actuator[i] = actuator[i];
        }

        object.flags = flags;
        object.io_manufacturer = manufacturer;
    }

    iohcObject::iohcObject(std::string serialized) {
        uint8_t eval[sizeof(object)];

        hexStringToBytes(std::move(serialized), eval);
        memcpy(object.node, eval, sizeof(object));
    }
    
    address* iohcObject::getNode() {
        return reinterpret_cast<address*>(object.node); // ((address *)object.node);
    }

    address *iohcObject::getBackbone() {
        return reinterpret_cast<address*>(object.backbone); // ((address *)object.backbone);
    }

    std::tuple<uint16_t, uint8_t> iohcObject::getTypeSub() {
        return std::make_tuple(((object.actuator[0]<<8) + (object.actuator[1]))>>6, object.actuator[1] & 0x3f);
    }

    std::string iohcObject::serialize() {
        return (bytesToHexString(object.node, sizeof(iohcObject_t)));
    }
    
    void iohcObject::dump1W() {
        printf("Address: %2.2x%2.2x%2.2x, ", object.node[0], object.node[1], object.node[2]);
        printf("Backbone: %2.2x%2.2x%2.2x, ", object.backbone[0], object.backbone[1], object.backbone[2]);
        printf("Typ/Sub: %2.2x%2.2x, ", object.actuator[0], object.actuator[1]);
        printf("lp: %u, io: %u, rf: %u, ta: %2.2ums, ", object.flags&0x03?1:0, object.flags&0x04?1:0, object.flags&0x08?1:0, 5<<(object.flags>>6));
        printf("%s\n", (object.io_manufacturer <= MAX_MANUFACTURER)?man_id[object.io_manufacturer-1]:man_id[MAX_MANUFACTURER-1]);
    }
    void iohcObject::dump2W() {
//        Serial.printf("Address: %2.2x%2.2x%2.2x, ", object.node[0], object.node[1], object.node[2]);
//        Serial.printf("Backbone: %2.2x%2.2x%2.2x, ", object.backbone[0], object.backbone[1], object.backbone[2]);
//        Serial.printf("Typ/Sub: %2.2x%2.2x, ", object.actuator[0], object.actuator[1]);
//        Serial.printf("lp: %u, io: %u, rf: %u, ta: %2.2ums, ", object.flags&0x03?1:0, object.flags&0x04?1:0, object.flags&0x08?1:0, 5<<(object.flags>>6));
//        Serial.printf("%s\n", (object.io_manufacturer <= MAX_MANUFACTURER)?man_id[object.io_manufacturer-1]:man_id[MAX_MANUFACTURER-1]);
    }
}