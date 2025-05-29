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

#include <iohcCryptoHelpers.h>
#include <crypto2Wutils.h> 
/*
    Helper function to convert a string containing hex numbers to a bytes sequence; one byte every two characters
*/
uint8_t hexStringToBytes(const std::string hexString, uint8_t *byteString) {
size_t i;

    if (hexString.length() % 2 != 0)
        return 0;

    for (i = 0; i < hexString.length(); i += 2)
        byteString[i/2] = (char) strtol(hexString.substr(i, 2).c_str(), nullptr, 16);

    return i/2;
}

/**
    Helper function to convert a byte sequence to an hex string
*/
std::string bytesToHexString(const uint8_t *byteString, uint8_t len) {
    char rec[len*2+1];

    for (uint8_t i=0; i<len; i++)
        sprintf((rec+(i*2)), "%2.2x", byteString[i]);

    return std::string(rec);
}

namespace iohcCrypto {
    std::string transfer_key = "34c3466ed88f4e8e16aa473949884373";


#if defined(ESP8266)
    AES128 aes128;
    CTR<AES128> ctraes128;
#elif defined(ESP32)
    mbedtls_aes_context aes;
#endif
    
    uint16_t computeCrc(uint8_t data, uint16_t crc = 0) {
        crc ^= data;
        for (int i = 0; i < 8; ++i) {
            unsigned int remainder = (crc & 1) ? CRC_POLYNOMIAL_CCITT : 0;
            crc = (crc >> 1) ^ remainder;
        }
        return crc;
    }

    /*
    Returns the CRC value for the given data frame.
    Used for whole io-homecontrol frames integrity check
    */
    uint16_t radioPacketComputeCrc(uint8_t *buffer, uint8_t bufferLength) {
        uint16_t crc = 0;

        for (uint8_t i=0; i<bufferLength; i++)
            crc = computeCrc(buffer[i], crc);

        return crc;
    }

    /*
    Returns the CRC value for the given data frame.
    Used for whole io-homecontrol frames integrity check
    */
    uint16_t radioPacketComputeCrc(std::vector<uint8_t>& buffer) {
        uint16_t crc = 0;

        for (uint8_t buf : buffer)
            crc = computeCrc(buf, crc);

        return crc;
    }

    std::tuple<uint8_t, uint8_t> computeChecksum(uint8_t frame_byte, uint8_t chksum1, uint8_t chksum2) {
        uint8_t tmpchksum = frame_byte ^ chksum2;
        chksum2 = ((chksum1 & 0x7f)<<1) & 0xff;
        if (tmpchksum >= 0x80)
            chksum2 |= 1;

        if ((chksum1 & 0x80) == 0)
            return std::make_tuple(chksum2, (tmpchksum<<1)&0xff);

        return std::make_tuple(chksum2^0x55, ((tmpchksum<<1)^0x5b)&0xff);
    }

    std::vector<uint8_t> constructInitialValue(const std::vector<uint8_t>& frame_data, const uint8_t *challenge = nullptr, const uint8_t *sequence_number = nullptr) {
        if (!challenge && !sequence_number) {
            printf("Cannot create initial value: no mode selected\n");
            return {};
        }

        std::vector<uint8_t> initial_value(16, 0);
        initial_value[8] = 0;
        initial_value[9] = 0;
        size_t i = 0;
        while (i < frame_data.size()) {
            std::tie(initial_value[8], initial_value[9]) = computeChecksum(frame_data[i], initial_value[8], initial_value[9]);
            if (i < 8)
                initial_value[i] = frame_data[i];
            i++;
        }

        if (i < 8)
            for (size_t j = i; j < 8; j++)
                initial_value[j] = 0x55;

        if (!challenge && sequence_number) {
            for (i = 12; i < 16; i++)
                initial_value[i] = 0x55;

            initial_value[10] = sequence_number[0];
            initial_value[11] = sequence_number[1];
        }
        else if (challenge) {
            for (i = 10; i < 16; i++)
                initial_value[i] = challenge[i - 10];
        }

        return initial_value;
    }

/*
    Calculate HMAC using as input:
    - Packet Sequence Number
    - Controller key in clear
    - frame data starting from Command byte
*/
    void create_1W_hmac(uint8_t *hmac, const uint8_t *seq_number, uint8_t *controller_key, const std::vector<uint8_t>& frame_data) {
        std::vector<uint8_t> iv;
        #if defined(HELTEC)
            mbedtls_aes_init(&aes);
        #endif

        iv = constructInitialValue(frame_data, nullptr, seq_number);

        #if defined(ESP8266)
            aes128.setKey(controller_key, 16);
            aes128.encryptBlock(hmac, iv.data());
        #elif defined(ESP32)
            mbedtls_aes_setkey_enc( &aes, controller_key, 128 );
           
//        for (uint8_t a=0; a<16; a++){
//            hmac[a] = 0;
//        }
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, iv.data(), hmac);
//        mbedtls_aes_free( &aes );
        #endif        

    }

/*
    Encrypt (or decrypt if called with encrypted) the transmitted key using as input:
    - Node address
    - Key in clear (or encrypted to decrypt)
*/
    void encrypt_1W_key(const uint8_t *node_address, uint8_t *key) {
        #if defined(ESP32)
            mbedtls_aes_init(&aes);
        #endif

        uint8_t btransfer[16];
        uint8_t captured[16] = {};

        hexStringToBytes(transfer_key, btransfer);

        std::vector<uint8_t> iv(16, 0);
        for (int i = 0; i < 13; i += 3) {
            iv[i] = node_address[0];
            iv[i + 1] = node_address[1];
            iv[i + 2] = node_address[2];
        }
        iv[15] = node_address[0];

    /*  
        uint8_t captured[16] = {0};
        aes128.setKey((uint8_t *)btransfer, 16);
        aes128.encryptBlock(captured, iv.data());

        for (int i = 0; i < 16; ++i)
            key[i] ^= captured[i];
    */
        #if defined(ESP8266)
    //  Use CTR encryption instead of AES. It includes xor with the key
            ctraes128.setKey(btransfer, 16);
            ctraes128.setIV(iv.data(), 16);
            ctraes128.setCounterSize(4);
            ctraes128.encrypt(key, key, 16);
        #elif defined(ESP32)
            size_t iv_offset = 0;
            mbedtls_aes_setkey_enc( &aes, (uint8_t *)btransfer, 128 );
            mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_ENCRYPT, 16, &iv_offset, iv.data(), (uint8_t *)key, captured);
            for (int i = 0; i < 16; ++i)
                key[i] = captured[i];
            mbedtls_aes_free( &aes );
        #endif
    }
}