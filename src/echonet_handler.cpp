/**
 *  Copyright (c) 2025. CRIDP https://github.com/cridp

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

          http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Created by Pascal on 15/09/2025.
*/
#include <user_config.h>

#if defined(UECHO)
#include "echonet_handler.hpp"

#include <iostream>
#include <map>
#include <sstream>

// ECHONET Lite est une variable globale, Cela garantit qu'elle n'est pas détruite à la fin de la fonction setup().
uecho::Controller ctrl;

void prepare_virtual_devices() {
     // To Examples: create a simple light device
     auto* light = new uecho::Object();
     light->setCode(0x029001); // Lighting device class, instance 1
     light->setDescription("uEcho-Light");

     // Add properties with their attributes (Read/Write/Anno)
     // This is crucial for other controllers to understand what they can do.
     auto* opStatusProp = new uecho::Property(0x80, "Operating status", uecho::PropertyAttr::Read | uecho::PropertyAttr::Write | uecho::PropertyAttr::Anno, {0x30}); // On
     light->addProperty(opStatusProp);
     light->addProperty(new uecho::Property(0x81, "Location", uecho::PropertyAttr::Read | uecho::PropertyAttr::Write | uecho::PropertyAttr::Anno, {0x60}));
     light->addProperty(new uecho::Property(0x88, "Fault status", uecho::PropertyAttr::Read | uecho::PropertyAttr::Anno, {0x42})); // No fault
     light->addProperty(new uecho::Property(0x8A, "Manufacturer", uecho::PropertyAttr::Read, {0xba, 0x11, 0xad})); // Use a valid code
     light->addProperty(new uecho::Property(0x82, "Version", uecho::PropertyAttr::Read, {0x01, 0x01, 0x01, 0x00})); // Ver 1.1, standard message format

     // Set a listener for property write requests to handle commands
     // For a General Lighting device (0x0290), the ON/OFF command is sent to the Operating Status property (0x80).
     // light->setPropertyRequestListener([lightOnOffProp](const uecho::Message* msg) {
     light->setPropertyRequestListener([light, opStatusProp](const uecho::Message* msg) {
          for (const auto& prop : *msg->getProperties()) {
             // if (prop->getCode() == 0xB0) { // Light ON/OFF setting
             if (prop->getCode() == 0x80) { // Listen for Operating Status commands
                  if (prop->getData().size() > 0) {
                      uecho::byte val = prop->getData()[0];
                     bool stateChanged = false;
                     if (val == 0x30) { // ON
                         std::cout << "uEcho COMMAND: Light turned ON" << std::endl;
                         stateChanged = opStatusProp->setData({0x30});
                      } else if (val == 0x31) { // OFF
                         std::cout << "uEcho COMMAND: Light turned OFF" << std::endl;
                         stateChanged = opStatusProp->setData({0x31});
                      }
                      // If the state changed and the property is announceable, send a notification.
                      if (stateChanged && opStatusProp->isAnnounceable()) {
                          uecho::Message infMsg;
                          infMsg.setESV(uecho::Esv::Notification);
                          infMsg.setSourceObjectCode(light->getCode());
                          // Announce to all Node Profiles on the network
                          infMsg.setDestinationObjectCode(uecho::NodeProfileObject);

                          auto* infProp = new uecho::Property(*opStatusProp);
                          infMsg.addProperty(infProp);

                          ctrl.announceMessage(&infMsg);
                      }

                 }
             }
         }
     });

     ctrl.getLocalNode()->addObject(light);
}
void print_node(uecho::Node* node) {
    std::ostringstream ss;
    ss << "[" << node->getAddress() << "]";
    for (auto obj : *node->getObjects()) {
        if (obj->getDescription())
            ss << "  - " << obj->getDescription() << " (" << std::hex << obj->getCode() << ")";
        else
            ss << "  - " << std::hex << " (" << std::hex << obj->getCode() << ")";
        for (auto prop : *obj->getProperties()) {
            ss << "    * EPC " << std::hex << static_cast<int>(prop->getCode()) << " " << prop->getName() << " = ";
            const auto& propData = prop->getData();
            for (auto val : propData) {
                ss << std::hex << static_cast<int>(val) << " ";
            }
        }
    }
    ss << std::dec << std::endl;
    std::cout << ss.str();
}

void print_found_devices(bool verbose) {
    std::cout << "Found " << ctrl.getNodes()->size() << " nodes." << std::endl;

    // The database is now populated once in prepare_uecho() before the controller starts.
    auto &db = uecho::standard::StandardDatabase::getSharedInstance();
    auto localNode = ctrl.getLocalNode();

    // Create the manufacturer request message once, outside the loop, for efficiency.
    uecho::Message man_req_msg;
    man_req_msg.setESV(uecho::Esv::ReadRequest);
    man_req_msg.setDestinationObjectCode(uecho::NodeProfileObject);
    man_req_msg.setSourceObjectCode(uecho::NodeProfileObject);
    auto man_prop = new uecho::Property();
    man_prop->setCode(0x8A); // Manufacturer Code
    man_req_msg.addProperty(man_prop);
    // The man_req_msg destructor will handle deleting man_prop.
    // uecho::Message man_res_msg; // Create a fresh response message for each request.

    for (auto node : *ctrl.getNodes()) {
        if (node == localNode) continue;
        // if (!verbose) {
            // print_node(node);
            // continue;
        // }

        // const char* manufactureName = UNKNOWN_STRING;
        // uecho::Message man_res_msg; // Create a fresh response message for each request.
        // if (ctrl.postMessage(node, &man_req_msg, &man_res_msg)) {
        //    if (man_res_msg.getPropertyCount() > 0) {
        //         // Assuming getProperties() returns a pointer to a custom::list class.
        //         // first() is the new C++ method to get the first element.
        //         if (auto res_prop = man_res_msg.getProperties()->first()) { // Good practice to check if the pointer is valid
        //             if (std::vector<byte> prop_data = res_prop->getData(); prop_data.size() >= 3) {
        //                 int man_code = (prop_data[0] << 16) + (prop_data[1] << 8) + prop_data[2];
        //                 std::cout << " Search Manufacturer code: " << man_code << std::endl;
        //                 if (uecho::Manufacture* man = db->getManufacture(man_code)) {
        //                     manufactureName = man->getName().c_str();
        //                 }
        //             }
        //         }
        //     }
        // }
        // delete man_prop;
        // std::cout << "  Manufacturer: " << manufactureName << std::endl;

        // --- Querying Object Capabilities ---
        // Now that we have the node, let's inspect its objects to find out what they can do.
        uecho::Message prop_map_req; uecho::Message prop_map_res;
        prop_map_req.setESV(uecho::Esv::ReadRequest); // 0x62 (98)
        // prop_map_req.setDestinationObjectCode(obj->getCode());
        prop_map_req.setSourceObjectCode(uecho::NodeProfileObject);
        auto prop_map_prop = new uecho::Property();
        prop_map_prop->setCode(0x9F); // Get property map
        prop_map_req.addProperty(prop_map_prop);

        for (auto obj : *(node->getObjects())) {
            // Skip the node profile object, as we are interested in device objects.
            if (obj->getCode() == uecho::NodeProfileObject) continue;

            // Use the database to identify the object class.
            auto class_info = db.getObject(obj->getGroupCode(), obj->getClassCode());

            const char* className = class_info ? class_info->getDescription() : UNKNOWN_STRING;
            std::cout << "    - Found device: " << className << " (" << std::hex << obj->getCode() << ")" << std::endl;
            obj->setDescription(className);

           // For devices, we know how to handle (like a Home Air Conditioner), query their capabilities.
           // Create a request to get the object's property map (EPC 0x9F)

           // prop_map_req.setESV(uecho::Esv::ReadRequest); // 0x62 (98)
           prop_map_req.setDestinationObjectCode(obj->getCode());
           // prop_map_req.setSourceObjectCode(uecho::NodeProfileObject);
           // auto prop_map_prop = new uecho::Property();
           // prop_map_prop->setCode(0x9F); // Get property map
           // prop_map_req.addProperty(prop_map_prop);

           // uecho::Message prop_map_res;

       // if (ctrl.postMessage(node, &prop_map_req, &prop_map_res)) {
            // std::cout << "      ctrl.postMessage" << std::endl;
            // if (prop_map_res.getPropertyCount() > 0) {
               // std::cout << "      prop_map_res.getProperties() " << prop_map_res.getPropertyCount() << std::endl;
               // uecho::Property *res_prop = prop_map_res.getProperties()->first();
               // for (auto res_prop : *prop_map_res.getProperties())
                   // std::cout << "      res_prop 0x" << std::hex << static_cast<int>(res_prop->getCode()) << std::endl;
               // if (/*res_prop &&*/ res_prop->getCode() == 0x9F) {
               //     std::cout << "      Supported properties (Get Map):" << std::endl;
               //     const auto& map_data = res_prop->getData();
               //     // Note: This handles the simple list format (<=16 properties).
               //     // A full implementation would also need to parse the 17-byte bitmap format.
               //     if (!map_data.empty()) {
               //         int prop_count = map_data[0];
               //         for (int i = 1; i <= prop_count && i < map_data.size(); ++i) {
               //             byte prop_code = map_data[i];
               //             // const char* prop_name = class_info ? class_info->getPropertyName(prop_code).c_str() : UNKNOWN_STRING;
               //             // std::cout << "        - EPC 0x" << std::hex << static_cast<int>(prop_code) << " (" << prop_name << ")" << std::endl;
               //             std::cout << "        - EPC 0x" << std::hex << static_cast<int>(prop_code) <<  std::endl;
               //         }
               //     }
               // }
            // }
        // }
       }
    }

    // --- Memory Optimization ---
    // Now that we have identified all manufacturers, we can release the memory
    // used by the manufacturer database to save RAM.
    // db->clearManufactures();
    // std::cout << "Manufacturer database cleared to save memory." << std::endl;
}

void setup_uEcho() {
    // #include <WiFiMultiCast.h>
    // #include <WiFiUdp.h>
    // WiFiUDP udp;
    // const uint8_t udpState = udp.beginMulticast(IPAddress(224, 0, 23, 0), 3610);
    // ets_printf("Multicast started for Echonet Lite state: %d\n", udpState);

    ets_printf("Starting uEcho Controller...\n");

    // 1. Populate the database first. This is critical to prevent memory corruption.
    // If the listener receives a message and tries to get a property name before
    // the database is populated, it can lead to crashes.
    // ets_printf("Populating ECHONET Lite standard database...\n");
    auto &db = uecho::standard::StandardDatabase::getSharedInstance();
    db.populate();

    // Prepare virtual devices before starting the controller
    prepare_virtual_devices();

    // 3. Start the controller's network task.
    if (!ctrl.start()) {
        std::cerr << "Could not start controller" << std::endl;
        return;
    }

    // ctrl.getLocalNode()->getServer()->getUdpServer()->stop();
    // 4. Send the discovery message.
    ets_printf("Searching for ECHONET Lite devices...\n");
    ctrl.search();
    // 5. Wait for responses. The listener will be called automatically.
    ets_printf("Waiting for responses...\n");
    uecho::util::Wait(5000);

    // 6. (Optional) After discovery, perform additional actions on found nodes.
    print_found_devices(true);

    // 2. Set the listener that will handle discovered nodes.
    ctrl.setNodeListener([](uecho::Node* node, uecho::NodeStatus status, const uecho::Message* msg) {
        // if (status == uecho::NodeStatus::Updated) {
            // ets_printf("--- Node Updated --- ");
            print_node(node);
        // }
    });
}

std::map<const uecho::byte, const char*> propNameCache;

// void uecho_print_ctrl_multicastmessages(uEchoController* ctrl, uEchoMessage* msg) {
    // digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
    // size_t opc = uecho_message_getopc(msg);
    // size_t esv = uecho_message_getesv(msg);
    // Happen when a controller requests a propertyesv = uecho_message_getesv(msg)
    // ets_printf("CTRL -> SRCADDR %s EDH1 %1X EDH2 %1X ID %02X SRCOBJ %03X DSTOBJ %03X ESV %02X (%s) %ld OPC",
    //     uecho_message_getsourceaddress(msg).c_str(),
    //     uecho_message_getehd1(msg),
    //     uecho_message_getehd2(msg),
    //     uecho_message_gettid(msg),
    //     uecho_message_getsourceobjectcode(msg),
    //     uecho_message_getdestinationobjectcode(msg),
    //     esv, esv == uEchoEsvReadRequest ? "READ" :
    //     esv == uEchoEsvWriteRequest ? "WRITE" :
    //     esv == uEchoEsvWriteResponse ? "WRITE RESPONSE" :
    //     esv == uEchoEsvReadResponse ? "READ RESPONSE" :
    //     esv == uEchoEsvWriteReadRequest ? "WRITE READ" :
    //     esv == uEchoEsvWriteReadResponse ? "WRITE READ RESPONSE" :
    //     esv == uEchoEsvNotification ? "NOTIFICATION" :
    //     esv == uEchoEsvNotificationRequest ? "NOTIFICATION REQUEST" :
    //     esv == uEchoEsvWriteRequestResponseRequired ? "WRITE REQUEST RESPONSE REQUIRED" :
    //     esv == uEchoEsvWriteRequestResponseRequiredError ? "WRITE REQUEST RESPONSE REQUIRED ERROR" :
    //     esv == uEchoEsvReadRequestError ? "READ REQUEST ERROR" :
    //     esv == uEchoEsvWriteRequestError ? "WRITE REQUEST ERROR" :
    //     esv == uEchoEsvNotificationResponse ? "NOTIFICATION RESPONSE" :
    //     esv == uEchoEsvNotificationResponseRequired ? "NOTIFICATION RESPONSE REQUIRED" : "",
    //     opc
    //     );
    //
    // for (size_t n = 0; n < opc; n++) {
    //     uEchoProperty *prop = uecho_message_getproperty(msg, n);
    //     ets_printf(" PROP %02X (%s)", uecho_property_getcode(prop), uecho_property_getname(prop));
    // }
//     ets_printf("CTRL -> SRCADDR %s SRCOBJ %03X DSTOBJ %03X ESV %02X (%s) %ld OPC",
//         uecho_message_getsourceaddress(msg).c_str(),
//         // uecho_message_getehd1(msg),
//         // uecho_message_getehd2(msg),
//         // uecho_message_gettid(msg),
//         uecho_message_getsourceobjectcode(msg),
//         uecho_message_getdestinationobjectcode(msg),
//         esv, esv == uEchoEsvReadRequest ? "READ" :
//         esv == uEchoEsvWriteRequest ? "WRITE" :
//         esv == uEchoEsvWriteResponse ? "WRITE RESPONSE" :
//         esv == uEchoEsvReadResponse ? "READ RESPONSE" :
//         esv == uEchoEsvWriteReadRequest ? "WRITE READ" :
//         esv == uEchoEsvWriteReadResponse ? "WRITE READ RESPONSE" :
//         esv == uEchoEsvNotification ? "NOTIFICATION" :
//         esv == uEchoEsvNotificationRequest ? "NOTIFICATION REQUEST" :
//         esv == uEchoEsvWriteRequestResponseRequired ? "WRITE REQUEST RESPONSE REQUIRED" :
//         esv == uEchoEsvWriteRequestResponseRequiredError ? "WRITE REQUEST RESPONSE REQUIRED ERROR" :
//         esv == uEchoEsvReadRequestError ? "READ REQUEST ERROR" :
//         esv == uEchoEsvWriteRequestError ? "WRITE REQUEST ERROR" :
//         esv == uEchoEsvNotificationResponse ? "NOTIFICATION RESPONSE" :
//         esv == uEchoEsvNotificationResponseRequired ? "NOTIFICATION RESPONSE REQUIRED" : "",
//         opc
//         );
//     for (size_t n = 0; n < opc; n++) {
//         uEchoProperty *prop = uecho_message_getproperty(msg, n);
//
//         if ( propNameCache.contains(uecho_property_getcode(prop))) {
//             ets_printf(" PROP %02X (%s)", uecho_property_getcode(prop), propNameCache.at(uecho_property_getcode(prop)));
//         } else {
//             ets_printf(" PROP %02X (%s)", uecho_property_getcode(prop), "UNCACHED");
//             // propNameCache.emplace(uecho_property_getcode(prop), uecho_property_getname(prop));
//          }
//     }
//     ets_printf("\n");
//     digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
// }

// void uecho_print_node_multicastmessages(/*uEchoNode* node,*/ uEchoMessage* msg) {
//     size_t opc = uecho_message_getopc(msg);
//     size_t esv = uecho_message_getesv(msg);
//     // Happen when a controller requests a propertyesv = uecho_message_getesv(msg)
//     ets_printf("NODE -> SRCADDR %s EDH1 %1X EDH2 %1X ID %02X SRCOBJ %03X DSTOBJ %03X ESV %02X (%s) %ld OPC",
//         uecho_message_getsourceaddress(msg).c_str(),
//         uecho_message_getehd1(msg),
//         uecho_message_getehd2(msg),
//         uecho_message_gettid(msg),
//         uecho_message_getsourceobjectcode(msg),
//         uecho_message_getdestinationobjectcode(msg),
//         esv, esv == uEchoEsvReadRequest ? "READ" :
//         esv == uEchoEsvWriteRequest ? "WRITE" :
//         esv == uEchoEsvWriteResponse ? "WRITE RESPONSE" :
//         esv == uEchoEsvReadResponse ? "READ RESPONSE" :
//         esv == uEchoEsvWriteReadRequest ? "WRITE READ" :
//         esv == uEchoEsvWriteReadResponse ? "WRITE READ RESPONSE" :
//         esv == uEchoEsvNotification ? "NOTIFICATION" :
//         esv == uEchoEsvNotificationRequest ? "NOTIFICATION REQUEST" :
//         esv == uEchoEsvWriteRequestResponseRequired ? "WRITE REQUEST RESPONSE REQUIRED" :
//         esv == uEchoEsvWriteRequestResponseRequiredError ? "WRITE REQUEST RESPONSE REQUIRED ERROR" :
//         esv == uEchoEsvReadRequestError ? "READ REQUEST ERROR" :
//         esv == uEchoEsvWriteRequestError ? "WRITE REQUEST ERROR" :
//         esv == uEchoEsvNotificationResponse ? "NOTIFICATION RESPONSE" :
//         esv == uEchoEsvNotificationResponseRequired ? "NOTIFICATION RESPONSE REQUIRED" : "",
//         opc
//         );
//
//     for (size_t n = 0; n < opc; n++) {
//         uEchoProperty *prop = uecho_message_getproperty(msg, n);
//         if ( propNameCache.contains(uecho_property_getcode(prop))) {
//             ets_printf(" PROP %02X (%s)", uecho_property_getcode(prop), propNameCache.at(uecho_property_getcode(prop)));
//         } else {
//         ets_printf(" PROP %02X (%s)", uecho_property_getcode(prop), "UNCACHED");
//         // propNameCache.emplace(uecho_property_getcode(prop), uecho_property_getname(prop));
//         }
//     }
//
//     ets_printf("\n");
// }

// void uecho_lighting_node_messagelistener(uEchoNode* obj, uEchoMessage* msg) {
//     uecho_print_multicastmessages(NULL, msg);
// }

// bool uecho_lighting_propertyrequesthandler(uEchoProperty* prop, uEchoEsv esv, size_t pdc, byte* edt) {
//     digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
//     printf("ESV = %02X : %02X (%ld), ", esv, uecho_property_getcode(prop), pdc);
//
//     if ((pdc != 1) || !edt) {
//         printf("Bad Request\n");
//         return false;
//     }
//
//     byte status = edt[0];
//
//     // TODO : Set the status to hardware
//     switch (status) {
//     case LIGHT_PROPERTY_POWER_ON:
//         printf("POWER = ON\n");
//         break;
//     case LIGHT_PROPERTY_POWER_OFF:
//         printf("POWER = OFF\n");
//         break;
//     default:
//         printf("POWER = %02X\n", status);
//         break;
//     }
//
//     digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
//     return true;
// }

/// @see
/// https://github.com/futomi/node-echonet-lite/blob/master/EDT-02.md#class-90
// uEchoObject* uecho_light_new() {
//
//     byte prop[32];
//
//     uEchoObject *obj = uecho_device_new();
//     // Mono functional lighting class
//     uecho_object_setcode(obj, LIGHT_OBJECT_CODE);
//
//     uecho_object_setname(obj, "uEchoLight");
//     uecho_device_setinstallationlocation(obj, LocationGarden);
//     // Set your manufacture code
//     uecho_object_setmanufacturercode(obj, 0x000105);
//
//     // Operation status property EPC: 0x80
//     uecho_object_setproperty(obj, LIGHT_PROPERTY_POWER_CODE, uEchoPropertyAttrReadWrite);
//     prop[0] = LIGHT_PROPERTY_POWER_ON;
//     uecho_object_setpropertydata(obj, LIGHT_PROPERTY_POWER_CODE, prop, 1);
//
//     // Set property observer
//     uecho_object_setpropertywriterequesthandler(obj, LIGHT_PROPERTY_POWER_CODE, uecho_lighting_propertyrequesthandler);
//
//     return obj;
// }
//
// void uecho_light_delete(uEchoObject* obj) {
//     return uecho_object_delete(obj);
// }
//
// uEchoMessage* create_readpropertymessagebycode(int objCode, byte propCode) {
//     uEchoMessage *msg = uecho_message_new();
//     uecho_message_setesv(msg, uEchoEsvReadRequest);
//     uecho_message_setdestinationobjectcode(msg, objCode);
//     uecho_message_setproperty(msg, propCode, NULL, 0);
//     return msg;
// }
//
// uEchoMessage* create_readpropertymessage(uEchoProperty* prop) {
//     return create_readpropertymessagebycode(
//         uecho_object_getcode(uecho_property_getparentobject(prop)),
//         uecho_property_getcode(prop));
// }
//
// uEchoMessage* create_readmanufacturecodemessage() {
//     return create_readpropertymessagebycode(
//         0x0EF001,
//         0x8A);
// }
//
// std::string get_nodemanufacturename(uEchoController *ctrl, uEchoDatabase *db, uEchoNode *node) {
//     std::string manufactureName;// const char *manufactureName = NULL;
//     uEchoMessage *reqMsg = create_readmanufacturecodemessage();
//     uEchoMessage *resMsg = uecho_message_new();
//     if (uecho_controller_postmessage(ctrl, node, reqMsg, resMsg) && (uecho_message_getopc(resMsg) == 1)) {
//         if (uEchoProperty *resProp = uecho_message_getproperty(resMsg, 0)) {
//             int manufactureCode = uecho_bytes_toint(uecho_property_getdata(resProp), uecho_property_getdatasize(resProp));
//             if (uEchoManufacture *manufacture = uecho_database_getmanufacture(db, manufactureCode)) {
//                 manufactureName = uecho_manufacture_getname(manufacture);
//             }
//         }
//     }
//     uecho_message_delete(reqMsg);
//     uecho_message_delete(resMsg);
//
//     return manufactureName;
// }
//
// void print_founddevices(uEchoController* ctrl, bool verbose) {
//     digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
//
//     // uEchoObject* obj;
//     // int objNo;
//
//
//   for (uEchoNode *node = uecho_controller_getnodes(ctrl); node; node = uecho_node_next(node)) {
//     if (!verbose) {
//       ets_printf("%-15s ", uecho_node_getaddress(node).c_str());
//       int objNo = 0;
//       for (uEchoObject* obj = uecho_node_getobjects(node); obj; obj = uecho_object_next(obj)) {
//         ets_printf("[%d] %06X ", objNo, uecho_object_getcode(obj));
//         objNo++;
//       }
//       ets_printf("\n");
//       continue;
//     }
//
// //     uEchoDatabase *db = uecho_standard_getdatabase();
// //     std::string manufactureName = get_nodemanufacturename(ctrl, db, node);
// //     if (manufactureName.empty()) manufactureName = UNKNOWN_STRING;
// //     ets_printf("%-15s (%s)\n", uecho_node_getaddress(node).c_str(), manufactureName.c_str());
// //
// //     int objNo = 0;
// //     for (uEchoObject* obj = uecho_node_getobjects(node); obj; obj = uecho_object_next(obj)) {
// //       ets_printf("[%d] %06X (%s) -> ", objNo, uecho_object_getcode(obj), uecho_object_getname(obj).empty() ? UNKNOWN_STRING : uecho_object_getname(obj).c_str());
// //       int propNo = 0;
// //       for (uEchoProperty *prop = uecho_object_getproperties(obj); prop; prop = uecho_property_next(prop)) {
// //         if (uecho_property_isreadrequired(prop)) {
// //           ets_printf("[%d] [%d] %02X ", objNo, propNo, uecho_property_getcode(prop));
// //           uEchoMessage *reqMsg = create_readpropertymessage(prop);
// //           uEchoMessage *resMsg = uecho_message_new();
// //           if (bool hasResProp = uecho_controller_postmessage(ctrl, node, reqMsg, resMsg)) {
// //             for (int resPropNo = 0; resPropNo < uecho_message_getopc(resMsg); resPropNo++) {
// //               ets_printf("\t(%s = ", uecho_property_getname(prop));
// // propNameCache.insert_or_assign(uecho_property_getcode(prop), uecho_property_getname(prop));
// //               uEchoProperty *resProp = uecho_message_getproperty(resMsg, resPropNo);
// //               if (!resProp) continue;
// //               byte *resPropBytes = uecho_property_getdata(resProp);
// //               for (int n = 0; n < uecho_property_getdatasize(resProp); n++) {
// //                 ets_printf("%02X", resPropBytes[n]);
// //               }
// //               ets_printf(")");
// //             }
// //           }
// //           ets_printf("\n");
// //           uecho_message_delete(reqMsg);
// //           uecho_message_delete(resMsg);
// //         }
// //         propNo++;
// //       }
// //       objNo++;
// //     }
// //
// //     if (uecho_node_next(node))
// //       ets_printf("\n");
//    }
//     digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
// }
//
//

#endif
