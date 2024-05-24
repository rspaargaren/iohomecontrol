#include <iohcOtherDevice2W.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <iohcCryptoHelpers.h>
#include <string>
#include <iohcRadio.h>
#include <vector> 
#include <iohcDevice.h>

namespace IOHC {
    iohcOtherDevice2W* iohcOtherDevice2W::_iohcOtherDevice2W = nullptr;

    iohcOtherDevice2W::iohcOtherDevice2W() = default;

    iohcOtherDevice2W* iohcOtherDevice2W::getInstance() {
        if (!_iohcOtherDevice2W) {
            _iohcOtherDevice2W = new iohcOtherDevice2W();
            _iohcOtherDevice2W->load();
        }
        return _iohcOtherDevice2W;
    }

    uint8_t fake_gateway[3] = {0xba, 0x11, 0xad};

    void iohcOtherDevice2W::init(iohcPacket* packet, size_t typn) {
        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen = sizeof(_header) - 1;

        packet->payload.packet.header.cmd = 0x2A; //0x28; //typn;
        // Common Flags
        packet->payload.packet.header.CtrlByte1.asStruct.EndFrame = 0;
        packet->payload.packet.header.CtrlByte2.asByte = 0;
        packet->payload.packet.header.CtrlByte2.asStruct.Prio = 1;
        packet->payload.packet.header.CtrlByte2.asStruct.LPM = 0;
        
        // Broadcast Target
        u_int16_t bcast = typn;
        //            uint16_t bcast = /*(_type.at(typn)<<6)*/(typn)<<6 + 0b111111;
        packet->payload.packet.header.target[0] = 0x00;
        packet->payload.packet.header.target[1] = bcast >> 8;
        packet->payload.packet.header.target[2] = bcast & 0x00ff;

        packet->frequency = CHANNEL2;
        packet->repeatTime = 50;
        packet->repeat = 0;
        packet->lock = false;
    }

    void iohcOtherDevice2W::cmd(Other2WButton cmd, Tokens* data) {
        // Emulates device button press
        if (!_radioInstance) {
            Serial.println("NO RADIO INSTANCE");
            _radioInstance = iohcRadio::getInstance();
        } // Verify !

        // for (uint8_t typn=0; typn<_type.size(); typn++) { // Pre-allocate packets vector; one packet for each remote type loaded
        //     if (!packets2send[typn])
        //         packets2send[typn] = (IOHC::iohcPacket *)malloc(sizeof(IOHC::iohcPacket));
        //     else
        //         memset((void *)packets2send[typn]->payload.buffer, 0x00, sizeof(packets2send[typn]->payload.buffer));
        // }
        // packets2send[0] = new iohcPacket; //(IOHC::iohcPacket *)malloc(sizeof(IOHC::iohcPacket));
        // packets2send[0]->payload.packet.header.CtrlByte1.asByte = 8; // Header len if protocol version is 0 else 10
        // packets2send[0]->payload.packet.header.CtrlByte2.asByte = 0;
        switch (cmd) {
            case Other2WButton::discovery: {
               packets2send.clear();
                 int bec = 0;
//                for (int x = 0; x < 15; x++) {
//                    for (int i = 0; i < 15; i++) {
                        for (int j = 0; j < 255; j++) {
                            packets2send.push_back(new iohcPacket);
                            // packets2send[j] = new iohcPacket;
                            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                            std::string discovery = "d430477706ba11ad31"; //28"; //"2b578ebc37334d6e2f50a4dfa9";
                            packets2send.back()/*[j]*/->buffer_length = hexStringToBytes(discovery, packets2send.back()/*[j]*/->payload.buffer);
                            init(packets2send.back()/*[j]*//*->payload.buffer[4]*/, bec);
                            bec += 0x01;
                            // packets2send.back()/*[j]*/->repeatTime = 225;
//                            packets2send.back()/*[j]*/->decode(); // Not for the cozydevice -> change and adapt for KLR 2W
                            digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                        }
//                    }
//                }
                        _radioInstance->send(packets2send);
            break;
            }
            case Other2WButton::getName: {
                packets2send.clear();
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                std::vector<uint8_t> toSend = {}; // {0x0C, 0x60, 0x01, 0xFF};

                // const char* dat = data->at(1).c_str();
 
                int value = std::stol(data->at(1).c_str(), nullptr, 16);
                int8_t target[3];
                target[0] = static_cast<uint8_t>(value >> 16);
                target[1] = static_cast<uint8_t>(value >> 8);
                target[2] = static_cast<uint8_t>(value);
                // uint8_t target[3] = {0x08, 0x42, 0xE3};
                // toSend[3] = custom; 

                packets2send.push_back(new iohcPacket);
                init(packets2send.back(), 0);
                packets2send.back()->payload.packet.header.cmd = SEND_GET_NAME_0x50;
                // memorizeSend.memorizedData = toSend;
                // memorizeSend.memorizedCmd = SEND_WRITE_PRIVATE_0x20;

                packets2send.back()->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
                packets2send.back()->payload.packet.header.CtrlByte1.asByte += toSend.size();

                memcpy(packets2send.back()->payload.packet.header.source, fake_gateway, 3);
                memcpy(packets2send.back()->payload.packet.header.target, target, 3);
                memcpy(packets2send.back()->payload.buffer + 9, toSend.data(), toSend.size());

                packets2send.back()->buffer_length = toSend.size() + 9;
                
                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                // packets2send.back()->delayed = 501;
               _radioInstance->send(packets2send);
                break;
            }
            //     case DeviceButton::powerOn: {
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //         std::vector<uint8_t> toSend ={0x0C, 0x60, 0x01, 0x2C};

            //         packets2send[0]->payload.packet.header.cmd = IOHC::iohcDevice::SEND_WRITE_PRIVATE_0x20;
            //         memorizeSend.memorizedData = toSend;
            //         memorizeSend.memorizedCmd = IOHC::iohcDevice::SEND_WRITE_PRIVATE_0x20;

            //         packets2send[0]->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
            //         packets2send[0]->payload.packet.header.CtrlByte1.asByte += toSend.size(); // + 8;

            //         memcpy(packets2send[0]->payload.packet.header.source, gateway, 3);
            //         memcpy(packets2send[0]->payload.packet.header.target, master_to, 3);
            //         memcpy(packets2send[0]->payload.buffer + 9, toSend.data(), toSend.size());

            //         packets2send[0]->buffer_length = toSend.size() + 9; //packet2send[0]->payload.packet.header.framelength +1;
            //         packets2send[0]->frequency = CHANNEL3;
            //         packets2send[0]->stamp = 25;
            //         packets2send[0]->repeat = 1; //0; // Need to stop txMode
            //         packets2send[0]->lock = true; //true; // Need to received ASAP
            //         packets2send[0+1] = nullptr;

            //         packets2send[0]->decode();
            //         _radioInstance->send(packets2send);                               // Verify !
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //     }
            //     break;
            // case DeviceButton::setTemp: {
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //         int temp = 10 * atof(data);
            //         std::vector<uint8_t> toSend = {0x0C, 0x61, 0x01, 0x03, 0xFF, 0x00};
            //         toSend[4] = temp;

            //         packets2send[0]->payload.packet.header.cmd = SEND_WRITE_PRIVATE_0x20;
            //         memorizeSend.memorizedData = toSend; //.assign(toSend, toSend + 6);
            //         memorizeSend.memorizedCmd = SEND_WRITE_PRIVATE_0x20;

            //         packets2send[0]->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
            //         packets2send[0]->payload.packet.header.CtrlByte1.asByte += toSend.size(); // + 8;

            //         memcpy(packets2send[0]->payload.packet.header.source, gateway, 3);
            //         memcpy(packets2send[0]->payload.packet.header.target, master_to, 3);
            //         memcpy(packets2send[0]->payload.buffer + 9, toSend.data(), toSend.size());

            //         packets2send[0]->buffer_length = toSend.size() + 9; //packet2send[0]->payload.packet.header.framelength +1;
            //         packets2send[0]->frequency = CHANNEL3;
            //         packets2send[0]->stamp = 25;
            //         packets2send[0]->repeat = 1; //0; // Need to stop txMode
            //         packets2send[0]->lock = true; //true; // Need to received ASAP
            //         packets2send[0+1] = nullptr;

            //         packets2send[0]->decode();
            //         _radioInstance->send(packets2send);                               // Verify !
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //     }
            //     break;
            // case DeviceButton::setMode: {
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //         std::vector<uint8_t> toSend ={0x0C, 0x61, 0x01, 0x00, 0xFF};

            //         if (strcasecmp(data, "auto") == 0) toSend[4] = 0x00;
            //         if (strcasecmp(data, "prog") == 0) toSend[4] = 0x02;
            //         if (strcasecmp(data, "manual") == 0) toSend[4] = 0x01;

            //         packets2send[0]->payload.packet.header.cmd = SEND_WRITE_PRIVATE_0x20;
            //         memorizeSend.memorizedData = toSend;
            //         memorizeSend.memorizedCmd = SEND_WRITE_PRIVATE_0x20;

            //         packets2send[0]->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
            //         packets2send[0]->payload.packet.header.CtrlByte1.asByte += toSend.size(); // + 8;

            //         memcpy(packets2send[0]->payload.packet.header.source, gateway, 3);
            //         memcpy(packets2send[0]->payload.packet.header.target, master_to, 3);
            //         memcpy(packets2send[0]->payload.buffer + 9, toSend.data(), toSend.size());

            //         packets2send[0]->buffer_length = toSend.size() + 9; //packet2send[0]->payload.packet.header.framelength +1;
            //         packets2send[0]->frequency = CHANNEL3;
            //         packets2send[0]->stamp = 25;
            //         packets2send[0]->repeat = 1; //0; // Need to stop txMode
            //         packets2send[0]->lock = true; //true; // Need to received ASAP
            //         packets2send[0+1] = nullptr;

            //         packets2send[0]->decode();
            //         _radioInstance->send(packets2send);                               // Verify !
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //     }
            //     break;
            // case DeviceButton::setPresence: {
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //         std::vector<uint8_t> toSend = {0x0C, 0x61, 0x01, 0x10, 0xFF};// Accepted command {0x0C, 0x61, 0x11, 0x00, 0x00/01}; Answer  {0x0C, 0x61, 0x11, 0x00, 0x00}

            //         if (strcasecmp(data, "on") == 0) toSend[4] = 0x01;
            //         if (strcasecmp(data, "off") == 0) toSend[4] = 0x00;

            //         packets2send[0]->payload.packet.header.cmd = SEND_WRITE_PRIVATE_0x20;
            //         memorizeSend.memorizedData = toSend;
            //         memorizeSend.memorizedCmd = SEND_WRITE_PRIVATE_0x20;

            //         packets2send[0]->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
            //         packets2send[0]->payload.packet.header.CtrlByte1.asByte += toSend.size(); // + 8;

            //         memcpy(packets2send[0]->payload.packet.header.source, gateway, 3);
            //         memcpy(packets2send[0]->payload.packet.header.target, master_to, 3);
            //         memcpy(packets2send[0]->payload.buffer + 9, toSend.data(), toSend.size());

            //         packets2send[0]->buffer_length = toSend.size() + 9; //packet2send[0]->payload.packet.header.framelength +1;
            //         packets2send[0]->frequency = CHANNEL3;
            //         packets2send[0]->stamp = 25;
            //         packets2send[0]->repeat = 1; //0; // Need to stop txMode
            //         packets2send[0]->lock = true; //true; // Need to received ASAP
            //         packets2send[0+1] = nullptr;

            //         packets2send[0]->decode();
            //         _radioInstance->send(packets2send);                               // Verify !
            //         digitalWrite(RX_LED, digitalRead(RX_LED)^1);
            //     }
            //     break;
        }
        //        save(); // Save Other associated devices
    }

    bool iohcOtherDevice2W::load() {
        _radioInstance = iohcRadio::getInstance();
        if (LittleFS.exists(OTHER_2W_FILE))
            Serial.printf("Loading Other 2W devices settings from %s\n", OTHER_2W_FILE);
        else {
            Serial.printf("*2W Other devices not available\n");
            return false;
        }

        fs::File f = LittleFS.open(OTHER_2W_FILE, "r", true);
        /*Dynamic*/JsonDocument doc; //(256);
        deserializeJson(doc, f);
        f.close();

        // Iterate through the JSON object
        for (JsonPair kv: doc.as<JsonObject>()) {
            hexStringToBytes(kv.key().c_str(), _node);
            auto jobj = kv.value().as<JsonObject>();
            //     hexStringToBytes(jobj["key"].as<const char*>(), _key);
            hexStringToBytes(jobj["dst"].as<const char *>(), _dst);
            //     uint8_t btmp[2];
            //     hexStringToBytes(jobj["sequence"].as<const char*>(), btmp);

            //            /*hexStringToBytes*/(jobj["type"].as<const char*>(), _type);

            //     _sequence = (btmp[0]<<8)+btmp[1];
            //     JsonArray jarr = jobj["type"];

            //     // Iterate through the JSON array
            //     for (uint8_t i=0; i<jarr.size(); i++)
            //         _type.insert(_type.begin()+i, jarr[i].as<uint16_t>());

            //     _manufacturer = jobj["manufacturer_id"].as<uint8_t>();
        }

        return true;
    }
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
    bool iohcOtherDevice2W::save() {
        fs::File f = LittleFS.open(OTHER_2W_FILE, "a+");
        /*Dynamic*/JsonDocument doc; //(256);
        // It's the gateway
        // JsonObject jobj = doc.createNestedObject(bytesToHexString(_node, sizeof(_node)));
        auto jobj = doc[bytesToHexString(_node, sizeof(_node))].to<JsonObject>();
    //        jobj["key"] = bytesToHexString(_key, sizeof(_key));
        jobj["dst"] = _dst;
        //        uint8_t btmp[2];
        //        btmp[1] = _sequence & 0x00ff;
        //        btmp[0] = _sequence >> 8;
        //        jobj["sequence"] = bytesToHexString(btmp, sizeof(btmp));

        //        jobj["_type"] = _type;

        //        JsonArray jarr = jobj.createNestedArray("type");
        //        for (uint8_t i=0; i<_type.size(); i++)
        //            if (_type[i])
        //                jarr.add(_type.at(i));
        //            else
        //                break;
        //        jobj["manufacturer_id"] = _manufacturer;

        serializeJsonPretty/*serializeJson*/(doc, f);
        f.close();

        return true;
    }
}
