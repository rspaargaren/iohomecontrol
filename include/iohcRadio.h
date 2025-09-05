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

#ifndef IOHC_RADIO_H
#define IOHC_RADIO_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Delegate.h>
#include <cstdint>

#include <board-config.h>
#include <iohcCryptoHelpers.h>
#include <iohcPacket.h>

#if defined(RADIO_SX127X)
        #include <SX1276Helpers.h>
#endif
#if defined(RADIO_SX126X)
        #include <SX126xHelpers.h>
#endif
#if defined(ESP32)
    #include <TickerUsESP32.h>
#endif

#define SM_GRANULARITY_US               130ULL  // Ticker function frequency in uS (100 minimum) 4 x 26µs = 104
#define SM_GRANULARITY_MS               1       // Ticker function frequency in uS
#define SM_PREAMBLE_RECOVERY_TIMEOUT_US 1378 // 12500   // SM_GRANULARITY_US * PREAMBLE_LSB //12500   // Maximum duration in uS of Preamble before reset of receiver
#define DEFAULT_SCAN_INTERVAL_US        13520   // Default uS between frequency changes

/*
    Singleton class to implement an IOHC Radio abstraction layer for controllers.
    Implements all needed functionalities to receive and send packets from/to the air, masking complexities related to frequency hopping
    IOHC timings, async sending and receiving through callbacks, ...
*/
namespace IOHC {
    using CallbackFunction = Delegate<bool(iohcPacket *iohc)>;

    class iohcRadio  {
        public:
            static iohcRadio *getInstance();
            virtual ~iohcRadio() = default;
            enum class RadioState : uint8_t {
                IDLE,        ///< Default state: nothing happening
                RX,          ///< Receiving mode
                TX,          ///< Transmitting mode
                PREAMBLE,    ///< Preamble detected
                PAYLOAD,     ///< Payload available
                LOCKED,      ///< Frequency locked
                ERROR        ///< Error or unknown state
            };
            void start(uint8_t num_freqs, uint32_t *scan_freqs, uint32_t scanTimeUs, CallbackFunction rxCallback, CallbackFunction txCallback);
            void send(std::vector<iohcPacket*>&iohcTx);
            void send_2(std::vector<iohcPacket*>&iohcTx);

        void sendAuto(std::vector<iohcPacket*>&iohcTx); // Nouvelle version pour AutoTxRx
        static void setRadioState(RadioState newState);
        static const char* radioStateToString(RadioState state);
        volatile static RadioState radioState;

            volatile static bool _g_preamble;
            volatile static bool _g_payload;
            volatile static bool f_lock;
            static void tickerCounter(iohcRadio *radio);
            static TaskHandle_t txTaskHandle; // TX Task handle
            static volatile bool txComplete;
            //static void setPreambleLength(uint16_t preambleLen);

        private:
            iohcRadio();
            bool receive(bool stats);
            bool sent(iohcPacket *packet);

            static iohcRadio *_iohcRadio;
            static uint8_t _flags[2];
            volatile static unsigned long _g_payload_millis;
            
            volatile static bool send_lock;
            volatile static bool txMode;

            volatile uint32_t tickCounter = 0;
            volatile uint32_t preCounter = 0;
            volatile uint8_t txCounter = 0;
            static void txTaskLoop(void *pvParameters);
            static void lightTxTask(void *pvParameters);
            //TaskHandle_t txTaskHandle = nullptr;
            static void IRAM_ATTR onTxTicker(void *arg);

            uint8_t num_freqs = 0;
            uint32_t *scan_freqs{};
            uint32_t scanTimeUs{};
            uint8_t currentFreqIdx = 0;

        #if defined(ESP32)
            TimersUS::TickerUsESP32 TickTimer;
            TimersUS::TickerUsESP32 Sender;
        #endif
            iohcPacket *iohc{};
            iohcPacket *delayed{};
            
            CallbackFunction rxCB = nullptr;
            CallbackFunction txCB = nullptr;
            std::vector<iohcPacket*> packets2send{};
        protected:
            // static void i_preamble();
            // static void i_payload();
            static void packetSender(iohcRadio *radio);
            static void configureAutoTxRx(iohcPacket *packet); // Fonction auxiliaire pour activer Autotxrx
    };
}

#endif