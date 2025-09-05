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
#include "esp_log.h"

#include <iohcRadio.h>
#include <utility>
#include <log_buffer.h>
#define LONG_PREAMBLE_MS 1920
#define SHORT_PREAMBLE_MS 40

TaskHandle_t IOHC::iohcRadio::txTaskHandle = nullptr;

namespace IOHC {
    iohcRadio *iohcRadio::_iohcRadio = nullptr;
    volatile bool iohcRadio::_g_preamble = false;
    volatile bool iohcRadio::_g_payload = false;
    volatile unsigned long iohcRadio::_g_payload_millis = 0L;
    uint8_t iohcRadio::_flags[2] = {0, 0};
    volatile bool iohcRadio::f_lock = false;
    volatile bool iohcRadio::send_lock = false;
    volatile bool iohcRadio::txMode = false;
    volatile iohcRadio::RadioState iohcRadio::radioState = iohcRadio::RadioState::IDLE;
    volatile bool iohcRadio::txComplete = false;

    static IOHC::iohcPacket last1wPacket;
    static bool last1wPacketValid = false;

    TaskHandle_t handle_interrupt;
    /**
     * The function `handle_interrupt_task` waits for a notification and then calls the `tickerCounter`
     * function if certain conditions are met.
     *
     * @param pvParameters The `pvParameters` parameter in the `handle_interrupt_task` function is a void
     * pointer that can be used to pass any data or object to the task when it is created. In this specific
     * function, it is being cast to a pointer of type `iohcRadio` and then passed to the
     */
    void IRAM_ATTR handle_interrupt_task(void *pvParameters) {
        static uint32_t thread_notification;
        constexpr TickType_t xMaxBlockTime = pdMS_TO_TICKS(655 * 4); // 218.4 );
        while (true) {
            thread_notification = ulTaskNotifyTake(pdTRUE, xMaxBlockTime/*xNoDelay*/); // Attendre la notification
            if (thread_notification &&
                // (iohcRadio::_g_payload ||
                    // iohcRadio::_g_preamble)) {
                (iohcRadio::radioState == iohcRadio::RadioState::PAYLOAD ||
                 iohcRadio::radioState == iohcRadio::RadioState::PREAMBLE)) {

                iohcRadio::tickerCounter(static_cast<iohcRadio *>(pvParameters));
            }
        }
    }

    /**
     * The function `handle_interrupt_fromisr` reads digital inputs and notifies a thread to wake up when
     * the interrupt service routine is complete.
     */
    void IRAM_ATTR handle_interrupt_fromisr(/*void *arg*/) {
        iohcRadio::_g_preamble = digitalRead(RADIO_PREAMBLE_DETECTED);
        iohcRadio::f_lock = iohcRadio::_g_preamble;
iohcRadio::setRadioState(iohcRadio::_g_preamble ? iohcRadio::RadioState::PREAMBLE : iohcRadio::RadioState::RX);
        iohcRadio::_g_payload = digitalRead(RADIO_PACKET_AVAIL);
iohcRadio::setRadioState(iohcRadio::_g_payload ? iohcRadio::RadioState::PAYLOAD : iohcRadio::RadioState::RX);
        iohcRadio::txComplete = true;
        // Notify the thread so it will wake up when the ISR is complete
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(handle_interrupt/*_task*/, &xHigherPriorityTaskWoken);
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
        /* TODO this is wrongly named and/or assigned, but work like that*/
        //        printf("Starting TickTimer Handler...\n");
        //        TickTimer.attach_us(SM_GRANULARITY_US/*SM_GRANULARITY_MS*/, tickerCounter, this);

        //        attachInterrupt(RADIO_PACKET_AVAIL, i_payload, CHANGE); //
        //        attachInterrupt(RADIO_PREAMBLE_DETECTED, i_preamble, CHANGE); //
        attachInterrupt(RADIO_DIO0_PIN, handle_interrupt_fromisr, RISING); //CHANGE); //
        //        attachInterrupt(RADIO_DIO1_PIN, handle_interrupt_fromisr, RISING); // CHANGE); //
        attachInterrupt(RADIO_DIO2_PIN, handle_interrupt_fromisr, RISING); //CHANGE); //

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
                          CallbackFunction rxCallback = nullptr, CallbackFunction txCallback = nullptr) {
        this->num_freqs = num_freqs;
        this->scan_freqs = scan_freqs;
        this->scanTimeUs = scanTimeUs ? scanTimeUs : DEFAULT_SCAN_INTERVAL_US;
        this->rxCB = std::move(rxCallback);
        this->txCB = std::move(txCallback);

        Radio::clearBuffer();
        Radio::clearFlags();
        /* We always start at freq[0], the 1W/2W channel*/
        Radio::setCarrier(Radio::Carrier::Frequency, scan_freqs[0]); //868950000);
        // Radio::calibrate();
        Radio::setRx();
    }

/**
 * The `tickerCounter` function in C++ handles various radio operations based on different conditions
 * and configurations for SX127X
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

        Radio::readBytes(REG_IRQFLAGS1, _flags, sizeof(_flags));

        // If Int of PayLoad
        if (radioState == iohcRadio::RadioState::PAYLOAD) {
        // if (_g_payload) {
            // if TX ready?
            if (_flags[0] & RF_IRQFLAGS1_TXREADY) {
                radio->sent(radio->iohc);
                Radio::clearFlags();
                // if (radioState != iohcRadio::RadioState::TX) {
                if (!txMode) {
                    Radio::setRx();
                    radio->setRadioState(iohcRadio::RadioState::RX);
                    f_lock = false;
                }
                // radio->sent(radio->iohc); // Put after Workaround to permit MQTT sending. No more needed
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
        // if (_g_preamble) {
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

        // if (radioState != iohcRadio::RadioState::RX) return;
        if (f_lock) return;

        radio->tickCounter = radio->tickCounter + 1;
        if (radio->tickCounter * SM_GRANULARITY_US < radio->scanTimeUs) return;

        radio->tickCounter = 0;

        if (radio->num_freqs == 1) return;

        radio->currentFreqIdx += 1;
        if (radio->currentFreqIdx >= radio->num_freqs)
            radio->currentFreqIdx = 0;

        Radio::setCarrier(Radio::Carrier::Frequency, radio->scan_freqs[radio->currentFreqIdx]);

    }

    /**
     * The `send` function in the `iohcRadio` class sends packets stored in a vector with a specified
     * repeat time.
     *
     * @param iohcTx `iohcTx` is a reference to a vector of pointers to `iohcPacket` objects.
     *
     * @return If `txMode` is true, the `send` function will return early without executing the rest of the
     * code inside the function.
     */
    void iohcRadio::send(std::vector<iohcPacket *> &iohcTx) {
        // if (radioState == iohcRadio::RadioState::TX) return;
        if (txMode) return;

        packets2send = iohcTx; //std::move(iohcTx); //
        iohcTx.clear();

        txCounter = 0;
        setRadioState(iohcRadio::RadioState::TX);
        Sender.attach_ms(packets2send[txCounter]->repeatTime, packetSender, this);
    }

    void iohcRadio::send_2(std::vector<iohcPacket *> &iohcTx) {
        if (radioState == RadioState::TX) {
            ets_printf("TX: Already transmitting. Ignoring send()\n");
            return;
        }

        packets2send = std::move(iohcTx);
        txCounter = 0;
        ets_printf("TX: Preparing %d packet(s)\n", packets2send.size());
        setRadioState(RadioState::TX);

        iohc = packets2send[txCounter];

        // 🟢 Set long preamble for first packet
        Radio::setPreambleLength(LONG_PREAMBLE_MS);
        ets_printf("TX: Using LONG preamble (%d ms)\n", LONG_PREAMBLE_MS);

        // Send first packet immediately
        Radio::setStandby();
        Radio::clearFlags();
        Radio::writeBytes(REG_FIFO, iohc->payload.buffer, iohc->buffer_length);
        Radio::setTx();

        ets_printf("TX: Sent first packet at %llu us\n", esp_timer_get_time());

        if (iohc->repeat > 0) iohc->repeat--;

        // Start ticker for repeats (short preamble)
        Sender.attach_ms(iohc->repeatTime, &iohcRadio::onTxTicker, (void*)this);
    }

    void iohcRadio::onTxTicker(void *arg) {
        const auto radio = static_cast<iohcRadio *>(arg);

        // 🩵 Fallback: Check IRQFLAGS2 (0x3F) for PacketSent in FSK mode
        uint8_t irqFlags2 = Radio::readByte(0x3F); // REG_IRQFLAGS2
        if (irqFlags2 & 0x08) { // Bit 3 == PacketSent (TXDONE in FSK)
            ets_printf("FSK: Detected PacketSent (TXDONE) via register (ISR missed?)\n");
            Radio::writeByte(0x3F, 0x08); // Clear PacketSent bit
            iohcRadio::txComplete = true;
        }

        // 🛑 Check if all packets are sent
        if (radio->txCounter >= radio->packets2send.size()) {
            ets_printf("TX: All packets sent. Stopping Ticker.\n");
            radio->Sender.detach();
            Radio::setRx();
            radio->setRadioState(RadioState::RX);
            return;
        }

        // ⏳ Wait for TXDONE
        if (!radio->txComplete) {
            ets_printf("TX: Waiting for TXDONE... (state=%s)\n", radioStateToString(radio->radioState));
            return;
        }

        // ✅ TXDONE received
        radio->txComplete = false;
        ets_printf("TX: TXDONE flag set, ready to send repeat or next packet.\n");
        ESP_LOGD("RADIO", "TX: TXDONE flag set, LOGD.\n");

        // 🔁 Repeat logic
        if (radio->iohc->repeat > 0) {
            radio->iohc->repeat--;
            ets_printf("TX: Repeating current packet (%d repeats left)\n", radio->iohc->repeat);
        } else {
            radio->txCounter = radio->txCounter + 1;
            if (radio->txCounter < radio->packets2send.size()) {
                radio->iohc = radio->packets2send[radio->txCounter];
                ets_printf("TX: Moving to next packet %d/%d (repeat=%d)\n",
                           radio->txCounter + 1,
                           radio->packets2send.size(),
                           radio->iohc->repeat);
            }
        }

        // 👇 Only go RX after all packets
        if (radio->txCounter >= radio->packets2send.size()) {
            ets_printf("TX: All repeats done. Switching to RX\n");
            radio->Sender.detach();
            Radio::setRx();
            radio->setRadioState(RadioState::RX);
            return;
        } else {
            radio->setRadioState(RadioState::TX); // Stay TX until done
        }

        // 📡 Send next packet (short preamble)
        Radio::setPreambleLength(SHORT_PREAMBLE_MS);
        Radio::setStandby();
        Radio::clearFlags();
        Radio::writeBytes(REG_FIFO, radio->iohc->payload.buffer, radio->iohc->buffer_length);
        Radio::setTx();

        ets_printf("TX: Sent packet %d/%d at %llu us\n",
                   radio->txCounter + 1,
                   radio->packets2send.size(),
                   esp_timer_get_time());
    }

    void iohcRadio::lightTxTask(void *pvParameters) {
        const auto radio = static_cast<iohcRadio *>(pvParameters);
        while (true) {
            // Wacht tot er werk is
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            while (radio->txCounter < radio->packets2send.size()) {
                Radio::setStandby();
                Radio::clearFlags();
                Radio::writeBytes(REG_FIFO, radio->iohc->payload.buffer, radio->iohc->buffer_length);
                Radio::setTx();
                ets_printf("TX: lightTxTask sent packet at %llu us\n", esp_timer_get_time());

                if (radio->iohc->repeat > 0) radio->iohc->repeat--;

                if (radio->iohc->repeat == 0) radio->txCounter = radio->txCounter + 1;

                if (radio->iohc->repeatTime > 0)
                    vTaskDelay(pdMS_TO_TICKS(radio->iohc->repeatTime));
            }

            // Alles verzonden
            radio->packets2send.clear();
            Radio::setRx();
            radio->setRadioState(iohcRadio::RadioState::RX);
        }
    }

    void iohcRadio::sendAuto(std::vector<iohcPacket *> &iohcTx) {
         if (radioState == RadioState::TX) {
             ets_printf("TX: Already transmitting. Ignoring sendAuto()\n");
             return;
         }

         packets2send = std::move(iohcTx);
         txCounter = 0;
         ets_printf("TX: Preparing %d packet(s) for AutoTxRx\n", packets2send.size());
         setRadioState(RadioState::TX);

         // Configure AutoTxRx
         configureAutoTxRx(packets2send[txCounter]);

         ets_printf("TX: AutoTxRx started\n");
     }

    void iohcRadio::configureAutoTxRx(iohcPacket *packet) {
        ets_printf("TX: Configuring AutoTxRx for repeat=%d interval=%dms\n", packet->repeat, packet->repeatTime);

        // Set DIO mapping for AutoTxRx (if required)
        Radio::writeByte(REG_DIOMAPPING1, 0x40); // DIO0 = TxDone, DIO1 = RxDone

        // Set packet payload
        Radio::writeBytes(REG_FIFO, packet->payload.buffer, packet->buffer_length);

        // Set repeat count (payload->repeat) and interval (payload->repeatTime)
        //Radio::writeByte(REG_AUTOTX_REPEAT, packet->repeat);
        //Radio::writeByte(REG_AUTOTX_INTERVAL, packet->repeatTime / 10); // 10ms steps

        // Start AutoTxRx
        //Radio::writeByte(REG_OPMODE, RF_OPMODE_AUTOTXRX);
    }

/**
 * The function `packetSender` in the `iohcRadio` class handles the transmission of packets using radio
 * communication, including frequency setting, packet preparation, and handling of repeated
 * transmissions.
 * 
 * @param radio The `radio` parameter in the `packetSender` function is a pointer to an object of type
 * `iohcRadio`. It is used to access and manipulate data and functions within the `iohcRadio` class.
 */
    void IRAM_ATTR iohcRadio::packetSender(iohcRadio *radio) {
        digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
        // Stop frequency hopping
        f_lock = true;
        txMode = true; // Avoid Radio put in Rx mode at next packet sent/received
        // Using Delayed Packet radio->txCounter)
        if (radio->packets2send[radio->txCounter] == nullptr) {
            // Plus de Delayed Packet
            if (radio->delayed != nullptr)
                // Use Saved Delayed Packet
                radio->iohc = radio->delayed;
        } else
            radio->iohc = radio->packets2send[radio->txCounter];

        //        if (radio->iohc->frequency != 0) {
        if (radio->iohc->frequency != radio->scan_freqs[radio->currentFreqIdx]) {
            // printf("ChangedFreq !\n");
            Radio::setCarrier(Radio::Carrier::Frequency, radio->iohc->frequency);
        }
        // else {
        //     radio->iohc->frequency = radio->scan_freqs[radio->currentFreqIdx];
        // }

        Radio::setStandby();
        Radio::clearFlags();

        Radio::writeBytes(REG_FIFO, radio->iohc->payload.buffer, radio->iohc->buffer_length);

        packetStamp = esp_timer_get_time();
        radio->iohc->decode(true); //false);

        IOHC::lastSendCmd = radio->iohc->payload.packet.header.cmd;

        Radio::setTx();
        radio->setRadioState(iohcRadio::RadioState::TX);
        // There is no need to maintain radio locked between packets transmission unless clearly asked
        txMode = radio->iohc->lock;

        if (radio->iohc->repeat)
            radio->iohc->repeat -= 1;
        if (radio->iohc->repeat == 0) {
            radio->Sender.detach();
            //++radio->txCounter;
            radio->txCounter = radio->txCounter + 1;
            if (radio->txCounter < radio->packets2send.size() && radio->packets2send[radio->txCounter] != nullptr) {
                //if (radio->packets2send[++(radio->txCounter)]) {
                if (radio->packets2send[radio->txCounter]->delayed != 0) {
                    radio->delayed = radio->packets2send[radio->txCounter];
                    radio->packets2send[radio->txCounter] = nullptr;
                    radio->Sender.delay_ms(radio->delayed/*radio->packets2send[radio->txCounter]*/->delayed,
                                           packetSender, radio);
                } else {
                    radio->Sender.attach_ms(radio->packets2send[radio->txCounter]->repeatTime, packetSender, radio);
                }
            } else {
                // In any case, after last packet sent, unlock the radio
                txMode = false;
                radio->packets2send.clear();
            }
        }
        digitalWrite(RX_LED, digitalRead(RX_LED) ^ 1);
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
        if (txCB) ret = txCB(packet);
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

        // Create a new packet for this reception. Use a pointer to pass to the callback.
        auto* currentPacket = new iohcPacket();

        currentPacket->buffer_length = 0;
        currentPacket->frequency = scan_freqs[currentFreqIdx];

        _g_payload_millis = esp_timer_get_time();
        packetStamp = _g_payload_millis;

        if (stats) {
            currentPacket->rssi = static_cast<float>(Radio::readByte(REG_RSSIVALUE)) / -2.0f;
            int16_t thres = Radio::readByte(REG_RSSITHRESH);
            currentPacket->snr = currentPacket->rssi > thres ? 0 : (thres - currentPacket->rssi);
            int16_t f = static_cast<uint16_t>(Radio::readByte(REG_AFCMSB));
            f = (f << 8) | static_cast<uint16_t>(Radio::readByte(REG_AFCLSB));
            currentPacket->afc = f * 61.0;
        }

        while (Radio::dataAvail()) {
            if (currentPacket->buffer_length < MAX_FRAME_LEN) {
                currentPacket->payload.buffer[currentPacket->buffer_length++] = Radio::readByte(REG_FIFO);
            } else {
                // Prevent buffer overflow, discard extra bytes.
                Radio::readByte(REG_FIFO);
            }
        }

        if (/*bool is_duplicate = last1wPacketValid &&*/ *currentPacket == last1wPacket) {
            // ets_printf("Same packet, skipping\n");
        } else {
            if (rxCB) rxCB(currentPacket);
            currentPacket->decode(true); //stats);
            // addLogMessage(String(currentPacket->decodeToString(true).c_str()));

            // Save the new packet's data for the next comparison
            last1wPacket = *currentPacket;
            /*last1wPacketValid = true;*/
        }

        delete currentPacket; // The packet is processed or duplicated, we can free it.

        digitalWrite(RX_LED, false);
        return true;
    }

/**
 * The function `i_preamble` sets the value of `f_lock` based on the state of `_g_preamble`
 */
    // void IRAM_ATTR iohcRadio::i_preamble() {
    //     _g_preamble = digitalRead(RADIO_PREAMBLE_DETECTED);
    //     f_lock = _g_preamble;
    //     iohcRadio::setRadioState(_g_preamble ? iohcRadio::RadioState::PREAMBLE : iohcRadio::RadioState::RX);
    // }

/**
 * The function `iohcRadio::i_payload()` reads the value of a digital pin and stores it in a variable
 * `_g_payload`
 */
    // void IRAM_ATTR iohcRadio::i_payload() {
    //     _g_payload = digitalRead(RADIO_PACKET_AVAIL);
    //     iohcRadio::setRadioState(_g_payload ? iohcRadio::RadioState::PAYLOAD : iohcRadio::RadioState::RX);
    // }

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


    void IRAM_ATTR iohcRadio::setRadioState(const RadioState newState) {
        radioState = newState;
    }
}
