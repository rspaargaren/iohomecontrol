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

#ifndef CRYPTO_H
#define CRYPTO_H

#include <board-config.h>
#include <string>
#include <vector>
#include <tuple>

#if defined(ESP8266)
    #include <Crypto.h>
    #include <AES.h>
    #include <CTR.h>
#elif defined(ESP32)
    #include "mbedtls/aes.h"        // AES functions
#endif

#define CRC_POLYNOMIAL_CCITT    0x8408

uint8_t hexStringToBytes(std::string hexString, uint8_t *byteString);
std::string bytesToHexString(const uint8_t *byteString, uint8_t len);

namespace iohcCrypto {
    uint16_t computeCrc(uint8_t data, uint16_t crc);
    uint16_t radioPacketComputeCrc(uint8_t *buffer, uint8_t bufferLength);
    uint16_t radioPacketComputeCrc(std::vector<uint8_t>& buffer);
    void encrypt_1W_key(const uint8_t *node_address, uint8_t *key);
    void create_1W_hmac(uint8_t *hmac, const uint8_t *seq_number, uint8_t *controller_key, const std::vector<uint8_t>& frame_data);
}
#endif