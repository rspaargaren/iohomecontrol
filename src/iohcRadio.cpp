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

#include <esp32-hal-gpio.h>
#include <map>
#include <memory>
#include "esp_log.h"

#include <iohcRadio.h>
#include <utility>
#include <log_buffer.h>
#define LONG_PREAMBLE_MS 1920
#define SHORT_PREAMBLE_MS 40

namespace IOHC {
    iohcRadio *iohcRadio::_iohcRadio = nullptr;
    uint8_t iohcRadio::_flags[2] = {0, 0};
    std::atomic<iohcRadio::RadioState> iohcRadio::radioState = iohcRadio::RadioState::IDLE;
    std::atomic<bool> iohcRadio::txComplete = false;
    volatile bool __g_preamble = false;


    static TaskHandle_t handle_interrupt;
    /**
     * The function `handle_interrupt_task` waits for a notification and then calls the `tickerCounter`
     * function if certain conditions are met.
     *
     * @param pvParameters The `pvParameters` parameter in the `handle_interrupt_task` function is a void
     * pointer that can be used to pass any data or object to the task when it is created. In this specific
     * function, it is being cast to a pointer of type `iohcRadio` and then passed to the
     */
    void IRAM_ATTR handle_interrupt_task(void *pvParameters) {
        uint32_t thread_notification;
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS(655 * 4); // 218.4 );
        while (true) {
            thread_notification = ulTaskNotifyTake(pdTRUE, xMaxBlockTime/*xNoDelay*/); // Attendre la notification
            if (thread_notification &&
                (iohcRadio::radioState == iohcRadio::RadioState::PAYLOAD ||
                 iohcRadio::radioState == iohcRadio::RadioState::PREAMBLE)) {
                iohcRadio::tickerCounter((iohcRadio *) pvParameters);
            }
        }

    }

    /**
     * The function `handle_interrupt_fromisr` reads digital inputs and notifies a thread to wake up when
     * the interrupt service routine is complete.
     */
    void IRAM_ATTR handle_interrupt_fromisr() {
        bool preamble = digitalRead(RADIO_PREAMBLE_DETECTED);
        bool payload = digitalRead(RADIO_PACKET_AVAIL);

        if (payload) {
            // When in TX state DIO0 is mapped to PacketSent, otherwise it
            // signals PayloadReady. Use the current radio state to disambiguate
            // without touching SPI from the ISR.
            //if (iohcRadio::radioState == iohcRadio::RadioState::TX) {
            //    iohcRadio::txComplete = true;
            //    ets_printf("TX: TXDONE detected, flag set\n");
            //    iohcRadio::setRadioState(iohcRadio::RadioState::RX);
            //} else {
                iohcRadio::setRadioState(iohcRadio::RadioState::PAYLOAD);
            //}
        } else if (preamble) {
            iohcRadio::setRadioState(iohcRadio::RadioState::PREAMBLE);
        } else {
            iohcRadio::setRadioState(iohcRadio::RadioState::RX);
        }

        // Notify de RX state machine
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(handle_interrupt, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    iohcRadio::iohcRadio() {
        Radio::initHardware();
        Radio::calibrate();

        Radio::initRegisters(MAX_FRAME_LEN);
        Radio::setCarrier(Radio::Carrier::Deviation, 19200);
        Radio::setCarrier(Radio::Carrier::Bitrate, 38400);
        Radio::setCarrier(Radio::Carrier::Bandwidth, 250);
        Radio::setCarrier(Radio::Carrier::Modulation, Radio::Modulation::FSK);

        // Attach interrupts to Preamble detected and end of packet sent/received
#if defined(RADIO_SX127X)
        attachInterrupt(RADIO_DIO0_PIN, handle_interrupt_fromisr, RISING); //CHANGE); //
        attachInterrupt(RADIO_DIO2_PIN, handle_interrupt_fromisr, RISING); //CHANGE); //
#elif defined(CC1101)
        attachInterrupt(RADIO_PREAMBLE_DETECTED, i_preamble, RISING);
#endif

        // start state machine
        printf("Starting Interrupt Handler...\n");
        BaseType_t task_code = xTaskCreatePinnedToCore(handle_interrupt_task, "handle_interrupt_task", 8192,
                                                       this /*nullptr*//*device*/, /*tskIDLE_PRIORITY*/4,
                                                       &handle_interrupt, /*tskNO_AFFINITY*/xPortGetCoreID());
        if (task_code != pdPASS) {
            printf("ERROR STATEMACHINE Can't create task %d\n", task_code);
            // sx127x_destroy(device);
            return;
        }
    }

    /**
     * @brief The function `iohcRadio::getInstance()` returns a pointer to a single instance of the `iohcRadio`
     * class, creating it if it doesn't already exist.
     *
     * @return An instance of the `iohcRadio` class is being returned.
     */
    iohcRadio *iohcRadio::getInstance() {
        if (!_iohcRadio)
            _iohcRadio = new iohcRadio();
        return _iohcRadio;
    }

    /**
     * The `start` function initializes the radio with specified parameters and sets it to receive mode.
     * 
     * @param num_freqs The `num_freqs` parameter in the `start` function represents the number of
     * frequencies to scan. It is of type `uint8_t`, which means it is an unsigned 8-bit integer. This
     * parameter specifies how many frequencies the radio will scan during operation.
     * @param scan_freqs The `scan_freqs` parameter is an array of `uint32_t` values that represent the
     * frequencies to be scanned during the radio operation. The `start` function initializes the radio
     * with the provided frequencies for scanning.
     * @param scanTimeUs The `scanTimeUs` parameter in the `start` function of the `iohcRadio` class
     * represents the time interval in microseconds for scanning frequencies. If a specific value is
     * provided for `scanTimeUs`, it will be used as the scan interval. Otherwise, the default scan
     * interval defined as
     * @param rxCallback The `rxCallback` parameter is of type `IohcPacketDelegate`, which is a delegate or
     * function pointer that will be called when a packet is received by the radio. It is set to `nullptr`
     * by default if not provided during the function call.
     * @param txCallback The `txCallback` parameter in the `start` function of the `iohcRadio` class is of
     * type `IohcPacketDelegate`. It is a callback function that will be called when a packet is
     * transmitted by the radio. This callback function can be provided by the user of the `
     */
    void iohcRadio::start(uint8_t num_freqs, uint32_t *scan_freqs, uint32_t scanTimeUs,
                          IohcPacketDelegate rxCallback = nullptr, IohcPacketDelegate txCallback = nullptr) {
        this->num_freqs = num_freqs;
        this->scan_freqs = scan_freqs;
        this->scanTimeUs = scanTimeUs ? scanTimeUs : DEFAULT_SCAN_INTERVAL_US;
        this->rxCB = std::move(rxCallback);
        this->txCB = std::move(txCallback);

        Radio::clearBuffer();
        Radio::clearFlags();
        /* We always start at freq[0] the 1W/2W channel*/
        Radio::setCarrier(Radio::Carrier::Frequency, scan_freqs[0]); //868950000);
        // Radio::calibrate();
        Radio::setRx();
    }

    /**
     * The `tickerCounter` function in C++ handles various radio operations based on different conditions
     * and configurations for SX127X and CC1101 radios.
     * 
     * @param radio The `radio` parameter in the `iohcRadio::tickerCounter` function is a pointer to an
     * instance of the `iohcRadio` class. This pointer is used to access and modify the properties and
     * methods of the `iohcRadio` object within the function. The function uses this pointer
     * 
     * @return In the provided code snippet, the function `tickerCounter` is returning different values
     * based on the conditions met within the function. Here is a breakdown of the possible return
     * scenarios:
     */
    void IRAM_ATTR iohcRadio::tickerCounter(iohcRadio *radio) {
        // Not need to put in IRAM as we reuse task for µs instead ISR
#if defined(RADIO_SX127X)
        Radio::readBytes(REG_IRQFLAGS1, _flags, sizeof(_flags));

        // If Int of PayLoad
        if (radioState == iohcRadio::RadioState::PAYLOAD) {
            // if TX ready?
            if (_flags[0] & RF_IRQFLAGS1_TXREADY) {
                // Radio::clearFlags();
                // if (radioState != iohcRadio::RadioState::TX) {
                //     Radio::setRx();
                //     radio->setRadioState(iohcRadio::RadioState::RX);
                // }
                return;
            }
            // if in RX mode?
            radio->receive(false);
            Radio::clearFlags();
            radio->tickCounter = 0;
            radio->preCounter = 0;
            return;
        }

        if (radioState == iohcRadio::RadioState::PREAMBLE) {
            radio->tickCounter = 0;
            radio->preCounter = radio->preCounter + 1;
            //radio->preCounter += 1;

            //            if (_flags[0] & RF_IRQFLAGS1_SYNCADDRESSMATCH) radio->preCounter = 0;
            // In case of Sync received resets the preamble duration
            if ((radio->preCounter * SM_GRANULARITY_US) >= SM_PREAMBLE_RECOVERY_TIMEOUT_US) {
                // Avoid hanging on a too long preamble detect
                Radio::clearFlags();
                radio->preCounter = 0;
            }
        }

        if (radioState != iohcRadio::RadioState::RX) return;

        //if (++radio->tickCounter * SM_GRANULARITY_US < radio->scanTimeUs) return;
        radio->tickCounter = radio->tickCounter + 1;
        if (radio->tickCounter * SM_GRANULARITY_US < radio->scanTimeUs) return;

        radio->tickCounter = 0;

        if (radio->num_freqs == 1) return;

        radio->currentFreqIdx += 1;
        if (radio->currentFreqIdx >= radio->num_freqs)
            radio->currentFreqIdx = 0;

        Radio::setCarrier(Radio::Carrier::Frequency, radio->scan_freqs[radio->currentFreqIdx]);

#elif defined(CC1101)
        if (__g_preamble){
            radio->receive();
            radio->tickCounter = 0;
            radio->preCounter = 0;
            return;
        }

        if (radioState != iohcRadio::RadioState::RX)
            return;

        if ((++radio->tickCounter * SM_GRANULARITY_US) < radio->scanTimeUs)
            return;
#endif
    }

    /**
     * The `send` function in the `iohcRadio` class sends packets stored in a vector with a specified
     * repeat time.
     *
     * @param iohcTx `iohcTx` is a reference to a vector of pointers to `iohcPacket` objects. Ownership of the iohcPacket is transfered to this function.
     *
     * @return If `txMode` is true, the `send` function will return early without executing the rest of the
     * code inside the function.
     */
    void iohcRadio::send(std::vector<iohcPacket *> &iohcTx) {
        queueSend(iohcTx);
        startQueuedSend();
    }

    /**
     * The `send` function in the `iohcRadio` class sends the packet with a specified repeat time.
     *
     * @param iohcTx `iohcTx` is a pointer to a `iohcPacket` object. Ownership of the iohcPacket is transfered to this function.
     *
     * @return If `txMode` is true, the `send` function will return early without executing the rest of the
     * code inside the function.
     */
    void iohcRadio::send(iohcPacket *iohcTx) {
        std::vector<iohcPacket *> packets = { iohcTx };
        send(packets);
    }

    static portMUX_TYPE s_mutex = portMUX_INITIALIZER_UNLOCKED;
    template<typename T>
    T iohcRadio::accessInMutex(std::function<T()> handler) {
        taskENTER_CRITICAL(&s_mutex);
        T result = handler();
        taskEXIT_CRITICAL(&s_mutex);
        return result;
    }

    void iohcRadio::queueSend(std::vector<iohcPacket *> &iohcTx) {
        if (iohcTx.empty()) {
            return;
        }
        auto size = accessInMutex<int>([&iohcTx,this]() {
            sendQueue.push(std::move(iohcTx));
            return static_cast<int>(sendQueue.size());
        });
        
        ets_printf("TX: Queued send batch. Queue depth=%d\n", size);
    }

    void iohcRadio::transmitPacket(uint16_t preambleLen, iohcPacket *iohc) {
        digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
        Radio::setPreambleLength(preambleLen);
        Radio::setStandby();
        Radio::clearFlags();
        Radio::writeBytes(REG_FIFO, iohc->payload.buffer, iohc->buffer_length);
        Radio::setTx();
        setRadioState(RadioState::TX); 

        //packetStamp = esp_timer_get_time();
        //iohc->decode(true); //false);
        //IOHC::lastSendCmd = iohc->payload.packet.header.cmd;

        ets_printf("TX: Sending packet: %s\n", iohc->decodeToString(true).c_str());

        digitalWrite(RX_LED, false);
    }

    void iohcRadio::startQueuedSend() {
        auto canStartOnNextInQueue = accessInMutex<bool>([this]() { return !packets2send.empty() || sendQueue.empty(); });
        if (radioState == RadioState::TX || canStartOnNextInQueue) {
            return;
        }

        auto iohc = accessInMutex<iohcPacket *>([this]() {
            for(const auto& e: sendQueue.front())
                packets2send.push(e);
            sendQueue.pop();
            return packets2send.front();
        });

        ets_printf("TX: Preparing %d packet(s)\n", packets2send.size());

        txComplete = false;

        // 🟢 Sent package with short preamble for first packet
        transmitPacket(SHORT_PREAMBLE_MS, iohc);

        // Start ticker for repeats (short preamble)
        Sender.attach_ms(iohc->repeatTime, &iohcRadio::onTxTicker, (void*)this);
    }

    void iohcRadio::onTxTicker(void *arg) {
        iohcRadio *radio = (iohcRadio *)arg;

        // 🩵 Check IRQFLAGS2 (0x3F) for PacketSent in FSK mode (indicates previous transmit is complete)
        uint8_t irqFlags2 = Radio::readByte(REG_IRQFLAGS2); // REG_IRQFLAGS2
        if (irqFlags2 & RF_IRQFLAGS2_PACKETSENT) { // Bit 3 == PacketSent (TXDONE in FSK)
            Radio::writeByte(REG_IRQFLAGS2, RF_IRQFLAGS2_PACKETSENT); // Clear PacketSent bit
            iohcRadio::txComplete = true;
        }

        // ⏳ Wait for TXDONE
        if (!radio->txComplete) {
            ets_printf("TX: Waiting for TXDONE... (state=%s)\n", radioStateToString(radio->radioState));
            return;
        }

        // ✅ TXDONE received
        ESP_LOGD("RADIO", "TXDONE flag set, ready to send repeat or next packet.\n");

        auto iohc = radio->accessInMutex<iohcPacket *>([&radio]() { 
            return radio->packets2send.front();
        });

        radio->sent(iohc);

        radio->txComplete = false;
        // 🔁 Repeat logic
        if (iohc->repeat > 0) {
            iohc->repeat--;
            ets_printf("TX: Repeating current packet (%d repeats left)\n", iohc->repeat);
        
            // 📡 Send packet (short preamble)
            radio->transmitPacket(SHORT_PREAMBLE_MS, iohc);
        } else {
            // No more repeats, advance to next packet when available
            delete iohc;

            auto nextIohc = radio->accessInMutex<iohcPacket *>([&radio]() { 
                radio->packets2send.pop();
                if (!radio->packets2send.empty()) {
                    return radio->packets2send.front();
                }
                return static_cast<iohcPacket *>(nullptr);
            });
 
            if (nextIohc == nullptr) {
                // 👇 Only go RX after all packets
                ets_printf("TX: All repeats done. Switching to RX\n");

                radio->Sender.detach();
                Radio::setRx();
                radio->setRadioState(RadioState::RX);
                radio->startQueuedSend();
            } else {
                ets_printf("TX: Moving to next packet\n");
     
                // 📡 Send packet (short preamble)
                radio->transmitPacket(SHORT_PREAMBLE_MS, nextIohc);

                radio->Sender.detach();
                radio->Sender.attach_ms(nextIohc->repeatTime, &iohcRadio::onTxTicker, (void*)radio);
            }
        }
    }

    /**
     * The `sent` function in the `iohcRadio` class checks if a callback function `txCB` is set and calls
     * it with a packet as a parameter, returning the result.
     * 
     * @param packet The `packet` parameter is a pointer to an object of type `iohcPacket`.
     * 
     * @return The `sent` function is returning a boolean value, which is determined by the result of
     * calling the `txCB` function with the `packet` parameter. If `txCB` is not null, the return value
     * will be the result of calling `txCB(packet)`, otherwise it will be `false`.
     */
    bool IRAM_ATTR iohcRadio::sent(iohcPacket *packet) {
        bool ret = false;
        if (packet) {
            packetStamp = esp_timer_get_time();
            packet->decode(true);
            addLogMessage(String(packet->decodeToString(true).c_str()));
        }
        if (txCB) {
            ret = txCB(packet);
        }
        return ret;
    }

    //    static uint8_t RF96lnaMap[] = { 0, 0, 6, 12, 24, 36, 48, 48 };
    /**
     * The `iohcRadio::receive` function in C++ toggles an LED, reads radio data, processes it, and
     * triggers a callback function.
     * 
     * @param stats The `stats` parameter in the `iohcRadio::receive` function is a boolean parameter that
     * is used to determine whether to gather additional statistics during the radio reception process. If
     * `stats` is set to `true`, the function will collect and process additional information such as RSSI
     * (Received Signal
     * 
     * @return The function `iohcRadio::receive` is returning a boolean value `true`.
     */
    bool IRAM_ATTR iohcRadio::receive(bool stats = false) {
        digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
        // bool frmErr = false;
        auto iohc = std::make_unique<iohcPacket>();
        iohc->buffer_length = 0;
        iohc->frequency = scan_freqs[currentFreqIdx];

        packetStamp = esp_timer_get_time();
#if defined(RADIO_SX127X)
        if (stats) {
            iohc->rssi = static_cast<float>(Radio::readByte(REG_RSSIVALUE)) / -2.0f;
            int16_t thres = Radio::readByte(REG_RSSITHRESH);
            iohc->snr = iohc->rssi > thres ? 0 : (thres - iohc->rssi);
            //            iohc->lna = RF96lnaMap[ (Radio::readByte(REG_LNA) >> 5) & 0x7 ];
            int16_t f = (uint16_t) Radio::readByte(REG_AFCMSB);
            f = (f << 8) | (uint16_t) Radio::readByte(REG_AFCLSB);
            //            iohc->afc = f * (32000000.0 / 524288.0); // static_cast<float>(1 << 19));
            iohc->afc = /*(int32_t)*/f * 61.0;
            //            iohc->rssiAt = micros();
        }
#elif defined(CC1101)
        __g_preamble = false;

        uint8_t tmprssi=Radio::SPIgetRegValue(REG_RSSI);
        if (tmprssi>=128)
            iohc->rssi = (float)((tmprssi-256)/2)-74;
        else
            iohc->rssi = (float)(tmprssi/2)-74;

        uint8_t bytesInFIFO = Radio::SPIgetRegValue(REG_RXBYTES, 6, 0);
        size_t readBytes = 0;
        uint32_t lastPop = millis();
#endif

#if defined(RADIO_SX127X)

        while (Radio::dataAvail()) {
            iohc->payload.buffer[iohc->buffer_length++] = Radio::readByte(REG_FIFO);
            if (iohc->buffer_length == 32) {
                break; // avoid buffer overflow
            }
        }

#elif defined(CC1101)
        uint8_t lenghtFrameCoded = 0xFF;
        uint8_t tmpBuffer[64]={0x00};
        while (readBytes < lenghtFrameCoded) {
            if ( (readBytes>=1) && (lenghtFrameCoded==0xFF) ){ // Obtain frame lenght
                lenghtFrame = (Radio::reverseByte( ((uint8_t)(tmpBuffer[0]<<4) | (uint8_t)(tmpBuffer[1]>>4)))) & 0b00011111;
                lenghtFrameCoded = ((lenghtFrame + 2 + 1)*8) + ((lenghtFrame + 2 + 1)*2);   // Calculate Num of bits of encoded frame (add 2 bit per byte)
                lenghtFrameCoded = ceil((float)lenghtFrameCoded/8);                         // divide by 8 bits per byte and round to up
                Radio::setPktLenght(lenghtFrameCoded);
                //Serial.printf("BytesReaded: %d\tlenghtFrame: 0x%d\t lenghtFrameCoded: 0x%d\n", readBytes, lenghtFrame,  lenghtFrameCoded);
            }

            if (bytesInFIFO == 0) {
                if (millis() - lastPop > 5) {
                    // readData was required to read a packet longer than the one received.
                    //Serial.println("No data for more than 5mS. Stop here.");
                    break;
                } else {
                    delay(1);
                    bytesInFIFO = Radio::SPIgetRegValue(REG_RXBYTES, 6, 0);
                    continue;
                }
            }

            // read the minimum between "remaining length" and bytesInFifo
            uint8_t bytesToRead = (((uint8_t)(lenghtFrameCoded - readBytes))<(bytesInFIFO)?((uint8_t)(lenghtFrameCoded - readBytes)):(bytesInFIFO));
            Radio::SPIreadRegisterBurst(REG_FIFO, bytesToRead, &(tmpBuffer[readBytes]));
            readBytes += bytesToRead;
            lastPop = millis();

            // Get how many bytes are left in FIFO.
            bytesInFIFO = Radio::SPIgetRegValue(REG_RXBYTES, 6, 0);
        }


        frmErr=true;
        if (lenghtFrameCoded<255){
            int8_t lenFuncDecodeFrame = Radio::decodeFrame(tmpBuffer, lenghtFrameCoded);
            if (lenFuncDecodeFrame>0 && lenFuncDecodeFrame<=MAX_FRAME_LEN){
                if (iohcUtils::radioPacketComputeCrc(tmpBuffer, lenFuncDecodeFrame) == 0 ){
                    iohc->buffer_length = lenFuncDecodeFrame;
                    memcpy(iohc->payload.buffer, tmpBuffer, lenFuncDecodeFrame);  // volcamos el resultado al array de origen
                    frmErr=false;
                }
            }
        }

        // Flush then standby according to RXOFF_MODE (default: RADIOLIB_CC1101_RXOFF_IDLE)
        if (Radio::SPIgetRegValue(REG_MCSM1, 3, 2) == RF_RXOFF_IDLE) {
            Radio::SPIsendCommand(CMD_IDLE);                    // set mode to standby
            Radio::SPIsendCommand(CMD_FLUSH_RX | CMD_READ);     // flush Rx FIFO
        }

        Radio::SPIsendCommand(CMD_RX);
        setRadioState(iohcRadio::RadioState::RX);

#endif

        // Radio::clearFlags();
        if (rxCB) rxCB(iohc.get());
        iohc->decode(true); //stats);
        addLogMessage(String(iohc->decodeToString(true).c_str()));

        digitalWrite(RX_LED, false);
        return true;
    }

    /**
     * The `i_preamble` interrupt handler updates the radio state when a preamble is
     * detected on the current channel.
     */
    void IRAM_ATTR iohcRadio::i_preamble() {
        __g_preamble = true;
        iohcRadio::setRadioState(iohcRadio::RadioState::PREAMBLE);
    }

    const char* iohcRadio::radioStateToString(RadioState state) {
        switch (state) {
            case RadioState::IDLE:     return "IDLE";
            case RadioState::RX:       return "RX";
            case RadioState::TX:       return "TX";
            case RadioState::PREAMBLE: return "PREAMBLE";
            case RadioState::PAYLOAD:  return "PAYLOAD";
            case RadioState::LOCKED:   return "LOCKED";
            case RadioState::ERROR:    return "ERROR";
            default:                   return "UNKNOWN";
        }
    }


    void IRAM_ATTR iohcRadio::setRadioState(RadioState newState) {
        radioState = newState;
        // Optional debug:
        //printf("State changed to: %d\n", static_cast<int>(newState));
        ets_printf("State: %s\n", radioStateToString(newState));
    }
}
