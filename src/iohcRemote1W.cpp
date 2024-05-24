#include <iohcRemote1W.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include <iohcCryptoHelpers.h>


namespace IOHC {
    iohcRemote1W* iohcRemote1W::_iohcRemote1W = nullptr;

    iohcRemote1W::iohcRemote1W() = default;

    iohcRemote1W* iohcRemote1W::getInstance() {
        if (!_iohcRemote1W) {
            _iohcRemote1W = new iohcRemote1W();
            _iohcRemote1W->load();
        }
        return _iohcRemote1W;
    }

    void iohcRemote1W::init(iohcPacket* packet, uint16_t/*size_t*/ typn) {
        //           for (size_t typn=0; typn<_type.size(); typn++) { // Pre-allocate packets vector; one packet for each remote type loaded
        // Pas besoin de new ici, std::array gère automatiquement la mémoire
        // On peut directement accéder à l'élément avec l'opérateur [].
        //            auto& packet = packets2send[typn];
        //                packets2send[typn] = new iohcPacket; //(iohcPacket *) malloc(sizeof(iohcPacket));
        //            else
        //                memset((void *)packets2send[typn]->payload.buffer, 0x00, sizeof(packets2send[typn]->payload.buffer));
        // Common Flags
        //printf("CreateNew packet\n");

        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen = sizeof(_header) - 1;
        packet->payload.packet.header.CtrlByte1.asStruct.Protocol = 1;
        packet->payload.packet.header.CtrlByte1.asStruct.StartFrame = 1;
        packet->payload.packet.header.CtrlByte1.asStruct.EndFrame = 1;
        packet->payload.packet.header.CtrlByte2.asByte = 0;
        packet->payload.packet.header.CtrlByte2.asStruct.LPM = 1;
        // Broadcast Target
        uint16_t bcast = (typn << 6) + 0b111111; // (_type.at(typn) << 6) + 0b111111;
        packet->payload.packet.header.target[0] = 0x00;
        packet->payload.packet.header.target[1] = bcast >> 8;
        packet->payload.packet.header.target[2] = bcast & 0x00ff;

        packet->frequency = CHANNEL2;
        packet->repeatTime = 40;
        packet->repeat = 0;
        packet->lock = false;
        // }
    }

    std::vector<uint8_t> frame;

    void iohcRemote1W::cmd(RemoteButton cmd, Tokens* data) {
        std::string description = data->at(1).c_str();

        auto it = std::find_if( remotes.begin(), remotes.end(),  [&] ( const remote &r  ) {
                 return description == r.description;
              } );

        remote& r = *it;
        bool found = true;
        if (it == remotes.end()) {
            printf("ERROR %s NOT IN JSON", description.c_str());
            found = false;
            // return;
        }
/*
        int value = 0;
        try {
            value = std::strtol(data->at(1).c_str(), nullptr, 16);
        } catch (const std::invalid_argument& e) {
            printf("ERROR: Invalid 1W address format (std::invalid_argument): %s\n", data->at(1).c_str());
            // return;
        } catch (const std::out_of_range& e) {
            printf("ERROR: USE A VALID 1W ADDRESS");
            //return;
        } catch (const std::exception& e) {
            printf("ERROR: Unexpected exception during conversion (%s).", e.what());
            // return;
        }
*/
/*
        auto it = std::find_if(remotes.begin(), remotes.end(), [&](const remote& r) {
            // Create a temporary object (assuming you have a way to construct it)
            iohcRemote1W tempRemote;
            return tempRemote.compareTarget(r);
        });

        if (it == remotes.end()) {
            printf("ERROR %p NOT IN JSON", target);
            return; }
*/
/*
        // remote address
        address remoteAddress = {static_cast<uint8_t>(value >> 16),
                                 static_cast<uint8_t>(value >> 8),
                                 static_cast<uint8_t>(value)};

        const remote* foundRemote = nullptr;
        for (const auto& oneRemote : remotes) {
            if (memcmp(oneRemote.node, remoteAddress, sizeof(address)) == 0) {
                foundRemote = &oneRemote;
                break;
            }
        }
        if (foundRemote != nullptr)
            printf( "Remote found : %s\n", foundRemote->description.c_str());
*/
        // Emulates remote button press
        switch (cmd) {
            case RemoteButton::Pair: {
                // 0x2e: 0x1120 + target broadcast + source + 0x2e00 + sequence + hmac
                packets2send.clear();

                IOHC::relStamp = esp_timer_get_time();
//                for (auto&r: remotes) {
                if (!found) break;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                    auto* packet = new iohcPacket;
                    IOHC::iohcRemote1W::init(packet, r.type[0]);
                    // Packet length
                    packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x2e);

                    // Source (me)
                    for (size_t i = 0; i < sizeof(address); i++)
                        packet->payload.packet.header.source[i] = r.node[i];

                    //Command
                    packet->payload.packet.header.cmd = 0x2e;
                    // Data
                    packet->payload.packet.msg.p0x2e.data = 0x00;
                    // Sequence
                    packet->payload.packet.msg.p0x2e.sequence[0] = r.sequence >> 8;
                    packet->payload.packet.msg.p0x2e.sequence[1] = r.sequence & 0x00ff;
                    r.sequence += 1;
                    // hmac
                    frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + 2);
                    uint8_t hmac[16];
                    iohcCrypto::create_1W_hmac(hmac, packet->payload.packet.msg.p0x2e.sequence, r.key, frame);

                    for (uint8_t i = 0; i < 6; i++)
                        packet->payload.packet.msg.p0x2e.hmac[i] = hmac[i];

                    packet->buffer_length = packet->payload.packet.header.CtrlByte1.asStruct.MsgLen + 1;

                    packets2send.push_back(packet);
                    // if (typn) packet->payload.packet.header.CtrlByte2.asStruct.LPM = 0; //TODO only first is LPM
                    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
//                }
                _radioInstance->send(packets2send);
                break;
            }

            case RemoteButton::Remove: {
                // 0x39: 0x1c00 + target broadcast + source + 0x3900 + sequence + hmac
                packets2send.clear();

                IOHC::relStamp = esp_timer_get_time();
//                for (auto&r: remotes) {
                if (!found) break;

                digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                    auto* packet = new iohcPacket;
                    IOHC::iohcRemote1W::init(packet, r.type[0]);
                    // Packet length
                    //                    packet->payload.packet.header.CtrlByte1.asStruct.MsgLen = sizeof(_header) - 1;
                    packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x2e);

                    // Source (me)
                    for (size_t i = 0; i < sizeof(address); i++)
                        packet->payload.packet.header.source[i] = r.node[i];

                    //Command
                    packet->payload.packet.header.cmd = 0x39;
                    // Data
                    packet->payload.packet.msg.p0x2e.data = 0x00;
                    // Sequence
                    packet->payload.packet.msg.p0x2e.sequence[0] = r.sequence >> 8;
                    packet->payload.packet.msg.p0x2e.sequence[1] = r.sequence & 0x00ff;
                    r.sequence += 1;
                    // hmac
                    uint8_t hmac[16];
                    frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + 2);
                    iohcCrypto::create_1W_hmac(hmac, packet->payload.packet.msg.p0x2e.sequence, r.key, frame);
                    for (uint8_t i = 0; i < 6; i++)
                        packet->payload.packet.msg.p0x2e.hmac[i] = hmac[i];

                    packet->buffer_length = packet->payload.packet.header.CtrlByte1.asStruct.MsgLen + 1;

                    packets2send.push_back(packet);
                    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
//                }
                _radioInstance->send(packets2send);
                //printf("\n");
                break;
            }

            case RemoteButton::Add: {
                // 0x30: 0x1100 + target broadcast + source + 0x3000 + ???
                packets2send.clear();

                IOHC::relStamp = esp_timer_get_time();
//                for (auto&r: remotes) {
                if (!found) break;

                    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                    auto* packet = new iohcPacket;
                    IOHC::iohcRemote1W::init(packet, r.type[0]);
                    // Packet length
                    packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x30);

                    // Source (me)
                    for (size_t i = 0; i < sizeof(address); i++)
                        packet->payload.packet.header.source[i] = r.node[i];
                    //Command
                    packet->payload.packet.header.cmd = 0x30;

                    // Encrypted key
                    uint8_t encKey[16];
                    memcpy(encKey, r.key, 16);
                    iohcCrypto::encrypt_1W_key(r.node, encKey);
                    memcpy(packet->payload.packet.msg.p0x30.enc_key, encKey, 16);

                    // Manufacturer
                    packet->payload.packet.msg.p0x30.man_id = r.manufacturer;
                    // Data
                    packet->payload.packet.msg.p0x30.data = 0x01;
                    // Sequence
                    packet->payload.packet.msg.p0x30.sequence[0] = r.sequence >> 8;
                    packet->payload.packet.msg.p0x30.sequence[1] = r.sequence & 0x00ff;
                    r.sequence += 1;

                    packet->buffer_length = packet->payload.packet.header.CtrlByte1.asStruct.MsgLen + 1;

                    packets2send.push_back(packet);
                    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
//                }
                _radioInstance->send(packets2send);
                break;
            }
            case RemoteButton::testKey: {
                std::string controller_key = "d94a00399a46b5aba67f3809b68ecc52"; // In clear
                std::string node_address = "8ad42e";
                std::string dest_address = "00003f";
                //    std::string sequence = "1934";

                uint8_t bcontroller[16];
                uint8_t bnode[3];
                uint8_t bdest[3];
                uint8_t bseq[2] = {0x1a, 0x40}; // <-- Set Sequance number here
                uint8_t output[16];


                hexStringToBytes(controller_key, bcontroller);
                hexStringToBytes(node_address, bnode);
                hexStringToBytes(dest_address, bdest);

                std::string tmp = "f60000003f8ad42e000161d20000001a4731661e6275d2";
                uint8_t btmp[32];
                uint8_t bLen = hexStringToBytes(tmp, btmp);
                std::vector buffer(btmp, btmp + bLen);
                uint16_t crc = iohcCrypto::radioPacketComputeCrc(btmp, bLen);
                printf("--> %s (%d) crc %2.2x%2.2x <--\t\t", tmp.c_str(), bLen, crc & 0x00ff, crc >> 8);
                crc = iohcCrypto::radioPacketComputeCrc(buffer);
                printf("--> alt crc %2.2x%2.2x <--\n\n", crc & 0x00ff, crc >> 8);


                printf("Node address: ");
                for (unsigned char idx: bnode)
                    printf("%2.2x", idx);
                printf("\n");
                printf("Dest address: ");
                for (unsigned char idx: bdest)
                    printf("%2.2x", idx);
                printf("\n");
                printf("Controller key in clear:  ");
                for (unsigned char idx: bcontroller)
                    printf("%2.2x", idx);
                printf("\n");

                std::vector node(bnode, bnode + 3);
                std::vector controller(bcontroller, bcontroller + 16);
                std::vector<uint8_t> ret;

                iohcCrypto::encrypt_1W_key(bnode, bcontroller);
                printf("Controller key encrypted: ");
                for (unsigned char idx: bcontroller)
                    printf("%2.2x", idx);
                printf("\n");


                uint16_t test = (bseq[0] << 8) + bseq[1];

                for (uint8_t i = 0; i < 0x20; i++) {
                    test += 1;
                    bseq[1] = test & 0x00ff;
                    bseq[0] = test >> 8;

                    /*
                    Main parameter:
                    0x0000    100% Open
                    0xd200    Current (used as stop)
                    0xC800    0% Open
                    0xD803    Secured ventilation
                    */
                    //  00 01 61 c800 00 00 1a05
                    //    uint8_t bframe[9] = {0x00, 0x01, 0x61, 0xc8, 0x00, 0x02, 0x00, 0x1a, 0x07};
                    std::vector seq(bseq, bseq + 2);
                    uint8_t bframe1[7] = {0x00, 0x01, 0x61, 0x00, 0x00, 0x02, 0x00};
                    // <-- Set packet here, excluding sequence number, then adjust length
                    std::vector frame1(bframe1, bframe1 + 7); // <-- adjust packet length
                    hexStringToBytes(controller_key, bcontroller);
                    iohcCrypto::create_1W_hmac(output, bseq, bcontroller, frame1);

                    printf("Open: f620");
                    for (unsigned char idx: bdest)
                        printf("%2.2x", idx);
                    for (unsigned char idx: bnode)
                        printf("%2.2x", idx);
                    for (unsigned char idx: bframe1) // <-- adjust packet length
                        printf("%2.2x", idx);
                    for (unsigned char idx: bseq)
                        printf("%2.2x", idx);
                    for (uint8_t idx = 0; idx < 6; idx++)
                        printf("%2.2x", output[idx]);
                    printf("\t\t");

                    uint8_t bframe2[7] = {0x00, 0x01, 0x61, 0xC8, 0x00, 0x02, 0x00};
                    // <-- Set packet here, excluding sequence number, then adjust length
                    std::vector frame2(bframe2, bframe2 + 7); // <-- adjust packet length
                    hexStringToBytes(controller_key, bcontroller);
                    iohcCrypto::create_1W_hmac(output, bseq, bcontroller, frame2);

                    printf("Close: f620");
                    for (unsigned char idx: bdest)
                        printf("%2.2x", idx);
                    for (unsigned char idx: bnode)
                        printf("%2.2x", idx);
                    for (unsigned char idx: bframe2) // <-- adjust packet length
                        printf("%2.2x", idx);
                    for (unsigned char idx: bseq)
                        printf("%2.2x", idx);
                    for (uint8_t idx = 0; idx < 6; idx++)
                        Serial.printf("%2.2x", output[idx]);
                    printf("\n");
                }
                break;
            }
            default: {
                packets2send.clear();

                IOHC::relStamp = esp_timer_get_time();
                // 0x00: 0x1600 + target broadcast + source + 0x00 + Originator + ACEI + Main Param + FP1 + FP2 + sequence + hmac
//                for (auto&r: remotes) {
                if (!found) break;

                    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);

                    auto* packet = new iohcPacket;
                    IOHC::iohcRemote1W::init(packet, r.type[0]);
                    // Packet length
                    // packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x00);
                    // Source (me)
                    for (size_t i = 0; i < sizeof(address); i++)
                        packet->payload.packet.header.source[i] = r.node[i];
                    //Command
                    packet->payload.packet.header.cmd = 0x00;
                    packet->payload.packet.msg.p0x00_14.origin = 0x01; // Command Source Originator is: 0x01 User
                    //Acei packet->payload.packet.msg.p0x00.acei;
                    setAcei(packet->payload.packet.msg.p0x00_14.acei, 0x43); //0xE7); //0x61);
                    switch (cmd) {
                        // Switch for Main Parameter of cmd 0x00: Open/Close/Stop/Ventilation
                        case RemoteButton::Open:
                            packet->payload.packet.msg.p0x00_14.main[0] = 0x00;
                            packet->payload.packet.msg.p0x00_14.main[1] = 0x00;
                            break;
                        case RemoteButton::Close:
                            packet->payload.packet.msg.p0x00_14.main[0] = 0xc8;
                            packet->payload.packet.msg.p0x00_14.main[1] = 0x00;
                            break;
                        case RemoteButton::Stop:
                            packet->payload.packet.msg.p0x00_14.main[0] = 0xd2;
                            packet->payload.packet.msg.p0x00_14.main[1] = 0x00;
                            break;
                        case RemoteButton::Vent:
                            packet->payload.packet.msg.p0x00_14.main[0] = 0xd8;
                            packet->payload.packet.msg.p0x00_14.main[1] = 0x03;
                            break;
                        case RemoteButton::ForceOpen:
                            packet->payload.packet.msg.p0x00_14.main[0] = 0x64;
                            packet->payload.packet.msg.p0x00_14.main[1] = 0x00;
                            break;
                        case RemoteButton::Mode1:{
                            /* fast = 4x13 Increment fp2 - slow = 0x01 4x13 followed 0x00 4x14 Main 0xD2
                            Every 9 : 10:31:38.367 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db000900000323e7ceefedf9ce81        SEQ 23e7 MAC ceefedf9ce81  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
16:59:58.148 > (21) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 01 >  DATA(13)  01430500112416406780a53021    SEQ 2416 MAC 406780a53021  Org 1 Acei 43 Main 5 fp1 0 fp2 11  Acei 2 0 1 1  Type All
16:59:58.188 > (21) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 01 <  DATA(13)  01430500112416406780a53021    SEQ 2416 MAC 406780a53021  Org 1 Acei 43 Main 5 fp1 0 fp2 11  Acei 2 0 1 1  Type All 
16:59:58.212 > (21) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 01 <  DATA(13)  01430500112416406780a53021    SEQ 2416 MAC 406780a53021  Org 1 Acei 43 Main 5 fp1 0 fp2 11  Acei 2 0 1 1  Type All 
16:59:58.238 > (21) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 01 <  DATA(13)  01430500112416406780a53021    SEQ 2416 MAC 406780a53021  Org 1 Acei 43 Main 5 fp1 0 fp2 11  Acei 2 0 1 1  Type All 
16:59:58.267 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143d200000024179f18402aa33d  SEQ 2417 MAC 9f18402aa33d  Org 1 Acei 43 Main D200 fp1 0 fp2 0  Acei 2 0 1 1  Type All 
16:59:58.292 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143d200000024179f18402aa33d  SEQ 2417 MAC 9f18402aa33d  Org 1 Acei 43 Main D200 fp1 0 fp2 0  Acei 2 0 1 1  Type All 
16:59:58.318 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143d200000024179f18402aa33d  SEQ 2417 MAC 9f18402aa33d  Org 1 Acei 43 Main D200 fp1 0 fp2 0  Acei 2 0 1 1  Type All 
16:59:58.342 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143d200000024179f18402aa33d  SEQ 2417 MAC 9f18402aa33d  Org 1 Acei 43 Main D200 fp1 0 fp2 0  Acei 2 0 1 1  Type All 
16:59:58.398 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db000900000324182ea14f27d208        SEQ 2418 MAC 2ea14f27d208  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
16:59:58.422 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db000900000324182ea14f27d208        SEQ 2418 MAC 2ea14f27d208  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
16:59:58.448 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db000900000324182ea14f27d208        SEQ 2418 MAC 2ea14f27d208  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
16:59:58.472 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db000900000324182ea14f27d208        SEQ 2418 MAC 2ea14f27d208  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
                           */
                            //   r.sequence = 0x0835; //DEBUG
                            packet->payload.packet.header.cmd = 0x01;
                            packet->payload.packet.msg.p0x01_13.main = 0x00;
                            packet->payload.packet.msg.p0x01_13.fp1 = 0x01; // Observed // 0x02; //IZYMO
                            packet->payload.packet.msg.p0x01_13.fp2 = r.sequence & 0xFF;
                            // if (packet->payload.packet.header.source[2] == 0x1A) {packet->payload.packet.msg.p0x01_13.fp1 = 0x80;packet->payload.packet.msg.p0x01_13.fp2 = 0xD3;packet->payload.packet.header.source[2] = 0x1B; packet->payload.packet.msg.p0x01_13.fp2 = r.sequence--;}
                            break;
                        }
                        
                        case RemoteButton::Mode2: {
                            /* Always: press = 0x01 4x13 followed by release = 0x01 4x13 Increment fp2
12:46:44.045 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 >  DATA(13)  0143000276085a643d86021cdf    SEQ 085a MAC 643d86021cdf  Org 1 Acei 43 Main 0 fp1 2 fp2 76  Acei 2 0 1 1  Type All
12:46:44.068 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000276085a643d86021cdf    SEQ 085a MAC 643d86021cdf  Org 1 Acei 43 Main 0 fp1 2 fp2 76  Acei 2 0 1 1  Type All 
12:46:44.092 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000276085a643d86021cdf    SEQ 085a MAC 643d86021cdf  Org 1 Acei 43 Main 0 fp1 2 fp2 76  Acei 2 0 1 1  Type All 
12:46:44.117 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000276085a643d86021cdf    SEQ 085a MAC 643d86021cdf  Org 1 Acei 43 Main 0 fp1 2 fp2 76  Acei 2 0 1 1  Type All 
12:46:44.392 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000277085b9c9dd8d480dd    SEQ 085b MAC 9c9dd8d480dd  Org 1 Acei 43 Main 0 fp1 2 fp2 77  Acei 2 0 1 1  Type All 
12:46:44.414 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000277085b9c9dd8d480dd    SEQ 085b MAC 9c9dd8d480dd  Org 1 Acei 43 Main 0 fp1 2 fp2 77  Acei 2 0 1 1  Type All 
12:46:44.437 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000277085b9c9dd8d480dd    SEQ 085b MAC 9c9dd8d480dd  Org 1 Acei 43 Main 0 fp1 2 fp2 77  Acei 2 0 1 1  Type All 
12:46:44.463 > (21) 1W S 1 E 1  FROM B60D1B TO 00003F CMD 01 <  DATA(13)  0143000277085b9c9dd8d480dd    SEQ 085b MAC 9c9dd8d480dd  Org 1 Acei 43 Main 0 fp1 2 fp2 77  Acei 2 0 1 1  Type All
                           */
                            //   r.sequence = 0x085A; //DEBUG
                            packet->payload.packet.header.cmd = 0x01;
                            packet->payload.packet.msg.p0x01_13.main/*[0]*/ = 0x00;
                            // packet->payload.packet.msg.p0x01_13.main[1] = 0x02;
                            packet->payload.packet.msg.p0x01_13.fp1 = 0x02;
                            packet->payload.packet.msg.p0x01_13.fp2 =  r.sequence & 0xFF;
                            // if (packet->payload.packet.header.source[2] == 0x1A) {packet->payload.packet.header.source[2] = 0x1B; packet->payload.packet.msg.p0x01_13.fp2++; r.sequence += 1;} // DEBUG r.sequence;}
                            break;
                    }
                    /*Light or up/down*/
                    case RemoteButton::Mode3:{
                        // r.sequence = 0x2262; // DEBUG
/* 0x00 4x16 + 0x00 4x14 + 0x00 4x16
11:26:26.903 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 >  DATA(16)  0143000080d300002262bcff22b0d713      SEQ 2262 MAC bcff22b0d713  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 D3  Acei 2 0 1 1
11:26:26.927 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080d300002262bcff22b0d713      SEQ 2262 MAC bcff22b0d713  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 D3  Acei 2 0 1 1 
11:26:26.952 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080d300002262bcff22b0d713      SEQ 2262 MAC bcff22b0d713  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 D3  Acei 2 0 1 1 
11:26:26.976 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080d300002262bcff22b0d713      SEQ 2262 MAC bcff22b0d713  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 D3  Acei 2 0 1 1 
11:26:27.005 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143000000002262d92e2bb45c29  SEQ 2262 MAC d92e2bb45c29  Type All  Org 1 Acei 43 Main 0 fp1 0 fp2 0  Acei 2 0 1 1 
11:26:27.030 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143000000002262d92e2bb45c29  SEQ 2262 MAC d92e2bb45c29  Type All  Org 1 Acei 43 Main 0 fp1 0 fp2 0  Acei 2 0 1 1 
11:26:27.054 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143000000002262d92e2bb45c29  SEQ 2262 MAC d92e2bb45c29  Type All  Org 1 Acei 43 Main 0 fp1 0 fp2 0  Acei 2 0 1 1 
11:26:27.101 > (22) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(14)  0143000000002262d92e2bb45c29  SEQ 2262 MAC d92e2bb45c29  Type All  Org 1 Acei 43 Main 0 fp1 0 fp2 0  Acei 2 0 1 1 
11:26:27.129 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080c80000226359c4c4837a4f      SEQ 2263 MAC 59c4c4837a4f  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 C8  Acei 2 0 1 1 
11:26:27.156 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080c80000226359c4c4837a4f      SEQ 2263 MAC 59c4c4837a4f  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 C8  Acei 2 0 1 1 
11:26:27.179 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080c80000226359c4c4837a4f      SEQ 2263 MAC 59c4c4837a4f  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 C8  Acei 2 0 1 1 
11:26:27.206 > (24) 1W S 1 E 1  FROM B60D1A TO 0001BF CMD 00 <  DATA(16)  0143000080c80000226359c4c4837a4f      SEQ 2263 MAC 59c4c4837a4f  Type Light  Org 1 Acei 43 Main 0 fp1 80 fp2 C8  Acei 2 0 1 1 
*/
                        break;
                    }
                    case RemoteButton::Mode4: {
/* 0x00 4x16  MAIN D200 FP 20 FP2 CC DATA A200 or MAIN D200 FP 20 FP2 CD DATA 2E00 
Every 9 -> 0x20 12:41:28.171 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db0009000003233d56ca3c456f2d        SEQ 233d MAC 56ca3c456f2d  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All
10:10:36.905 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 >  DATA(16)  0143d20020cd2e00 23d5ec80e44be6b6      SEQ 23d5 MAC ec80e44be6b6  Org 1 Acei 43 Main D200 fp1 20 fp2 CD Data 2E00 Acei 2 0 1 1  Type All    
10:10:36.929 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(16)  0143d20020cd2e00 23d5ec80e44be6b6      SEQ 23d5 MAC ec80e44be6b6  Org 1 Acei 43 Main D200 fp1 20 fp2 CD Data 2E00 Acei 2 0 1 1  Type All 
10:10:36.955 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(16)  0143d20020cd2e00 23d5ec80e44be6b6      SEQ 23d5 MAC ec80e44be6b6  Org 1 Acei 43 Main D200 fp1 20 fp2 CD Data 2E00 Acei 2 0 1 1  Type All 
10:10:36.980 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(16)  0143d20020cd2e00 23d5ec80e44be6b6      SEQ 23d5 MAC ec80e44be6b6  Org 1 Acei 43 Main D200 fp1 20 fp2 CD Data 2E00 Acei 2 0 1 1  Type All 

10:10:41.420 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 >  DATA(16)  0143d20020cca200 23d6c90d142dae8a      SEQ 23d6 MAC c90d142dae8a  Org 1 Acei 43 Main D200 fp1 20 fp2 CC Data A200 Acei 2 0 1 1  Type All    
10:10:41.442 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(16)  0143d20020cca200 23d6c90d142dae8a      SEQ 23d6 MAC c90d142dae8a  Org 1 Acei 43 Main D200 fp1 20 fp2 CC Data A200 Acei 2 0 1 1  Type All 
10:10:41.469 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(16)  0143d20020cca200 23d6c90d142dae8a      SEQ 23d6 MAC c90d142dae8a  Org 1 Acei 43 Main D200 fp1 20 fp2 CC Data A200 Acei 2 0 1 1  Type All 
10:10:41.494 > (24) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 00 <  DATA(16)  0143d20020cca200 23d6c90d142dae8a      SEQ 23d6 MAC c90d142dae8a  Org 1 Acei 43 Main D200 fp1 20 fp2 CC Data A200 Acei 2 0 1 1  Type All 

10:12:18.352 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db0009000003 23dc49fa35972c4b        SEQ 23dc MAC 49fa35972c4b  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
10:12:18.376 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db0009000003 23dc49fa35972c4b        SEQ 23dc MAC 49fa35972c4b  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
10:12:18.402 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db0009000003 23dc49fa35972c4b        SEQ 23dc MAC 49fa35972c4b  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
10:12:18.427 > (23) 1W S 1 E 1  FROM B60D1A TO 00003F CMD 20 <  DATA(15)  02db0009000003 23dc49fa35972c4b        SEQ 23dc MAC 49fa35972c4b  Org 2 Acei DB Main 9 fp1 0 fp2 0  Acei 6 3 1 1  Type All 
*/
                            // r.sequence = 0x2313; // DEBUG
                            packet->payload.packet.header.cmd = 0x00;
                            packet->payload.packet.msg.p0x00_16.main[0] = 0xd2;
                            packet->payload.packet.msg.p0x00_16.main[1] = 0x00;
                            packet->payload.packet.msg.p0x00_16.fp1 = 0x20;
                            packet->payload.packet.msg.p0x00_16.fp2 = 0xCD; 
                            packet->payload.packet.msg.p0x00_16.data[0] = 0x2E; 
                            packet->payload.packet.msg.p0x00_16.data[1] = 0x00; 
                             if (packet->payload.packet.header.source[2] == 0x1B) {
                                // packet->payload.packet.header.source[2] = 0x1A;
                                packet->payload.packet.msg.p0x00_16.fp2 = 0xCC; 
                                packet->payload.packet.msg.p0x00_16.data[0] = 0xA2; 
                            }

                        break;
                    }
                        default: // If reaching default here, then cmd is not recognized, then return
                            return;
                    }
                    /*
                                        if (r.type == 6) { // Vert
                                            //typen
                                            packet->payload.packet.msg.p0x00_14.fp1 = 0x80;
                                            packet->payload.packet.msg.p0x00_14.fp2 = 0xD3;
                                            // Packet length
                                            packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x00_14);
                                        }
                    */
                    // if (r.type == 6) { // Jaune
                    //     packet->payload.packet.msg.p0x00.fp1 = 0x80;
                    //     packet->payload.packet.msg.p0x00.fp2 = 0xC8;
                    //     // Packet length
                    //     packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x00);
                    // }
                    // hmac
                    uint8_t hmac[16];
                    // frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + 7);
                    // + toAdd);

                    if (r.type[0] == 0 && (cmd == RemoteButton::Mode1 ||  cmd == RemoteButton::Mode2)) {
                        //                        // packet->payload.packet.header.cmd = 0x01;
                        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x01_13) ;
                        //                        // _sequence -= 1; // Use same sequence as light
                        packet->payload.packet.msg.p0x01_13.sequence[0] = r.sequence >> 8;
                        packet->payload.packet.msg.p0x01_13.sequence[1] = r.sequence & 0x00ff;
                        uint8_t toAdd = 5 + 1; // OK
                        frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + toAdd);
                        iohcCrypto::create_1W_hmac(hmac, packet->payload.packet.msg.p0x01_13.sequence, r.key, frame);
                        for (uint8_t i = 0; i < 6; i++) {
                            packet->payload.packet.msg.p0x01_13.hmac[i] = hmac[i];
                        }
                    }

                    else if (r.type[0] == 0 && (cmd == RemoteButton::Mode4 )) {
                        
                        //                        // packet->payload.packet.header.cmd = 0x01;
                        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x00_16) ;
                        //                        // _sequence -= 1; // Use same sequence as light
                        packet->payload.packet.msg.p0x00_16.sequence[0] = r.sequence >> 8;
                        packet->payload.packet.msg.p0x00_16.sequence[1] = r.sequence & 0x00ff;
                        uint8_t toAdd = 8 + 1;                        
                        frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + toAdd);
                        iohcCrypto::create_1W_hmac(hmac, packet->payload.packet.msg.p0x00_16.sequence, r.key, frame);
                        for (uint8_t i = 0; i < 6; i++) {
                            packet->payload.packet.msg.p0x00_16.hmac[i] = hmac[i];
                        }
                    }
                    else {
                        packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x00_14) ;
                        // Sequence
                        packet->payload.packet.msg.p0x00_14.sequence[0] = r.sequence >> 8;
                        packet->payload.packet.msg.p0x00_14.sequence[1] = r.sequence & 0x00ff;
                        uint8_t toAdd =  6 + 1; //OK
                        frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + toAdd);
                        iohcCrypto::create_1W_hmac(hmac, packet->payload.packet.msg.p0x00_14.sequence, r.key, frame);
                        for (uint8_t i = 0; i < 6; i++) {
                            packet->payload.packet.msg.p0x00_14.hmac[i] = hmac[i];
                        }
                    }
                    /*
                                        if (r.type == 0xff) {
                                            packet->payload.packet.header.cmd = 0x20;
                                            packet->payload.packet.msg.p0x00_14.origin = 0x02;
                                            packet->payload.packet.msg.p0x00_14.acei.asByte = 0xDB;
                                            packet->payload.packet.header.CtrlByte1.asStruct.MsgLen += sizeof(_p0x00_14);
                                        }
                    */
                    r.sequence += 1;
                    // hmac
                    // uint8_t hmac[16];
                    // frame = std::vector(&packet->payload.packet.header.cmd, &packet->payload.packet.header.cmd + 7 + toAdd);
                    // iohcCrypto::create_1W_hmac(hmac, packet->payload.packet.msg.p0x00.sequence, _key, frame);
                    // for (uint8_t i = 0; i < 6; i++) {
                    //     packet->payload.packet.msg.p0x00.hmac[i] = hmac[i];
                    //     packet->payload.packet.msg.p0x00_all.hmac[i] = hmac[i];
                    // }
                    packet->buffer_length = packet->payload.packet.header.CtrlByte1.asStruct.MsgLen + 1;

                    packets2send.push_back(packet);
                    digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
                }
                _radioInstance->send(packets2send);
                break;
//            }
        }
        this->save(); // Save sequence number
    }

    bool iohcRemote1W::load() {
        _radioInstance = iohcRadio::getInstance();

        if (LittleFS.exists(IOHC_1W_REMOTE))
            Serial.printf("Loading 1W remote settings from %s\n", IOHC_1W_REMOTE);
        else {
            Serial.printf("*1W remote not available\n");
            return false;
        }

        fs::File f = LittleFS.open(IOHC_1W_REMOTE, "r");
        /*Dynamic*/JsonDocument doc; //(8192);

        DeserializationError error = deserializeJson(doc, f); // buf.get());

        if (error) {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return false;
        }
        f.close();

        // Iterate through the JSON object
        for (JsonPair kv: doc.as<JsonObject>()) {
            remote r;
            // hexStringToBytes(kv.key().c_str(), _node);
            hexStringToBytes(kv.key().c_str(), r.node);
            // Serial.printf("%s\n", kv.key().c_str());

            auto jobj = kv.value().as<JsonObject>();
            // hexStringToBytes(jobj["key"].as<const char *>(), _key);
            hexStringToBytes(jobj["key"].as<const char *>(), r.key);

            uint8_t btmp[2];
            hexStringToBytes(jobj["sequence"].as<const char *>(), btmp);
            // _sequence = (btmp[0] << 8) + btmp[1];
            r.sequence = (btmp[0] << 8) + btmp[1];
            JsonArray jarr = jobj["type"];
            // Réservez de l'espace dans le vecteur pour éviter les allocations inutiles

            //_type.reserve(jarr.size());
            r.type.reserve(jarr.size());

            // Iterate through the JSON  type array
            for (auto && i : jarr) {
            // _type.insert(_type.begin() + i, jarr[i].as<uint16_t>());
            //_type.push_back(i.as<uint16_t>());
                r.type.push_back(i.as<uint8_t>());
            }
                       
            // _type = jobj["type"].as<u_int16_t>();
//            r.type = jobj["type"].as<u_int16_t>();

            // _manufacturer = jobj["manufacturer_id"].as<uint8_t>();
            r.manufacturer = jobj["manufacturer_id"].as<uint8_t>();
            r.description = jobj["description"].as<std::string>();
            remotes.push_back(r);
        }

        Serial.printf("Loaded %d x 1W remotes\n", remotes.size()); // _type.size());
        // _sequence = 0x1402;    // DEBUG
        return true;
    }
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
    bool iohcRemote1W::save() {
        fs::File f = LittleFS.open(IOHC_1W_REMOTE, "w+");
        /*Dynamic*/JsonDocument doc; //(8192);
        for (const auto&r: remotes) {
            // jobj["key"] = bytesToHexString(_key, sizeof(_key));
//            JsonObject jobj = doc.createNestedObject(bytesToHexString(r.node, sizeof(r.node)));
            auto jobj = doc[bytesToHexString(r.node, sizeof(r.node))].to<JsonObject>();
            jobj["key"] = bytesToHexString(r.key, sizeof(r.key));

            uint8_t btmp[2];
            // btmp[1] = _sequence & 0x00ff;
            // btmp[0] = _sequence >> 8;
            btmp[1] = r.sequence & 0x00ff;
            btmp[0] = r.sequence >> 8;

            jobj["sequence"] = bytesToHexString(btmp, sizeof(btmp));
            
            // JsonArray jarr = jobj.createNestedArray("type");
            auto jarr = jobj["type"].to<JsonArray>();
            for (uint8_t i : r.type) {
                // if (i)
                bool added = jarr.add(i);
                // else
                // break;
                }

            // jobj["manufacturer_id"] = _manufacturer;
            jobj["manufacturer_id"] = r.manufacturer;
            jobj["description"] = r.description;
        }
        serializeJson(doc, f);
        f.close();

        return true;
    }
}
