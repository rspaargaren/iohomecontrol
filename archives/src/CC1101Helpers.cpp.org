#include <Arduino.h>                // Is this not required?

#include <CC1101Helpers.h>
#if defined(CC1101)

#include <map>
#if defined(ESP8266)
    #include <TickerUs.h>
#elif defined(ESP32)
    #include <TickerUsESP32.h>
    #include <esp_task_wdt.h>
#endif

namespace Radio
{

    SPISettings SpiSettings(SPI_CLK_FRQ, MSBFIRST, SPI_MODE0);
//    WorkingParams _params;

/*
    uint8_t bufferIndex = 0;


    uint32_t freqs[MAX_FREQS] = FREQS2SCAN;
    uint8_t next_freq = 0;
    uint8_t scanCounter = 0;
*/

    // Simplified bandwidth registries evaluation
    std::map<uint8_t, regBandWidth> __bw =
    {
        {50, {0x03, 0x03}},     // 58KHz
        {100, {0x00, 0x03}},    // 102
        {125, {0x02, 0x02}},    // 135
        {200, {0x00, 0x02}},    // 203
        {250, {0x03, 0x01}}     // 232KHz
    };

    void SPI_beginTransaction(void)
    {
        digitalWrite(RADIO_NSS, LOW);
        SPI.beginTransaction(Radio::SpiSettings);
    }

    void SPI_endTransaction(void)
    {
        SPI.endTransaction();
        digitalWrite(RADIO_NSS, HIGH);
    }

    void initHardware(void)
    {
        // NEcesario?
        pinMode(RADIO_DIO_0, INPUT);
        pinMode(RADIO_DIO_2, INPUT);

        pinMode(RADIO_NSS, OUTPUT);
        digitalWrite(RADIO_NSS, HIGH);

        // Initialize SPI bus
    #if defined(ESP8266)
        SPI.pins(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
    #endif
        
    #if defined(ESP8266)
        SPI.begin();
    #elif defined(ESP32)
        SPI.begin(RADIO_SCLK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
    #endif

          // try to find the CC1101 chip
        uint8_t i = 0;
        bool flagFound = false;
        while((i < 10) && !flagFound) {
            uint16_t version = SPIgetRegValue(REG_VERSION);

            if((version == 0x14) || (version == 0x04) || (version == 0x17)) {
            flagFound = true;
            } else {
            //Serial.printf("CC1101 not found! (%d of 10 tries) RADIOLIB_CC1101_REG_VERSION == 0x%04X, expected 0x0004/0x0014\n", i + 1, version);
            delay(10);
            i++;
            }
        }

        // Reset as datasheet
        digitalWrite(RADIO_NSS, LOW);
        delayMicroseconds(5);
        digitalWrite(RADIO_NSS, HIGH);
        delayMicroseconds(40);
        digitalWrite(RADIO_NSS, LOW);
        delay(10);
        SPIsendCommand(CMD_RESET);

        delay(150);
        SPIsendCommand(CMD_IDLE);

        pinMode(SCAN_LED, OUTPUT);
        digitalWrite(SCAN_LED, 1);

    }


void SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, size_t len) {
  SPItransfer(CMD_WRITE, reg | CMD_BURST, data, NULL, len);
}

void SPIwriteRegister(uint16_t reg, uint8_t data) {
    SPItransfer(CMD_WRITE, reg, &data, NULL, 1);
}


void SPIsendCommand(uint8_t cmd) {
    SPI_beginTransaction();

    // send the command byte
    uint8_t status = 0;
    spiTransfer(&cmd, 1, &status);

    SPI_endTransaction();
  
    //RADIOLIB_VERBOSE_PRINTLN("CMD\tW\t%02X\t%02X", cmd, status);
    (void)status;
}

int16_t SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval, uint8_t checkMask) {
  // status registers require special command
  if(reg > REG_TEST0) {
    reg |= CMD_ACCESS_STATUS_REG;
  }

  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    //return(RADIOLIB_ERR_INVALID_BIT_RANGE);
    return(-11);
  }


    uint8_t currentValue = SPIreadRegister(reg);
    uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
    uint8_t newValue = (currentValue & ~mask) | (value & mask);
    SPIwriteRegister(reg, newValue);


    // check register value each millisecond until check interval is reached
    // some registers need a bit of time to process the change (e.g. SX127X_REG_OP_MODE)
    uint32_t start = micros();
    uint8_t readValue = 0x00;
    while(micros() - start < (checkInterval * 1000)) {
      readValue = SPIreadRegister(reg);
      if((readValue & checkMask) == (newValue & checkMask)) {
        // check passed, we can stop the loop
        return(0);
      }
    }

    // check failed, print debug info
    /*
    RADIOLIB_DEBUG_PRINTLN();
    RADIOLIB_DEBUG_PRINTLN("address:\t0x%X", reg);
    RADIOLIB_DEBUG_PRINTLN("bits:\t\t%d %d", msb, lsb);
    RADIOLIB_DEBUG_PRINTLN("value:\t\t0x%X", value);
    RADIOLIB_DEBUG_PRINTLN("current:\t0x%X", currentValue);
    RADIOLIB_DEBUG_PRINTLN("mask:\t\t0x%X", mask);
    RADIOLIB_DEBUG_PRINTLN("new:\t\t0x%X", newValue);
    RADIOLIB_DEBUG_PRINTLN("read:\t\t0x%X", readValue);
    */

    return(-16);

}



int16_t SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  // status registers require special command
  if(reg > REG_TEST0) {
    reg |= CMD_ACCESS_STATUS_REG;
  }

  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(-1);
  }

  uint8_t rawValue = SPIreadRegister(reg);
  uint8_t maskedValue = rawValue & ((0b11111111 << lsb) & (0b11111111 >> (7 - msb)));
  return(maskedValue);
}

uint8_t SPIreadRegister(uint8_t reg) {
    uint8_t resp = 0;
    SPItransfer(CMD_READ, reg, NULL, &resp, 1);
    return(resp);
}

void SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes) {
  //this->mod->SPIreadRegisterBurst(reg | RADIOLIB_CC1101_CMD_BURST, numBytes, inBytes);
    SPItransfer(CMD_READ, reg|CMD_BURST, NULL, inBytes, numBytes);
}

void spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
  for(size_t i = 0; i < len; i++) {
    in[i] = SPI.transfer(out[i]);
  }
}

void SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes) {
    // prepare the buffers
    size_t buffLen = (1 + numBytes);

    uint8_t* buffOut = new uint8_t[buffLen];
    uint8_t* buffIn = new uint8_t[buffLen];

    uint8_t* buffOutPtr = buffOut;

  // copy the command
    *(buffOutPtr++) = reg | cmd;

  // copy the data
  if(cmd == CMD_WRITE) {
    memcpy(buffOutPtr, dataOut, numBytes);
  } else {
    memset(buffOutPtr, 0x00, numBytes);         // We introduce a literal of 0x00 to fill the comand to be sent to CC1101
  }

  // do the transfer
  SPI_beginTransaction();
  spiTransfer(buffOut, buffLen, buffIn);
  SPI_endTransaction();
  
  // copy the data
  if(cmd == CMD_READ) {
    memcpy(dataIn, &buffIn[1], numBytes);
  }

  // print debug information
  #if defined(RADIOLIB_VERBOSE)
    uint8_t* debugBuffPtr = NULL;
    if(cmd == SPIwriteCommand) {
      RADIOLIB_VERBOSE_PRINT("W\t%X\t", reg);
      debugBuffPtr = &buffOut[this->SPIaddrWidth/8];
    } else if(cmd == SPIreadCommand) {
      RADIOLIB_VERBOSE_PRINT("R\t%X\t", reg);
      debugBuffPtr = &buffIn[this->SPIaddrWidth/8];
    }
    for(size_t n = 0; n < numBytes; n++) {
      RADIOLIB_VERBOSE_PRINT("%X\t", debugBuffPtr[n]);
    }
    RADIOLIB_VERBOSE_PRINTLN();
  #endif

  #if !defined(RADIOLIB_STATIC_ONLY)
    delete[] buffOut;
    delete[] buffIn;
  #endif
}

    void initRegisters(uint8_t maxPayloadLength = 0xff)
    {

        int16_t state = SPIsetRegValue(REG_MCSM0, RF_FS_AUTOCAL_IDLE_TO_RXTX, 5, 4);
        state |= SPIsetRegValue(REG_MCSM0, RF_PIN_CTRL_OFF, 1, 1);

        // set GDOs to Hi-Z so that it doesn't output clock on startup (might confuse GDO0 action)
        state |= SPIsetRegValue(REG_IOCFG0, RF_GDOX_HIGH_Z, 5, 0);
        state |= SPIsetRegValue(REG_IOCFG2, RF_GDOX_HIGH_Z, 5, 0);

        state |= SPIsetRegValue(REG_PKTCTRL1, RF_CRC_AUTOFLUSH_OFF | RF_APPEND_STATUS_OFF | RF_ADR_CHK_NONE, 3, 0);
        state |= SPIsetRegValue(REG_PKTCTRL0, RF_WHITE_DATA_OFF | RF_PKT_FORMAT_NORMAL, 6, 4);
        state |= SPIsetRegValue(REG_PKTCTRL0, RF_CRC_OFF, 2, 2);                          // REVISAR!!!!!!!!!!

        //state |= SPIsetRegValue(REG_PATABLE, 0xC2);                                     // Potencia para 10dBm y 868Mhz (32.4mA)

        state |= SPIsetRegValue(REG_PKTCTRL1, 7 << 5, 7, 5);                     // Set preamble quality threshold (set as max)
        state |= SPIsetRegValue(REG_MDMCFG1, RF_NUM_PREAMBLE_24, 6, 4);                  // Set preamble lenght
        state |= SPIsetRegValue(REG_MDMCFG2, RF_MOD_FORMAT_2_FSK, 6, 4);                 // No shapening / 2-FSK modulation
        state |= SPIsetRegValue(REG_MDMCFG2, RF_MANCHESTER_EN_OFF, 3, 3);                // No Manchester / NRZ encoding 
        state |= SPIsetRegValue(REG_PKTCTRL0, RF_WHITE_DATA_OFF, 6, 6);                  // No whitening / NRZ encoding
        state |= SPIsetRegValue(REG_MDMCFG2, RF_SYNC_MODE_16_16), 2, 0;                  // Require 16bits sync word matched
        state |= SPIsetRegValue(REG_PKTCTRL1, RF_ADR_CHK_NONE, 1, 0);                    // No Addr Check
        state |= SPIsetRegValue(REG_PKTCTRL1, RF_APPEND_STATUS_OFF, 2, 2);               // No append status Bytes
        state |= SPIsetRegValue(REG_ADDR, 0x00);                                         // Set Address to 0
        state |= SPIsetRegValue(REG_FIFOTHR, RF_FIFO_THR_TX_61_RX_4, 3, 0);     

        // flush FIFOs
        SPIsendCommand(CMD_FLUSH_RX | CMD_READ);
        SPIsendCommand(CMD_FLUSH_TX);

        SPIsetRegValue(REG_PKTCTRL0, RF_LENGTH_CONFIG_FIXED, 1, 0);
        SPIsetRegValue(REG_PKTLEN, 0xFF);                                               // Not needed? On Reset=0xFF


/*
        int16_t state=SPIsetRegValue(REG_MCSM0, RF_FS_AUTOCAL_IDLE_TO_RXTX, 5, 4);
        SPIsetRegValue(REG_MCSM0, RF_PIN_CTRL_OFF, 1, 1);
        SPIsetRegValue(REG_IOCFG0, RF_GDOX_HIGH_Z, 5, 0);
        SPIsetRegValue(REG_IOCFG2, RF_GDOX_HIGH_Z, 5, 0);
        SPIsetRegValue(REG_PKTCTRL1, RF_CRC_AUTOFLUSH_OFF | RF_APPEND_STATUS_OFF | RF_ADR_CHK_NONE, 3, 0);
        SPIsetRegValue(REG_PKTCTRL0, RF_WHITE_DATA_OFF | RF_PKT_FORMAT_NORMAL, 6, 4);
        SPIsetRegValue(REG_PKTCTRL0, RF_CRC_ON | 0x01, 2, 0);


        //freq
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_FREQ2, 0x21);
        SPIsetRegValue(REG_FREQ1, 0x6B);
        SPIsetRegValue(REG_FREQ0, 0xD0);
        SPIsetRegValue(REG_PATABLE, 0xC2);

        //birate
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_MDMCFG4, 0x7A);
        SPIsetRegValue(REG_MDMCFG3, 0x83);

        //bandwitdh
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_MDMCFG4, 0x7A);

        //freq dev
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_DEVIATN, 0x37);
        SPIsetRegValue(REG_DEVIATN, 0x34);

        // power
        SPIsetRegValue(REG_PATABLE, 0xC2);

        // Packet Lenght (variable)
        SPIsetRegValue(REG_PKTCTRL0, 1, 1, 0);
        SPIsetRegValue(REG_PKTLEN, 0xFF);

        //preamble lenght
        SPIsetRegValue(REG_PKTCTRL1, 7 << 5, 7, 5);
        SPIsetRegValue(REG_MDMCFG1, 0b01110000, 6, 4);

        //shapening
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_MDMCFG2, RF_MOD_FORMAT_2_FSK, 6, 4);

        //Encoding
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_MDMCFG2, RF_MANCHESTER_EN_OFF, 3, 3);
        SPIsetRegValue(REG_PKTCTRL0, RF_WHITE_DATA_OFF, 6, 6);

        // sync words
        SPIsetRegValue(REG_MDMCFG2, (false ? RF_SYNC_MODE_16_16_THR : RF_SYNC_MODE_16_16), 2, 0);
        SPIsetRegValue(REG_SYNC1, 0x12);
        SPIsetRegValue(REG_SYNC0, 0xAD);

        // Flush Fifos
        SPIsendCommand(CMD_FLUSH_RX | CMD_READ);
        SPIsendCommand(CMD_FLUSH_TX);


        // Config personal tras ini
        SPIsetRegValue(REG_MDMCFG2, (false ? RF_SYNC_MODE_16_16_THR : RF_SYNC_MODE_16_16), 2, 0);
        SPIsetRegValue(REG_SYNC1, 0xFF);
        SPIsetRegValue(REG_SYNC0, 0xB3);

        //CRC off
        SPIsetRegValue(REG_PKTCTRL0, RF_CRC_OFF, 2, 2);
        //datashapening
        SPIsendCommand(CMD_IDLE);
        SPIsetRegValue(REG_MDMCFG2, RF_MOD_FORMAT_2_FSK, 6, 4);
        //Fixed packed lenght
        SPIsetRegValue(REG_PKTCTRL0, 0, 1, 0);
        SPIsetRegValue(REG_PKTLEN, 0xFF);
        // disable addr filter
        SPIsetRegValue(REG_PKTCTRL1, RF_ADR_CHK_NONE, 1, 0);
        SPIsetRegValue(REG_PKTCTRL1, RF_ADR_CHK_NONE, 1, 0);
        SPIsetRegValue(REG_ADDR, 0x00);
*/

/*
        if (state==0)
            Serial.println("InitRegisters OK");
        else
            Serial.println("Error in InitRegisters");
*/
    }

    void calibrate(void)
    {
/*
        // RC Calibration (only call after setting correct frequency band)
        writeByte(REG_OSC, RF_OSC_RCCALSTART);
        // Start image and RSSI calibration
        writeByte(REG_IMAGECAL, (RF_IMAGECAL_AUTOIMAGECAL_MASK & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
        // Wait end of calibration
        do {} while (readByte(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING);
*/
    }

    void setStandby(void)
    {
/*
        writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_STANDBY);
*/
    }

    void setTx(void)    // Uncommon and incompatible settings
    {
/*
        // Enabling Sync word - Size must be set to SYNCSIZE_2 (0x01 in header file)
        writeByte(REG_SYNCCONFIG, (readByte(REG_SYNCCONFIG) & RF_SYNCCONFIG_SYNCSIZE_MASK) | RF_SYNCCONFIG_SYNCSIZE_2);

        writeByte(REG_OPMODE, (readByte(REG_OPMODE) & RF_OPMODE_MASK) | RF_OPMODE_TRANSMITTER);
        TxReady;
*/
    }

    void setRx(void)
    {
        setSyncWord(0xFF, 0xB3);                                                // Set SyncWord
        SPIsendCommand(CMD_IDLE);
        SPIsendCommand(CMD_FLUSH_RX | CMD_READ);
        SPIsetRegValue(REG_IOCFG0, RF_GDOX_RX_FIFO_FULL_OR_PKT_END);
        SPIsetRegValue(REG_IOCFG2, RF_GDOX_RX_FIFO_OVERFLOW, 5, 0);
        SPIsetRegValue(REG_FIFOTHR, RF_FIFO_THR_TX_61_RX_4, 3, 0);
        SPIsendCommand(CMD_RX);
    }

    void clearBuffer(void)
    {
        SPIsendCommand(CMD_FLUSH_RX | CMD_READ);
    }

    void clearFlags(void)
    {
/*
        uint8_t out[2] = {0xff, 0xff};
        writeBytes(REG_IRQFLAGS1, out, 2);
*/
    }

    bool preambleDetected(void)
    {
/*
        return (readByte(REG_IRQFLAGS1) & RF_IRQFLAGS1_PREAMBLEDETECT);
*/
        return(0);
    }

    bool syncedAddress(void)
    {
/*
        return (readByte(REG_IRQFLAGS1) & RF_IRQFLAGS1_SYNCADDRESSMATCH);
*/
        return(0);
    }

    bool dataAvail(void)
    {
/*
        return ((readByte(REG_IRQFLAGS2) & RF_IRQFLAGS2_FIFOEMPTY)?false:true); // Check if FIFO is Empty in RX
*/
        uint8_t bytesInFIFO = SPIgetRegValue(REG_RXBYTES, 6, 0);
        if (bytesInFIFO == 0)
            return(false);
        else
            return(true);
    }

    uint8_t readRegValue(uint8_t regAddr)
    {
        uint8_t getByte;

        if(regAddr > REG_TEST0) {
            regAddr |= CMD_ACCESS_STATUS_REG;
        }

        readBytes(regAddr, &getByte, 1);

        return (getByte);
    }

    uint8_t readByte(uint8_t regAddr)
    {
        uint8_t getByte;

        readBytes(regAddr, &getByte, 1);

        return (getByte);
    }

    void readBytes(uint8_t regAddr, uint8_t *out, uint8_t len)
    {
        SPI_beginTransaction();
        SPI.transfer(regAddr);                  // Send Address
        for (uint8_t idx=0; idx < len; ++idx)
            out[idx] = SPI.transfer(regAddr);   // Get data
        SPI_endTransaction();

        return;
    }

    void writeCMD(uint8_t CMDAddr)
    {
        SPI_beginTransaction();
        SPI.transfer(CMDAddr);
        SPI_endTransaction();
        return;
    }

    bool writeByte(uint8_t regAddr, uint8_t data, bool check)
    {
        return writeBytes(regAddr, &data, 1, check);
    }

    bool writeBytes(uint8_t regAddr, uint8_t *in, uint8_t len, bool check)
    {
        SPI_beginTransaction();
        SPI.write(regAddr | SPI_Write);      // Send Address with Write flag
        for (uint8_t idx=0; idx < len; ++idx)
            SPI.write(in[idx]);              // Send data
        SPI_endTransaction();

        if (check)
        {
            uint8_t getByte;

            SPI_beginTransaction();
            SPI.transfer(regAddr);                  // Send Address
            for (uint8_t idx=0; idx < len; ++idx)
            {
                getByte = SPI.transfer(regAddr);    // Get data
                if (in[idx] != getByte)
                {
                    SPI_endTransaction();
                    return false;
                }
            }
            SPI_endTransaction();
        }

        return true;
    }

    bool inStdbyOrSleep(void)
    {
        uint8_t data = SPIgetRegValue(REG_MARCSTATE,4,0);        
        if ((data == RF_MARC_STATE_SLEEP) || (data == RF_MARC_STATE_IDLE))
            return true;
        return false;
    }

    bool setCarrier(Carrier param, uint32_t value)
    {
        uint32_t tmpVal;
        uint32_t base = 1;
        //uint8_t out[4];
        regBandWidth bw;
        uint8_t e = 0;
        uint8_t m = 0;


//  Change of Frequency can be done while the radio is working thanks to Freq Hopping
        if (!inStdbyOrSleep())
            if (param != Carrier::Frequency)
                return false;

        switch (param)
        {
            case Carrier::Frequency:
                // set mode to standby
                SPIsendCommand(CMD_IDLE);

                //set carrier frequency
                tmpVal = (uint32_t)(((float_t)value/FXOSC)*(1<<16));
                SPIsetRegValue(REG_FREQ2, (tmpVal & 0xFF0000) >> 16, 7, 0);
                SPIsetRegValue(REG_FREQ1, (tmpVal & 0x00FF00) >> 8, 7, 0);
                SPIsetRegValue(REG_FREQ0, tmpVal & 0x0000FF, 7, 0);
                break;
            case Carrier::Bandwidth:
                bw = bwRegs(value);
                SPIsendCommand(CMD_IDLE);
                SPIsetRegValue(REG_MDMCFG4, (bw.Exp << 6) | (bw.Mant << 4), 7, 4); // Values limited, linked with a table pre-defined
                break;
            case Carrier::Deviation:
                SPIsendCommand(CMD_IDLE);
                e = 0;
                m = 0;
                getExpMant((float)value, 8, 17, 7, e, m);

                // set frequency deviation value
                SPIsetRegValue(REG_DEVIATN, (e << 4), 6, 4);
                SPIsetRegValue(REG_DEVIATN, m, 2, 0);
                break;
            case Carrier::Modulation:
                switch (value)
                {
                    case Modulation::FSK:
                        writeByte(REG_MDMCFG2, 0x02);   //Include DC Filter (Off) - Modulation (2-FSK) - Manchester Enc (Off) - Sync Word Mode (16bits)
                        break;
                }
                break;
            case Carrier::Bitrate:
                SPIsendCommand(CMD_IDLE);
                // calculate exponent and mantissa values
                e = 0;
                m = 0;
                getExpMant((float)(value), 256, 28, 14, e, m);
                // set bit rate value
                SPIsetRegValue(REG_MDMCFG4, e, 3, 0);
                SPIsetRegValue(REG_MDMCFG3, m);
                break;
        }

        return true;
    }

    regBandWidth bwRegs(uint8_t bandwidth)
    {
        for (auto it = __bw.begin(); it != __bw.end(); it++)
            if (it->first == bandwidth)
                return it->second;

        return __bw.rbegin()->second;
    }

    void dump()
    {
        uint8_t idx = 0x00;

        Serial.printf("*********************** Radio registers ***********************\n");
        do       
        {
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("*%2.2x=%2.2x\t", idx, SPIgetRegValue(idx)); idx += 1;
            Serial.printf("\n");
        }
        while (idx < 0x3F);
        Serial.printf("***************************************************************\n");
        Serial.printf("\n");
    }

    void dump2()
    {
        Serial.printf("*********************** Radio registers ***********************\n");
        for (uint8_t a=0; a<=0x3F; a++)
            Serial.printf("%X;%2.2X\n", a,  SPIgetRegValue(a));
        Serial.printf("***************************************************************\n");
        Serial.printf("\n");
    }

    uint8_t reverseByte(const uint8_t a){
        uint8_t v = a;
        v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
        v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
        v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
        return (uint8_t)v;
    }

    void getExpMant(float target, uint16_t mantOffset, uint8_t divExp, uint8_t expMax, uint8_t& exp, uint8_t& mant) {
    // get table origin point (exp = 0, mant = 0)
        float origin = (mantOffset * (float)FXOSC)/((uint32_t)1 << divExp);

        // iterate over possible exponent values
        for(int8_t e = expMax; e >= 0; e--) {
            // get table column start value (exp = e, mant = 0);
            float intervalStart = ((uint32_t)1 << e) * origin;

            // check if target value is in this column
            if(target >= intervalStart) {
            // save exponent value
            exp = e;

            // calculate size of step between table rows
                float stepSize = intervalStart/(float)mantOffset;

            // get target point position (exp = e, mant = m)
                mant = ((target - intervalStart) / stepSize);

            // we only need the first match, terminate
                return;
            }
        }
    }

    int16_t decodeFrame(uint8_t* data, uint8_t lenSrc, uint8_t lenDecoded){
        byte TmpArray[64];
        uint8_t byteDecoded;

        uint8_t shiftbits=4;
        uint8_t indice=0;
        uint8_t a=0;

        if (lenSrc>64 || lenSrc<10)
            return (-1);

        while (indice < lenDecoded){
            byteDecoded = ((data[a]<<shiftbits) | (data[a+1]>>(8-shiftbits)));
            shiftbits+=2;
            if (shiftbits==8){
                shiftbits=0;
                a++;
            }
            TmpArray[indice] = reverseByte(byteDecoded);

            if (lenDecoded==64){    // If lenDecoded default, then obtain frame lenght
                if(indice==0){  // Get lenght frame
                    lenDecoded = (TmpArray[indice]&0b00011111) + 1 + 2; // Longitud total= Crtl Byte 1 + Payload + CRC
                }
            }
            indice++;
            a++;
        }

        memcpy(data, TmpArray, lenDecoded);  // volcamos el resultado al array de origen

        return(lenDecoded);
    }

    void sendFrame(uint8_t* data, uint8_t lenFrame){
        for (uint8_t a=0; a<lenFrame; a++)
            Serial.printf("%2.2X ", data[a]);
        Serial.println();    

        /*
        SPIsendCommand(CMD_IDLE);
        SPIsendCommand(CMD_FLUSH_TX);
        SPIsetRegValue(REG_IOCFG2, RF_GDOX_SYNC_WORD_SENT_OR_PKT_RECEIVED, 5, 0);
        uint8_t dataSent = 0;
        uint8_t initialWrite = min((uint8_t)len, (uint8_t)(RADIOLIB_CC1101_FIFO_SIZE - dataSent));
        SPIwriteRegisterBurst(REG_FIFO, data, initialWrite);
        dataSent += initialWrite;
        SPIsendCommand(RADIOLIB_CC1101_CMD_TX);
          // keep feeding the FIFO until the packet is over
        while (dataSent < len) {
            // get number of bytes in FIFO
            uint8_t bytesInFIFO = SPIgetRegValue(RADIOLIB_CC1101_REG_TXBYTES, 6, 0);

            // if there's room then put other data
            if (bytesInFIFO < 64) {
                uint8_t bytesToWrite = min((uint8_t)(64 - bytesInFIFO), (uint8_t)(len - dataSent));
                SPIwriteRegisterBurst(REG_FIFO, &data[dataSent], bytesToWrite);
                dataSent += bytesToWrite;
            } else {
                // wait for radio to send some data
                //    * Does this work for all rates? If 1 ms is longer than the 1ms delay
                //    * then the entire FIFO will be transmitted during that delay.
                //    *
                //    * TODO: test this on real hardware
                delayMicroseconds(250);
            }
        }
        */
    }



    int16_t standby() {
        // set idle mode
        SPIsendCommand(CMD_IDLE);

    // wait until idle is reached
    uint32_t start = stamp();
    while(SPIgetRegValue(REG_MARCSTATE, 4, 0) != RF_MARC_STATE_IDLE) {
        yield();
        if(stamp() - start > 100) {
        // timeout, this should really not happen
        Serial.println("Critical Err. CC1101 do not enter in IDLE STATE");
        return(-1);
        }
    };

    // set RF switch (if present)
    //this->mod->setRfSwitchState(Module::MODE_IDLE);
    return(0);
    }

    void setSyncWord(uint8_t syncH, uint8_t syncL) {
        SPIsetRegValue(REG_SYNC1, syncH);
        SPIsetRegValue(REG_SYNC0, syncL);
    }
    
    void setPktLenght(uint8_t len){
        SPIsetRegValue(REG_PKTLEN, len);
    }


}
#endif