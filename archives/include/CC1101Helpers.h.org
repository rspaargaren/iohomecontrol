#ifndef CC1101HELPERS_H
#define CC1101HELPERS_H

#include "cc1101Regs-Fsk.h"
#include <board-config.h>
#include <SPI.h>

#if defined(ESP8266)

#elif defined(ESP32)
    #include "mbedtls/aes.h"        // AES functions
#endif

#define LSBFIRST 0
#define MSBFIRST 1

#define KHz     *1000
#define MHz     (KHz *1000)
#define FXOSC   26000000
//#define LOWER   525000000
//#define HIGHER  779000000

#define SPI_Write   0x80
#define SPI_Read    0x00

#define TxReady  {do {/* Checks new Mode is ready */} while (!(readByte(REG_IRQFLAGS1) & RF_IRQFLAGS1_TXREADY));}   // Check for TxReady flag
#define RxReady  {do {/* Checks new Mode is ready */} while (!(readByte(REG_IRQFLAGS1) & RF_IRQFLAGS1_PLLLOCK));}   // Check for PllLock flag; do not use with sequencer

#define RF_PACKETCONFIG2_IOHOME_POWERFRAME  0x10    // Missing from SX1276 FSK modem registers and bits definitions

/*
    Helper functions to setup and manage SX1276 registry configuration, query status and SPI interaction
*/
namespace Radio
{
    enum class Carrier {
        Frequency,
        Deviation,
        Bandwidth,
        Bitrate,
        Modulation
    };

    enum Modulation:uint32_t {
        OOK = 0x00,
        FSK,
        LoRa
    };

    typedef struct
    {
        uint32_t    carrierFrequency;
        uint8_t     rfOpMode;
        uint32_t    bitRate;
        uint32_t    deviation;
        uint8_t     seqConf[2];
    } WorkingParams;

    typedef struct
    {
        uint8_t     Mant;
        uint8_t     Exp;
    } regBandWidth;

    void initHardware(void);
    void initRegisters(uint8_t maxPayloadLength);
    void calibrate(void);
    void setStandby(void);
    void setTx(void);
    void setRx(void);
    void clearBuffer(void);
    void clearFlags(void);
    bool preambleDetected(void);
    bool syncedAddress(void);
    bool dataAvail(void);
    bool crcOk(void);
    uint8_t readRegValue(uint8_t regAddr);
    uint8_t readByte(uint8_t regAddr);
    void readBytes(uint8_t regAddr, uint8_t *out, uint8_t len);
    void writeCMD(uint8_t CMDAddr);
    bool writeByte(uint8_t regAddr, uint8_t data, bool check = NULL);
    bool writeBytes(uint8_t regAddr, uint8_t *in, uint8_t len, bool check = NULL);
    bool inStdbyOrSleep(void);
    bool setParams();
    bool setCarrier(Carrier param, uint32_t value);
    regBandWidth bwRegs(uint8_t bandwidth);
    void dump(void);
    void dump2(void);

    //void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override;
    void spiTransfer(uint8_t* out, size_t len, uint8_t* in);

    int16_t SPIgetRegValue(uint8_t reg, uint8_t msb = 7, uint8_t lsb = 0);
    int16_t SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb = 7, uint8_t lsb = 0, uint8_t checkInterval = 2, uint8_t checkMask = 0xFF);
    void SPIsendCommand(uint8_t cmd);
    uint8_t SPIreadRegister(uint8_t reg);
    void SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes);
    void SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, size_t numBytes);

    void SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, size_t len);
    void SPIwriteRegister(uint16_t reg, uint8_t data);

    static void getExpMant(float target, uint16_t mantOffset, uint8_t divExp, uint8_t expMax, uint8_t& exp, uint8_t& mant);
    uint8_t reverseByte(const uint8_t a);
    int16_t decodeFrame(uint8_t* data, uint8_t lenSrc, uint8_t lenDecoded=64);
    void sendFrame(uint8_t* data, uint8_t lenFrame);

    int16_t standby();
    void setSyncWord(uint8_t syncH, uint8_t syncL);
    void setPktLenght(uint8_t len);
}
#endif // CC1101HELPERS_H
