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

#include <iohcRadio.h>
#include <utility>
#define LONG_PREAMBLE_MS 1920
#define SHORT_PREAMBLE_MS 40

TaskHandle_t IOHC::iohcRadio::txTaskHandle = nullptr;

namespace IOHC {
    iohcRadio *iohcRadio::_iohcRadio = nullptr;
    volatile unsigned long iohcRadio::_g_payload_millis = 0L;
    uint8_t iohcRadio::_flags[2] = {0, 0};
    volatile bool iohcRadio::send_lock = false;
    volatile iohcRadio::RadioState iohcRadio::radioState = iohcRadio::RadioState::IDLE;
    volatile bool iohcRadio::txComplete = false;


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
                iohcRadio::txComplete = true;
                ets_printf("TX: TXDONE detected, flag set\n");
            //    iohcRadio::setRadioState(iohcRadio::RadioState::RX);
            //} else {
                iohcRadio::setRadioState(iohcRadio::RadioState::PAYLOAD);
            //}

            // Notify TX task that TXDONE occurred so the next packet can be
            // scheduled.
            if (iohcRadio::txTaskHandle) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                vTaskNotifyGiveFromISR(iohcRadio::txTaskHandle,
                                      &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
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





    void iohcRadio::txTaskLoop(void *pvParameters) {
    iohcRadio *radio = static_cast<iohcRadio *>(pvParameters);

    while (true) {
        // Wacht tot ISR aangeeft dat TX klaar is
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Delay tussen repeats
        if (radio->iohc && radio->iohc->repeatTime > 0) {
            vTaskDelay(pdMS_TO_TICKS(radio->iohc->repeatTime));
        }

        // Verstuur volgend pakket
        packetSender(radio);

        // Stop de task als alles klaar is
        if (radio->txCounter >= radio->packets2send.size()) {
            ets_printf("TX: Batch complete. Deleting TX task.\n");
            radio->txTaskHandle = nullptr;
            vTaskDelete(nullptr); // delete zichzelf
        }
    }
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
#if defined(RADIO_SX127X)
        //        attachInterrupt(RADIO_PACKET_AVAIL, i_payload, CHANGE); //
        //        attachInterrupt(RADIO_PREAMBLE_DETECTED, i_preamble, CHANGE); //
        attachInterrupt(RADIO_DIO0_PIN, handle_interrupt_fromisr, RISING); //CHANGE); //
        //        attachInterrupt(RADIO_DIO1_PIN, handle_interrupt_fromisr, RISING); // CHANGE); //
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
                radio->sent(radio->iohc);
                Radio::clearFlags();
                if (radioState != iohcRadio::RadioState::TX) {
                    Radio::setRx();
                    radio->setRadioState(iohcRadio::RadioState::RX);
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
     * @param iohcTx `iohcTx` is a reference to a vector of pointers to `iohcPacket` objects.
     *
     * @return If `txMode` is true, the `send` function will return early without executing the rest of the
     * code inside the function.
     */

    /**  
    void iohcRadio::send(std::vector<iohcPacket *> &iohcTx) {
        if (radioState == iohcRadio::RadioState::TX) return;

        packets2send = iohcTx; //std::move(iohcTx); //
        iohcTx.clear();

        txCounter = 0;
        setRadioState(iohcRadio::RadioState::TX);
        Sender.attach_ms(packets2send[txCounter]->repeatTime, packetSender, this);
    }
    */

void iohcRadio::send(std::vector<iohcPacket *> &iohcTx) {
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
    iohcRadio *radio = (iohcRadio *)arg;

    // 🛑 Controleer of alle packets al verzonden zijn
    if (radio->txCounter >= radio->packets2send.size()) {
        ets_printf("TX: All packets sent. Stopping Ticker.\n");
        radio->Sender.detach();
        Radio::setRx();
        radio->setRadioState(RadioState::RX);
        return;
    }

    // ⏳ Wachten tot vorige TX afgerond (TXDONE via ISR)
    if (!radio->txComplete) {
        ets_printf("TX: Waiting for TXDONE... (state=%s)\n",
                   radioStateToString(radio->radioState));
        return; // Blijf wachten tot ISR txComplete zet
    }

    // ✅ TXDONE ontvangen, reset flag
    radio->txComplete = false;
    ets_printf("TX: TXDONE flag set, ready to send repeat or next packet.\n");

    // 🔁 Repeat logica
    if (radio->iohc->repeat > 0) {
        radio->iohc->repeat--;
        ets_printf("TX: Repeating current packet (%d repeats left)\n",
                   radio->iohc->repeat);
    } else {
        radio->txCounter++;
        if (radio->txCounter < radio->packets2send.size()) {
            radio->iohc = radio->packets2send[radio->txCounter];
            ets_printf("TX: Moving to next packet %d/%d (repeat=%d)\n",
                       radio->txCounter + 1,
                       radio->packets2send.size(),
                       radio->iohc->repeat);
        } else {
            ets_printf("TX: Batch complete. Stopping Ticker.\n");
            radio->Sender.detach();
            Radio::setRx();
            radio->setRadioState(RadioState::RX);
            return;
        }
    }

    // 📡 Stuur volgende packet (met korte preamble)
    Radio::setPreambleLength(SHORT_PREAMBLE_MS);
    Radio::setStandby();
    Radio::clearFlags();
    Radio::writeBytes(REG_FIFO,
                      radio->iohc->payload.buffer,
                      radio->iohc->buffer_length);
    Radio::setTx();

    ets_printf("TX: Sent packet %d/%d at %llu us\n",
               radio->txCounter + 1,
               radio->packets2send.size(),
               esp_timer_get_time());
}


 void iohcRadio::lightTxTask(void *pvParameters) {
    iohcRadio *radio = static_cast<iohcRadio *>(pvParameters);
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

            if (radio->iohc->repeat == 0) radio->txCounter++;

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
        ets_printf("TX: Configuring AutoTxRx for repeat=%d interval=%dms\n",
                   packet->repeat, packet->repeatTime);

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
    ets_printf("T1 packetSender() fired at %llu us\n", esp_timer_get_time());
    if (!radio || radio->packets2send.empty()) {
        ets_printf("TX: No packets to send. Forcing cleanup.\n");
        radio->packets2send.clear();
        Radio::setRx(); // Go back to RX only after stop
        radio->setRadioState(iohcRadio::RadioState::RX);
        return;
    }

    // Check if all packets are sent
    if (radio->txCounter >= radio->packets2send.size()) {
        ets_printf("TX: All packets sent in batch.\n");
        if (!radio->iohc || !radio->iohc->lock) {
            ets_printf("TX: Unlocking radio and switching to RX.\n");
            radio->Sender.detach();
            radio->packets2send.clear();
            Radio::setRx();
            radio->setRadioState(iohcRadio::RadioState::RX);
        } else {
            ets_printf("TX: Lock is active, keeping radio in STANDBY.\n");
            radio->txCounter = 0; // Restart batch
        }
        return;
    }

    // Prepare and send next packet
    radio->iohc = radio->packets2send[radio->txCounter];
    ets_printf("TX: Sending packet %d/%d (repeat=%d, lock=%s)\n",
               radio->txCounter + 1,
               radio->packets2send.size(),
               radio->iohc->repeat,
               radio->iohc->lock ? "TRUE" : "FALSE");

    // Set radio to standby, clear flags, and load payload
    Radio::setStandby();
    Radio::clearFlags();
    Radio::writeBytes(REG_FIFO,
                      radio->iohc->payload.buffer,
                      radio->iohc->buffer_length);

    packetStamp = esp_timer_get_time();

    // Start transmission
    Radio::setTx();
    ets_printf("T2 after setTx() at %llu us\n", esp_timer_get_time());
    radio->setRadioState(iohcRadio::RadioState::TX);

    // Repeat logic
    if (radio->iohc->repeat > 0) {
        radio->iohc->repeat--;
    }

    if (radio->iohc->repeat == 0) {
        // Finished this packet
        radio->txCounter = radio->txCounter + 1;
    }

    // Toggle TX LED
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
        iohc = new iohcPacket;
        iohc->buffer_length = 0;
        iohc->frequency = scan_freqs[currentFreqIdx];

        _g_payload_millis = esp_timer_get_time();
        packetStamp = _g_payload_millis;
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
        if (rxCB) rxCB(iohc);
        iohc->decode(true); //stats);
        //free(iohc); // correct Bug memory
        delete iohc;
        digitalWrite(RX_LED, false);
        return true;
    }

/**
 * The `i_preamble` interrupt handler updates the radio state when a preamble is
 * detected on the current channel.
 */
    void IRAM_ATTR iohcRadio::i_preamble() {
#if defined(RADIO_SX127X)
        bool preamble = digitalRead(RADIO_PREAMBLE_DETECTED);
#elif defined(CC1101)
        __g_preamble = true;
        bool preamble = __g_preamble;
#endif
        iohcRadio::setRadioState(preamble ? iohcRadio::RadioState::PREAMBLE : iohcRadio::RadioState::RX);
    }

/**
 * The `i_payload` interrupt handler reads the payload detection pin and sets
 * the radio state accordingly.
 */
    void IRAM_ATTR iohcRadio::i_payload() {
#if defined(RADIO_SX127X)
        bool payload = digitalRead(RADIO_PACKET_AVAIL);
        iohcRadio::setRadioState(payload ? iohcRadio::RadioState::PAYLOAD : iohcRadio::RadioState::RX);
#endif
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
