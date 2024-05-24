#include <Arduino.h>

#include <SX1276Helpers.h>
#include <board-config.h>

#if defined(RADIO_SX127X)
    #include <map>

#if defined(ESP8266)
    #include <TickerUs.h>
#elif defined(ESP32)
    #define CONFIG_DISABLE_HAL_LOCKS true
    #include <TickerUsESP32.h>
    #include <esp_task_wdt.h>
    #include <SPI.h>
    // #include <SPIeX.h>
#endif

namespace Radio {
    SPISettings SpiSettings(4000000, MSBFIRST, SPI_MODE0);
    
    // Simplified bandwidth registries evaluation
    std::map<uint8_t, regBandWidth> __bw =
    {
        {25, {0x01, 0x04}}, // 25KHz
        {50, {0x01, 0x03}},
        {100, {0x01, 0x02}},
        {125, {0x00, 0x02}},
        {200, {0x01, 0x01}},
        {250, {0x00, 0x01}} // 250KHz
    };

    void IRAM_ATTR SPI_beginTransaction() {
        SPI.beginTransaction(Radio::SpiSettings);
        digitalWrite(RADIO_NSS, LOW);
    }

    void IRAM_ATTR SPI_endTransaction() {
        digitalWrite(RADIO_NSS, HIGH);
        SPI.endTransaction();
    }

    void initHardware() {
        printf("\nSPI Init");

        gpio_pullup_en((gpio_num_t)RADIO_MISO);

        // SPI pins configuration
#if defined(ESP8266)
        SPI.pins(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
#endif
        pinMode(RADIO_RESET, INPUT); // Connected to Reset; floating for POR

        // Check the availability of the Radio
        while (!digitalRead(RADIO_RESET)) {
#if defined(ESP8266)
            wdt_reset();
#elif defined(ESP32)
            esp_task_wdt_reset();
#endif
            delayMicroseconds(1);
        }
        delayMicroseconds(BOARD_READY_AFTER_POR);

        // Initialize SPI bus
#if defined(ESP8266)
        SPI.begin();
#elif defined(ESP32)
        SPI.begin(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
#endif
        // SPI.setFrequency(SPI_CLK_FRQ);
        // SPI.setDataMode(SPI_MODE0);
        // SPI.setBitOrder(MSBFIRST);
        SPI.setHwCs(true);

        // Disable SPI device
        // Disable device NRESET pin
        pinMode(RADIO_NSS, OUTPUT);
        pinMode(RADIO_RESET, OUTPUT);
        digitalWrite(RADIO_RESET, HIGH);
        digitalWrite(RADIO_NSS, HIGH);
        delayMicroseconds(BOARD_READY_AFTER_POR);

        // SPI.beginTransaction(Radio::SpiSettings);
        // SPI.endTransaction();

        writeByte(REG_OPMODE, RF_OPMODE_STANDBY); // Put Radio in Standby mode

        pinMode(SCAN_LED, OUTPUT);
        digitalWrite(SCAN_LED, 1);
        printf("\nRadio Chip is ready\n");
    }

    void initRegisters(uint8_t maxPayloadLength = 0xff) {
        // Firstly put radio in StandBy mode as some parameters cannot be changed differently
        writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_STANDBY);

        // ---------------- Common Register init section ----------------
        // Switch-off clockout
        writeByte(REG_OSC, RF_OSC_CLKOUT_OFF); // This only give power saveing maybe we can use it as ticker µs

        // Variable packet lenght, generates working CRC. 
        // Packet mode, IoHomeOn, IoHomePowerFrame to be added (0x10) to avoid rx to newly detect the preamble during tx radio shutdown
        // Must CRCAUTOCLEAR_ON or do full clean FIFO !
        writeByte(
            REG_PACKETCONFIG1,
            RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE | RF_PACKETCONFIG1_DCFREE_OFF | RF_PACKETCONFIG1_CRC_ON |
            RF_PACKETCONFIG1_CRCAUTOCLEAR_ON | RF_PACKETCONFIG1_CRCWHITENINGTYPE_CCITT |
            RF_PACKETCONFIG1_ADDRSFILTERING_OFF);
        writeByte(REG_PACKETCONFIG2, RF_PACKETCONFIG2_DATAMODE_PACKET | RF_PACKETCONFIG2_IOHOME_ON | RF_PACKETCONFIG2_IOHOME_POWERFRAME);
        // Is IoHomePowerFrame useful ?

        // Preamble shall be set to AA for packets to be received by appliances. Sync word shall be set with different values if Rx or Tx
        writeByte(REG_SYNCCONFIG, RF_SYNCCONFIG_AUTORESTARTRXMODE_WAITPLL_OFF | RF_SYNCCONFIG_PREAMBLEPOLARITY_AA | RF_SYNCCONFIG_SYNC_ON); //0x51); // 0x91); // TODOVERIFY 0x92
        //RF_SYNCCONFIG_AUTORESTARTRXMODE_WAITPLL_ON | RF_SYNCCONFIG_PREAMBLEPOLARITY_AA | RF_SYNCCONFIG_SYNC_ON);
        
        // Set Sync word to 0xff33 both for rx and tx
        writeByte(REG_SYNCVALUE1, SYNC_BYTE_1);
        writeByte(REG_SYNCVALUE2, SYNC_BYTE_2);

        // Mapping of pins DIO0 to DIO3
        // DIO0: PayloadReady|PacketSent    DIO1: FIFO empty    DIO2: Sync   | DIO3: TxReady
        // Mapping of pins DIO4 and DIO5
        // DIO4: PreambleDetect  DIO5: Data
        // DIO Mapping Data Packet Table 30 Page 69
        writeByte(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00 | RF_DIOMAPPING1_DIO1_01 | RF_DIOMAPPING1_DIO2_11 | RF_DIOMAPPING1_DIO3_01); // Org
        //        writeByte(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00 | RF_DIOMAPPING1_DIO1_01 | RF_DIOMAPPING1_DIO2_10 | RF_DIOMAPPING1_DIO3_01); // timeout on DIO2 for test
        writeByte(REG_DIOMAPPING2, RF_DIOMAPPING2_MAP_PREAMBLEDETECT | RF_DIOMAPPING2_DIO4_11 | RF_DIOMAPPING2_DIO5_10);
        // Preamble on DIO4

        // Enable Fast Hoping (frequency change) // Not needed all the time
        // Not using that, as it miss a lot of frames
        if (MAX_FREQS != 1)
            writeByte(REG_PLLHOP, readByte(REG_PLLHOP) | RF_PLLHOP_FASTHOP_ON); 

        // ---------------- TX Register init section ----------------
        // PA boost maximum power
        // writeByte(REG_PACONFIG, RF_PACONFIG_PASELECT_MASK | RF_PACONFIG_PASELECT_PABOOST);
        // writeByte(REG_OCP, RF_OCP_TRIM_240_MA); // 0x37); //200mA
        // writeByte(REG_PADAC, 0x87); // turn 20dBm mode on

        // PA Ramp: No Shaping, Ramp up/down 15us
        writeByte(REG_PARAMP, RF_PARAMP_MODULATIONSHAPING_00 | RF_PARAMP_0012_US); //_0015_US); //_0031_US); //
        // Setting Preamble Length
        writeByte(REG_PREAMBLEMSB, PREAMBLE_MSB);
        writeByte(REG_PREAMBLELSB, PREAMBLE_LSB);
        // FIFO Threshold - currently useless
        writeByte(REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTARTCONDITION_FIFONOTEMPTY);

        // ---------------- RX Register init section ----------------
        // Set lenght checking if passed as parameter
        // The use of maxPayloadLength is not working. Prevents generating PayloadReady signal
        writeByte(REG_PAYLOADLENGTH, 0xff);
        // RSSI precision +-2dBm
        writeByte(REG_RSSICONFIG, RF_RSSICONFIG_SMOOTHING_8); // 8->0.512 ms // _128); // _32); //_256); //
        // Activates Timeout interrupt on Preamble
        writeByte(REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT);
        // 250KHz BW with AFC
        writeByte(REG_AFCBW, RF_AFCBW_MANTAFC_16 | RF_AFCBW_EXPAFC_1);

        writeByte(REG_AFCFEI, 0x01);
        // if AGC_AUTO_ON, RF_LNA_GAIN_XX do nothing
        writeByte(REG_LNA, RF_LNA_BOOST_ON | RF_LNA_GAIN_G1); // 0xC3) ; 

        // Enables Preamble Detect, 2 bytes
        writeByte(
            REG_PREAMBLEDETECT,
            RF_PREAMBLEDETECT_DETECTOR_ON | RF_PREAMBLEDETECT_DETECTORSIZE_2 | RF_PREAMBLEDETECT_DETECTORTOL_10);

        // PA boost maximum power
        writeByte(REG_PACONFIG, RF_PACONFIG_PASELECT_MASK | RF_PACONFIG_PASELECT_PABOOST);
        writeByte(REG_OCP, RF_OCP_ON | RF_OCP_TRIM_240_MA); // 0x37); //200mA //0x3B 240mA
        writeByte(REG_PADAC,0x87); //  RF_PADAC_20DBM_MASK | RF_PADAC_20DBM_ON); // turn 20dBm mode on
    }

    /**
     * 
     */
    void calibrate() {
        // Save context
        uint8_t regPaConfigInitVal = readByte(REG_PACONFIG);

        // Cut the PA just in case, RFO output, power = -1 dBm
        writeByte(REG_PACONFIG, RF_PACONFIG_PASELECT_RFO);
        // RC Calibration (only call after setting correct frequency band)
        writeByte(REG_OSC, RF_OSC_RCCALSTART);
        // Start image and RSSI calibration
        writeByte(REG_IMAGECAL, (RF_IMAGECAL_AUTOIMAGECAL_MASK & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
        // Wait end of calibration
        while ((readByte(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) { }
        // Set a Frequency in HF band
        Radio::setCarrier(Radio::Carrier::Frequency, 868000000);
        // Start image and RSSI calibration
        writeByte(REG_IMAGECAL, (RF_IMAGECAL_AUTOIMAGECAL_MASK & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
        // Wait end of calibration
        while ((readByte(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING) {
        }

        // Restore context
        writeByte(REG_PACONFIG, regPaConfigInitVal);    
    }

    /*!
     * Performs the Rx chain calibration for LF and HF bands
     * \remark Must be called just after the reset so all registers are at their
     *         default values
     */
    // void RxChainCalibration( void ) {
    //     uint8_t regPaConfigInitVal;
    //     uint32_t initialFreq;

    //     // Save context
    //     regPaConfigInitVal = readByte( REG_PACONFIG );
    //     initialFreq = ( double )( ( ( uint32_t )readByte( REG_FRFMSB ) << 16 ) |
    //                               ( ( uint32_t )readByte( REG_FRFMID ) << 8 ) |
    //                               ( ( uint32_t )readByte( REG_FRFLSB ) ) ) * ( double )FREQ_STEP;

    //     // Cut the PA just in case, RFO output, power = -1 dBm
    //     writeByte( REG_PACONFIG, 0x00 );

    //     // Launch Rx chain calibration for LF band
    //     writeByte ( REG_IMAGECAL, ( readByte( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    //     while( ( readByte( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    //     {
    //     }

    //     // Sets a Frequency in HF band
    //     SetChannel( 868000000 );

    //     // Launch Rx chain calibration for HF band
    //     writeByte ( REG_IMAGECAL, ( readByte( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    //     while( ( readByte( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    //     {
    //     }

    //     // Restore context
    //     writeByte( REG_PACONFIG, regPaConfigInitVal );
    //     SetChannel( initialFreq );
    // }
    void IRAM_ATTR setStandby() {
        writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_STANDBY);
    }

    void IRAM_ATTR setTx() {
        // Uncommon and incompatible settings
        // Enabling Sync word - Size must be set to SYNCSIZE_2 (0x01 in header file)
        writeByte(REG_SYNCCONFIG, (readByte(REG_SYNCCONFIG) & RF_SYNCCONFIG_SYNCSIZE_MASK) | RF_SYNCCONFIG_SYNCSIZE_2);
        writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_TRANSMITTER);
        
        TxReady;
    }

    void IRAM_ATTR setRx() {
        // Uncommon and incompatible settings
        writeByte(REG_SYNCCONFIG, (readByte(REG_SYNCCONFIG) & RF_SYNCCONFIG_SYNCSIZE_MASK) | RF_SYNCCONFIG_SYNCSIZE_3);
        writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_RECEIVER);
        
        RxReady;
        /*
                // Start Sequencer
                writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_RECEIVER);
                writeByte(REG_SEQCONFIG1, readByte(REG_SEQCONFIG1 | RF_SEQCONFIG1_SEQUENCER_START));
        */
    }


    void readBurst(uint8_t regAddr, uint8_t* buffer, uint8_t size) {
        for (uint8_t i = 0; i < size; ++i) {
            buffer[i] = readByte(regAddr + i);
        }
    } // Clears FIFO at startup to avoid dirty reads
    // void clearBuffer() {
    //     for (uint8_t idx=0; idx <= 64; ++idx)
    //         readByte(REG_FIFO);
    // }
    void clearBuffer() {
        // Taille du buffer FIFO du SX1276
        const uint8_t bufferSize = 64;

        // Lire le buffer par paquets de 32 octets
        for (uint8_t i = 0; i < bufferSize; i += 32) {
            uint8_t buffer[32]; // Tableau temporaire pour stocker les octets lus
            readBytes/*Burst*/(REG_FIFO, buffer, sizeof(buffer)); // Lire 32 octets à la fois
        }
    }

    //     void clearFlags() {
    //         uint8_t out[2] = {0xff, 0xff};
    //         writeBytes(REG_IRQFLAGS1, out, 2);
    //     }
    // void clearFlags_A() {
    //   uint8_t flags = readByte(REG_IRQFLAGS1);
    //   flags &= ~0xFF; // Efface tous les drapeaux
    //   writeByte(REG_IRQFLAGS1, flags);
    // }
    void IRAM_ATTR clearFlags() {
        uint16_t flags = readWord(REG_IRQFLAGS1);
        flags &= ~0xFFFF; // Efface tous les drapeaux
        writeWord(REG_IRQFLAGS1, flags);
    }

    bool IRAM_ATTR preambleDetected() {
        return readByte(REG_IRQFLAGS1) & RF_IRQFLAGS1_PREAMBLEDETECT;
    }

    bool IRAM_ATTR syncedAddress() {
        return readByte(REG_IRQFLAGS1) & RF_IRQFLAGS1_SYNCADDRESSMATCH;
    }

    bool IRAM_ATTR dataAvail() {
        return (readByte(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOEMPTY) == 0; //?false:true;
    }

    uint8_t IRAM_ATTR readByte(uint8_t regAddr) {
        uint8_t getByte;
        readBytes(regAddr, &getByte, 1);

        return (getByte);
    }

    void IRAM_ATTR readBytes(uint8_t regAddr, uint8_t* out, uint8_t len) {
        SPI_beginTransaction();
        SPI.transfer(regAddr); // Send Address
        for (uint8_t idx = 0; idx < len; ++idx) {
            out[idx] = SPI.transfer(regAddr); // Get data
        }
        SPI_endTransaction();
    }

    bool IRAM_ATTR writeByte(uint8_t regAddr, uint8_t data, bool check) {
        return writeBytes(regAddr, &data, 1, check);
    }

    auto IRAM_ATTR writeBytes(uint8_t regAddr, uint8_t* in, uint8_t len, bool check) -> bool {
        SPI_beginTransaction();
        SPI.write(regAddr | SPI_Write); // Send Address with Write flag
        for (uint8_t idx = 0; idx < len; ++idx) {
            SPI.write(in[idx]); // Send data
        }
        SPI_endTransaction();

        if (check) {
            SPI_beginTransaction();
            SPI.transfer(regAddr); // Send Address
            for (uint8_t idx = 0; idx < len; ++idx) {
                uint8_t getByte = SPI.transfer(regAddr); // Get data
                if (in[idx] != getByte) {
                    SPI_endTransaction();
                    return false;
                }
            }
            SPI_endTransaction();
        }

        return true;
    }

    uint16_t IRAM_ATTR readWord(uint8_t regAddr) {
        uint8_t lowByte = readByte(regAddr);
        uint8_t highByte = readByte(regAddr + 1);
        return (highByte << 8) | lowByte;
    }

    void IRAM_ATTR writeWord(uint8_t regAddr, uint16_t value) {
        writeByte(regAddr, value >> 8);
        writeByte(regAddr + 1, value & 0xFF);
    }

    bool IRAM_ATTR inStdbyOrSleep() {
        uint8_t data = readByte(REG_OPMODE);
        data &= ~RF_OPMODE_MASK;
        if ((data == RF_OPMODE_SLEEP) || (data == RF_OPMODE_STANDBY))
            return true;

        return false;
    }

    bool IRAM_ATTR setCarrier(Carrier param, uint32_t value) {
        uint32_t tmpVal;
        uint8_t out[4];
        regBandWidth bw{};

        //  Change of Frequency can be done while the radio is working thanks to Freq Hopping
        if (!inStdbyOrSleep())
            if (param != Carrier::Frequency)
                return false;

        switch (param) {
            case Carrier::Frequency:
                /*uint32_t FRF = (newFreq * (uint32_t(1) << RADIOLIB_SX127X_DIV_EXPONENT)) / RADIOLIB_SX127X_CRYSTAL_FREQ;*/
                tmpVal = static_cast<uint32_t>((static_cast<float_t>(value) / FXOSC) * (1 << 19));
                out[0] = (tmpVal & 0x00ff0000) >> 16;
                out[1] = (tmpVal & 0x0000ff00) >> 8;
                out[2] = (tmpVal & 0x000000ff); // If Radio is active writing LSB triggers frequency change
                writeBytes(REG_FRFMSB, out, 3);
                break;
            case Carrier::Bandwidth:
                bw = bwRegs(value);
                writeByte(REG_RXBW, bw.Mant | bw.Exp);
                writeByte(REG_AFCBW, bw.Mant | bw.Exp);
                break;
            case Carrier::Deviation:
                tmpVal = static_cast<uint32_t>((static_cast<float_t>(value) / FXOSC) * (1 << 19));
                out[0] = (tmpVal & 0x0000ff00) >> 8;
                out[1] = (tmpVal & 0x000000ff);
                writeBytes(REG_FDEVMSB, out, 2);
            //                writeByte(REG_BITRATEFRAC, 5); // Little more precision
                break;
            case Carrier::Modulation:
                switch (value) {
                    case Modulation::FSK: {
                        uint8_t rfOpMode = readByte(REG_OPMODE);
                        rfOpMode &= RF_OPMODE_LONGRANGEMODE_MASK;
                        rfOpMode |= RF_OPMODE_LONGRANGEMODE_OFF;
                        rfOpMode &= RF_OPMODE_MODULATIONTYPE_MASK;
                        rfOpMode |= RF_OPMODE_MODULATIONTYPE_FSK;
                        rfOpMode &= RF_OPMODE_MASK;
                        rfOpMode |= RF_OPMODE_STANDBY;
                        rfOpMode &= ~0x08;
                        writeByte(REG_OPMODE, rfOpMode);
                        break;
                    }
                    case Modulation::LoRa:
                    case Modulation::OOK:
                    default: break;
                }
                break;
            case Carrier::Bitrate:
                tmpVal = FXOSC / value;
                out[0] = (tmpVal & 0x0000ff00) >> 8;
                out[1] = (tmpVal & 0x000000ff);
                writeBytes(REG_BITRATEMSB, out, 2);
                break;
        }

        return true;
    }

    regBandWidth bwRegs(uint8_t bandwidth) {
        for (auto&it: __bw)
            if (it.first == bandwidth)
                return it.second;

        return __bw.rbegin()->second;
    }

    void dump() {
        uint8_t idx = 0;

        Serial.printf("#Type\tRegister Name\tAddress[Hex]\tValue[Hex]\n");
        do {
            Serial.printf("REG\tname\t0x%2.2x\t0x%2.2x\n", idx, readByte(idx));
            idx += 1;
        }
        while (idx < 0x7f);
        Serial.printf("PKT\tFalse;False;255;0;\nXTAL\t32000000\n");
        // Serial.printf("\n");
        dumpReal();
    }
    void dumpReal() {
        uint8_t registers[0x80];
          registers[0] = 0x00;
          readBytes(0x01, registers + 1, 0x7F);
        // sx127x_dump_registers(registers, device);
        for (int idx = 0; idx < sizeof(registers); idx++) {
            if (idx != 0) {
                printf(",");
            }
        printf("0x%2.2x", registers[idx]);
        }
        printf("\n");

        dump_fsk_registers(registers);
    }
}
#endif
