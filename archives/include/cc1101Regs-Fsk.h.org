/*!
 * \file      cc1101-Fsk.h
 *
 * \brief     CC1101 FSK modem registers and bits definitions
 */
#ifndef __CC1101_REGS_FSK_H__
#define __CC1101_REGS_FSK_H__


// CC1101 SPI commands
#define CMD_READ                                0b10000000
#define CMD_WRITE                               0b00000000
#define CMD_BURST                               0b01000000
#define CMD_ACCESS_STATUS_REG                   0b01000000
#define CMD_FIFO_RX                             0b10000000
#define CMD_FIFO_TX                             0b00000000
#define CMD_RESET                               0x30
#define CMD_FSTXON                              0x31
#define CMD_XOFF                                0x32
#define CMD_CAL                                 0x33
#define CMD_RX                                  0x34
#define CMD_TX                                  0x35
#define CMD_IDLE                                0x36
#define CMD_WOR                                 0x38
#define CMD_POWER_DOWN                          0x39
#define CMD_FLUSH_RX                            0x3A
#define CMD_FLUSH_TX                            0x3B
#define CMD_WOR_RESET                           0x3C
#define CMD_NOP                                 0x3D

// CC1101 register map
#define REG_IOCFG2                              0x00
#define REG_IOCFG1                              0x01
#define REG_IOCFG0                              0x02
#define REG_FIFOTHR                             0x03
#define REG_SYNC1                               0x04
#define REG_SYNC0                               0x05
#define REG_PKTLEN                              0x06
#define REG_PKTCTRL1                            0x07
#define REG_PKTCTRL0                            0x08
#define REG_ADDR                                0x09
#define REG_CHANNR                              0x0A
#define REG_FSCTRL1                             0x0B
#define REG_FSCTRL0                             0x0C
#define REG_FREQ2                               0x0D
#define REG_FREQ1                               0x0E
#define REG_FREQ0                               0x0F
#define REG_MDMCFG4                             0x10
#define REG_MDMCFG3                             0x11
#define REG_MDMCFG2                             0x12
#define REG_MDMCFG1                             0x13
#define REG_MDMCFG0                             0x14
#define REG_DEVIATN                             0x15
#define REG_MCSM2                               0x16
#define REG_MCSM1                               0x17
#define REG_MCSM0                               0x18
#define REG_FOCCFG                              0x19
#define REG_BSCFG                               0x1A
#define REG_AGCCTRL2                            0x1B
#define REG_AGCCTRL1                            0x1C
#define REG_AGCCTRL0                            0x1D
#define REG_WOREVT1                             0x1E
#define REG_WOREVT0                             0x1F
#define REG_WORCTRL                             0x20
#define REG_FREND1                              0x21
#define REG_FREND0                              0x22
#define REG_FSCAL3                              0x23
#define REG_FSCAL2                              0x24
#define REG_FSCAL1                              0x25
#define REG_FSCAL0                              0x26
#define REG_RCCTRL1                             0x27
#define REG_RCCTRL0                             0x28
#define REG_FSTEST                              0x29
#define REG_PTEST                               0x2A
#define REG_AGCTEST                             0x2B
#define REG_TEST2                               0x2C
#define REG_TEST1                               0x2D
#define REG_TEST0                               0x2E
#define REG_PARTNUM                             0x30
#define REG_VERSION                             0x31
#define REG_FREQEST                             0x32
#define REG_LQI                                 0x33
#define REG_RSSI                                0x34
#define REG_MARCSTATE                           0x35
#define REG_WORTIME1                            0x36
#define REG_WORTIME0                            0x37
#define REG_PKTSTATUS                           0x38
#define REG_VCO_VC_DAC                          0x39
#define REG_TXBYTES                             0x3A
#define REG_RXBYTES                             0x3B
#define REG_RCCTRL1_STATUS                      0x3C
#define REG_RCCTRL0_STATUS                      0x3D
#define REG_PATABLE                             0x3E
#define REG_FIFO                                0x3F


#define RF_NO_MASK                                  0b11111111

// RF_CC1101_REG_IOCFG0
#define RF_GDO0_TEMP_SENSOR_OFF                    0b00000000  //  7     7   analog temperature sensor output: disabled (default)
#define RF_GDO0_TEMP_SENSOR_ON                     0b10000000  //  7     7                                     enabled
#define RF_GDO0_NORM                               0b00000000  //  6     6   GDO0 output: active high (default)
#define RF_GDO0_INV                                0b01000000  //  6     6                active low

// RF_REG_IOCFG2 + REG_IOCFG1 + REG_IOCFG0
#define RF_GDOX_RX_FIFO_FULL                       0x00        //  5     0   Rx FIFO full or above threshold
#define RF_GDOX_RX_FIFO_FULL_OR_PKT_END            0x01        //  5     0   Rx FIFO full or above threshold or reached packet end
#define RF_GDOX_TX_FIFO_ABOVE_THR                  0x02        //  5     0   Tx FIFO above threshold
#define RF_GDOX_TX_FIFO_FULL                       0x03        //  5     0   Tx FIFO full
#define RF_GDOX_RX_FIFO_OVERFLOW                   0x04        //  5     0   Rx FIFO overflowed
#define RF_GDOX_TX_FIFO_UNDERFLOW                  0x05        //  5     0   Tx FIFO underflowed
#define RF_GDOX_SYNC_WORD_SENT_OR_PKT_RECEIVED     0x06        //  5     0   sync word was sent or packet was received
#define RF_GDOX_PKT_RECEIVED_CRC_OK                0x07        //  5     0   packet received and CRC check passed
#define RF_GDOX_PREAMBLE_QUALITY_REACHED           0x08        //  5     0   received preamble quality is above threshold
#define RF_GDOX_CHANNEL_CLEAR                      0x09        //  5     0   RSSI level below threshold (channel is clear)
#define RF_GDOX_PLL_LOCKED                         0x0A        //  5     0   PLL is locked
#define RF_GDOX_SERIAL_CLOCK                       0x0B        //  5     0   serial data clock
#define RF_GDOX_SERIAL_DATA_SYNC                   0x0C        //  5     0   serial data output in: synchronous mode
#define RF_GDOX_SERIAL_DATA_ASYNC                  0x0D        //  5     0                          asynchronous mode
#define RF_GDOX_CARRIER_SENSE                      0x0E        //  5     0   RSSI above threshold
#define RF_GDOX_CRC_OK                             0x0F        //  5     0   CRC check passed
#define RF_GDOX_RX_HARD_DATA1                      0x16        //  5     0   direct access to demodulated data
#define RF_GDOX_RX_HARD_DATA0                      0x17        //  5     0   direct access to demodulated data
#define RF_GDOX_PA_PD                              0x1B        //  5     0   power amplifier circuit is powered down
#define RF_GDOX_LNA_PD                             0x1C        //  5     0   low-noise amplifier circuit is powered down
#define RF_GDOX_RX_SYMBOL_TICK                     0x1D        //  5     0   direct access to symbol tick of received data
#define RF_GDOX_WOR_EVNT0                          0x24        //  5     0   wake-on-radio event 0
#define RF_GDOX_WOR_EVNT1                          0x25        //  5     0   wake-on-radio event 1
#define RF_GDOX_CLK_256                            0x26        //  5     0   256 Hz clock
#define RF_GDOX_CLK_32K                            0x27        //  5     0   32 kHz clock
#define RF_GDOX_CHIP_RDYN                          0x29        //  5     0    (default for GDO2)
#define RF_GDOX_XOSC_STABLE                        0x2B        //  5     0
#define RF_GDOX_HIGH_Z                             0x2E        //  5     0   high impedance state (default for GDO1)
#define RF_GDOX_HW_TO_0                            0x2F        //  5     0
#define RF_GDOX_CLOCK_XOSC_1                       0x30        //  5     0   crystal oscillator clock: f = f(XOSC)/1
#define RF_GDOX_CLOCK_XOSC_1_5                     0x31        //  5     0                             f = f(XOSC)/1.5
#define RF_GDOX_CLOCK_XOSC_2                       0x32        //  5     0                             f = f(XOSC)/2
#define RF_GDOX_CLOCK_XOSC_3                       0x33        //  5     0                             f = f(XOSC)/3
#define RF_GDOX_CLOCK_XOSC_4                       0x34        //  5     0                             f = f(XOSC)/4
#define RF_GDOX_CLOCK_XOSC_6                       0x35        //  5     0                             f = f(XOSC)/6
#define RF_GDOX_CLOCK_XOSC_8                       0x36        //  5     0                             f = f(XOSC)/8
#define RF_GDOX_CLOCK_XOSC_12                      0x37        //  5     0                             f = f(XOSC)/12
#define RF_GDOX_CLOCK_XOSC_16                      0x38        //  5     0                             f = f(XOSC)/16
#define RF_GDOX_CLOCK_XOSC_24                      0x39        //  5     0                             f = f(XOSC)/24
#define RF_GDOX_CLOCK_XOSC_32                      0x3A        //  5     0                             f = f(XOSC)/32
#define RF_GDOX_CLOCK_XOSC_48                      0x3B        //  5     0                             f = f(XOSC)/48
#define RF_GDOX_CLOCK_XOSC_64                      0x3C        //  5     0                             f = f(XOSC)/64
#define RF_GDOX_CLOCK_XOSC_96                      0x3D        //  5     0                             f = f(XOSC)/96
#define RF_GDOX_CLOCK_XOSC_128                     0x3E        //  5     0                             f = f(XOSC)/128
#define RF_GDOX_CLOCK_XOSC_192                     0x3F        //  5     0                             f = f(XOSC)/192 (default for GDO0)


// RF_REG_FIFOTHR
#define RF_ADC_RETENTION_OFF                       0b00000000  //  6     6   do not retain ADC settings in sleep mode (default)
#define RF_ADC_RETENTION_ON                        0b01000000  //  6     6   retain ADC settings in sleep mode
#define RF_RX_ATTEN_0_DB                           0b00000000  //  5     4   Rx attenuation: 0 dB (default)
#define RF_RX_ATTEN_6_DB                           0b00010000  //  5     4                   6 dB
#define RF_RX_ATTEN_12_DB                          0b00100000  //  5     4                   12 dB
#define RF_RX_ATTEN_18_DB                          0b00110000  //  5     4                   18 dB
#define RF_FIFO_THR_TX_61_RX_4                     0b00000000  //  3     0   TX fifo threshold: 61, RX fifo threshold: 4
#define RF_FIFO_THR_TX_57_RX_8                     0b00000001  //  3     0   TX fifo threshold: 61, RX fifo threshold: 4
#define RF_FIFO_THR_TX_33_RX_32                    0b00000111  //  3     0   TX fifo threshold: 33, RX fifo threshold: 32
#define RF_FIFO_THRESH_TX                          33
#define RF_FIFO_THRESH_RX                         

// RF_REG_MDMCFG2
#define RF_DEM_DCFILT_OFF                          0b10000000  //  7     7   digital DC filter: disabled
#define RF_DEM_DCFILT_ON                           0b00000000  //  7     7                      enabled - only for data rates above 250 kBaud (default)
#define RF_MOD_FORMAT_2_FSK                        0b00000000  //  6     4   modulation format: 2-FSK (default)
#define RF_MOD_FORMAT_GFSK                         0b00010000  //  6     4                      GFSK
#define RF_MOD_FORMAT_ASK_OOK                      0b00110000  //  6     4                      ASK/OOK
#define RF_MOD_FORMAT_4_FSK                        0b01000000  //  6     4                      4-FSK
#define RF_MOD_FORMAT_MFSK                         0b01110000  //  6     4                      MFSK - only for data rates above 26 kBaud
#define RF_MANCHESTER_EN_OFF                       0b00000000  //  3     3   Manchester encoding: disabled (default)
#define RF_MANCHESTER_EN_ON                        0b00001000  //  3     3                        enabled
#define RF_SYNC_MODE_NONE                          0b00000000  //  2     0   synchronization: no preamble/sync
#define RF_SYNC_MODE_15_16                         0b00000001  //  2     0                    15/16 sync word bits
#define RF_SYNC_MODE_16_16                         0b00000010  //  2     0                    16/16 sync word bits (default)
#define RF_SYNC_MODE_30_32                         0b00000011  //  2     0                    30/32 sync word bits
#define RF_SYNC_MODE_NONE_THR                      0b00000100  //  2     0                    no preamble sync, carrier sense above threshold
#define RF_SYNC_MODE_15_16_THR                     0b00000101  //  2     0                    15/16 sync word bits, carrier sense above threshold
#define RF_SYNC_MODE_16_16_THR                     0b00000110  //  2     0                    16/16 sync word bits, carrier sense above threshold
#define RF_SYNC_MODE_30_32_THR                     0b00000111  //  2     0                    30/32 sync word bits, carrier sense above threshold

// RF_REG_MDMCFG1
#define RF_FEC_OFF                                 0b00000000  //  7     7   forward error correction: disabled (default)
#define RF_FEC_ON                                  0b10000000  //  7     7                             enabled - only for fixed packet length
#define RF_NUM_PREAMBLE_2                          0b00000000  //  6     4   number of preamble bytes: 2
#define RF_NUM_PREAMBLE_3                          0b00010000  //  6     4                             3
#define RF_NUM_PREAMBLE_4                          0b00100000  //  6     4                             4 (default)
#define RF_NUM_PREAMBLE_6                          0b00110000  //  6     4                             6
#define RF_NUM_PREAMBLE_8                          0b01000000  //  6     4                             8
#define RF_NUM_PREAMBLE_12                         0b01010000  //  6     4                             12
#define RF_NUM_PREAMBLE_16                         0b01100000  //  6     4                             16
#define RF_NUM_PREAMBLE_24                         0b01110000  //  6     4                             24
#define RF_CHANSPC_E                               0x02        //  1     0   channel spacing: df_channel = (f(XOSC) / 2^18) * (256 + CHANSPC_M) * 2^CHANSPC_E [Hz]


// RF_REG_PKTCTRL1
#define RF_PQT                                     0x00        //  7     5   preamble quality threshold
#define RF_CRC_AUTOFLUSH_OFF                       0b00000000  //  3     3   automatic Rx FIFO flush on CRC check fail: disabled (default)
#define RF_CRC_AUTOFLUSH_ON                        0b00001000  //  3     3                                              enabled
#define RF_APPEND_STATUS_OFF                       0b00000000  //  2     2   append 2 status bytes to packet: disabled
#define RF_APPEND_STATUS_ON                        0b00000100  //  2     2                                    enabled (default)
#define RF_ADR_CHK_NONE                            0b00000000  //  1     0   address check: none (default)
#define RF_ADR_CHK_NO_BROADCAST                    0b00000001  //  1     0                  without broadcast
#define RF_ADR_CHK_SINGLE_BROADCAST                0b00000010  //  1     0                  broadcast address 0x00
#define RF_ADR_CHK_DOUBLE_BROADCAST                0b00000011  //  1     0                  broadcast addresses 0x00 and 0xFF


// RF_REG_PKTCTRL0
#define RF_WHITE_DATA_OFF                          0b00000000  //  6     6   data whitening: disabled
#define RF_WHITE_DATA_ON                           0b01000000  //  6     6                   enabled (default)
#define RF_PKT_FORMAT_NORMAL                       0b00000000  //  5     4   packet format: normal (FIFOs)
#define RF_PKT_FORMAT_SYNCHRONOUS                  0b00010000  //  5     4                  synchronous serial
#define RF_PKT_FORMAT_RANDOM                       0b00100000  //  5     4                  random transmissions
#define RF_PKT_FORMAT_ASYNCHRONOUS                 0b00110000  //  5     4                  asynchronous serial
#define RF_CRC_OFF                                 0b00000000  //  2     2   CRC disabled
#define RF_CRC_ON                                  0b00000100  //  2     2   CRC enabled (default)
#define RF_LENGTH_CONFIG_FIXED                     0b00000000  //  1     0   packet length: fixed
#define RF_LENGTH_CONFIG_VARIABLE                  0b00000001  //  1     0                  variable (default)
#define RF_LENGTH_CONFIG_INFINITE                  0b00000010  //  1     0                  infinite

// RF_REG_MCSM1
#define RF_CCA_MODE_ALWAYS                         0b00000000  //  5     4   clear channel indication: always
#define RF_CCA_MODE_RSSI_THR                       0b00010000  //  5     4                             RSSI below threshold
#define RF_CCA_MODE_RX_PKT                         0b00100000  //  5     4                             unless receiving packet
#define RF_CCA_MODE_RSSI_THR_RX_PKT                0b00110000  //  5     4                             RSSI below threshold unless receiving packet (default)
#define RF_RXOFF_IDLE                              0b00000000  //  3     2   next mode after packet reception: idle (default)
#define RF_RXOFF_FSTXON                            0b00000100  //  3     2                                     FSTxOn
#define RF_RXOFF_TX                                0b00001000  //  3     2                                     Tx
#define RF_RXOFF_RX                                0b00001100  //  3     2                                     Rx
#define RF_TXOFF_IDLE                              0b00000000  //  1     0   next mode after packet transmission: idle (default)
#define RF_TXOFF_FSTXON                            0b00000001  //  1     0                                        FSTxOn
#define RF_TXOFF_TX                                0b00000010  //  1     0                                        Tx
#define RF_TXOFF_RX                                0b00000011  //  1     0                                        Rx


// RF_REG_MCSM0
#define RF_FS_AUTOCAL_NEVER                        0b00000000  //  5     4   automatic calibration: never (default)
#define RF_FS_AUTOCAL_IDLE_TO_RXTX                 0b00010000  //  5     4                          every transition from idle to Rx/Tx
#define RF_FS_AUTOCAL_RXTX_TO_IDLE                 0b00100000  //  5     4                          every transition from Rx/Tx to idle
#define RF_FS_AUTOCAL_RXTX_TO_IDLE_4TH             0b00110000  //  5     4                          every 4th transition from Rx/Tx to idle
#define RF_PO_TIMEOUT_COUNT_1                      0b00000000  //  3     2   number of counter expirations before CHP_RDYN goes low: 1 (default)
#define RF_PO_TIMEOUT_COUNT_16                     0b00000100  //  3     2                                                           16
#define RF_PO_TIMEOUT_COUNT_64                     0b00001000  //  3     2                                                           64
#define RF_PO_TIMEOUT_COUNT_256                    0b00001100  //  3     2                                                           256
#define RF_PIN_CTRL_OFF                            0b00000000  //  1     1   pin radio control: disabled (default)
#define RF_PIN_CTRL_ON                             0b00000010  //  1     1                      enabled
#define RF_XOSC_FORCE_OFF                          0b00000000  //  0     0   do not force XOSC to remain on in sleep (default)
#define RF_XOSC_FORCE_ON                           0b00000001  //  0     0   force XOSC to remain on in sleep



// RF_REG_MARCSTATE
#define RF_MARC_STATE_SLEEP                        0x00        //  4     0   main radio control state: sleep
#define RF_MARC_STATE_IDLE                         0x01        //  4     0                             idle
#define RF_MARC_STATE_XOFF                         0x02        //  4     0                             XOFF
#define RF_MARC_STATE_VCOON_MC                     0x03        //  4     0                             VCOON_MC
#define RF_MARC_STATE_REGON_MC                     0x04        //  4     0                             REGON_MC
#define RF_MARC_STATE_MANCAL                       0x05        //  4     0                             MANCAL
#define RF_MARC_STATE_VCOON                        0x06        //  4     0                             VCOON
#define RF_MARC_STATE_REGON                        0x07        //  4     0                             REGON
#define RF_MARC_STATE_STARTCAL                     0x08        //  4     0                             STARTCAL
#define RF_MARC_STATE_BWBOOST                      0x09        //  4     0                             BWBOOST
#define RF_MARC_STATE_FS_LOCK                      0x0A        //  4     0                             FS_LOCK
#define RF_MARC_STATE_IFADCON                      0x0B        //  4     0                             IFADCON
#define RF_MARC_STATE_ENDCAL                       0x0C        //  4     0                             ENDCAL
#define RF_MARC_STATE_RX                           0x0D        //  4     0                             RX
#define RF_MARC_STATE_RX_END                       0x0E        //  4     0                             RX_END
#define RF_MARC_STATE_RX_RST                       0x0F        //  4     0                             RX_RST
#define RF_MARC_STATE_TXRX_SWITCH                  0x10        //  4     0                             TXRX_SWITCH
#define RF_MARC_STATE_RXFIFO_OVERFLOW              0x11        //  4     0                             RXFIFO_OVERFLOW
#define RF_MARC_STATE_FSTXON                       0x12        //  4     0                             FSTXON
#define RF_MARC_STATE_TX                           0x13        //  4     0                             TX
#define RF_MARC_STATE_TX_END                       0x14        //  4     0                             TX_END
#define RF_MARC_STATE_RXTX_SWITCH                  0x15        //  4     0                             RXTX_SWITCH
#define RF_MARC_STATE_TXFIFO_UNDERFLOW             0x16        //  4     0                             TXFIFO_UNDERFLOW


/*/

// RF_REG_IOCFG2
#define RF_IOCFG2_GDO2_MASK                         0b01000000
#define RF_IOCFG2_GDO2_NORM                         0b00000000  //  6     6   GDO2 output: active high (default)
#define RF_IOCFG2_GDO2_INV                          0b01000000  //  6     6                active low


// RF_REG_IOCFG1
#define RF_IOCFG1_GDO1_DS_MASK                      0b10000000
#define RF_IOCFG1_GDO1_DS_LOW                       0b00000000  //  7     7   GDO1 output drive strength: low (default)
#define RF_IOCFG1_GDO1_DS_HIGH                      0b10000000  //  7     7                               high

#define RF_IOCFG1_GDO1_INV_MASK                     0b01000000
#define RF_IOCFG1_GDO1_NORM                         0b00000000  //  6     6   GDO1 output: active high (default)
#define RF_IOCFG1_GDO1_INV                          0b01000000  //  6     6                active low


// RF_REG_IOCFG0
#define RF_IOCFG0_GDO0_TEMP_SENSOR_MASK             0b10000000
#define RF_IOCFG0_GDO0_TEMP_SENSOR_OFF              0b00000000  //  7     7   analog temperature sensor output: disabled (default)
#define RF_IOCFG0_GDO0_TEMP_SENSOR_ON               0b10000000  //  7     7                                     enabled

#define RF_IOCFG0_GDO0_INV_MASK                     0b01000000
#define RF_IOCFG0_GDO0_NORM                         0b00000000  //  6     6   GDO0 output: active high (default)
#define RF_IOCFG0_GDO0_INV                          0b01000000  //  6     6                active low


// RF_REG_IOCFG2 + REG_IOCFG1 + REG_IOCFG0
#define RF_GDOX_MASK                                0b00111111
#define RF_GDOX_RX_FIFO_FULL                        0x00        //  5     0   Rx FIFO full or above threshold
#define RF_GDOX_RX_FIFO_FULL_OR_PKT_END             0x01        //  5     0   Rx FIFO full or above threshold or reached packet end
#define RF_GDOX_TX_FIFO_ABOVE_THR                   0x02        //  5     0   Tx FIFO above threshold
#define RF_GDOX_TX_FIFO_FULL                        0x03        //  5     0   Tx FIFO full
#define RF_GDOX_RX_FIFO_OVERFLOW                    0x04        //  5     0   Rx FIFO overflowed
#define RF_GDOX_TX_FIFO_UNDERFLOW                   0x05        //  5     0   Tx FIFO underflowed
#define RF_GDOX_SYNC_WORD_SENT_OR_PKT_RECEIVED      0x06        //  5     0   sync word was sent or packet was received
#define RF_GDOX_PKT_RECEIVED_CRC_OK                 0x07        //  5     0   packet received and CRC check passed
#define RF_GDOX_PREAMBLE_QUALITY_REACHED            0x08        //  5     0   received preamble quality is above threshold
#define RF_GDOX_CHANNEL_CLEAR                       0x09        //  5     0   RSSI level below threshold (channel is clear)
#define RF_GDOX_PLL_LOCKED                          0x0A        //  5     0   PLL is locked
#define RF_GDOX_SERIAL_CLOCK                        0x0B        //  5     0   serial data clock
#define RF_GDOX_SERIAL_DATA_SYNC                    0x0C        //  5     0   serial data output in: synchronous mode
#define RF_GDOX_SERIAL_DATA_ASYNC                   0x0D        //  5     0                          asynchronous mode
#define RF_GDOX_CARRIER_SENSE                       0x0E        //  5     0   RSSI above threshold
#define RF_GDOX_CRC_OK                              0x0F        //  5     0   CRC check passed
#define RF_GDOX_RX_HARD_DATA1                       0x16        //  5     0   direct access to demodulated data
#define RF_GDOX_RX_HARD_DATA0                       0x17        //  5     0   direct access to demodulated data
#define RF_GDOX_PA_PD                               0x1B        //  5     0   power amplifier circuit is powered down
#define RF_GDOX_LNA_PD                              0x1C        //  5     0   low-noise amplifier circuit is powered down
#define RF_GDOX_RX_SYMBOL_TICK                      0x1D        //  5     0   direct access to symbol tick of received data
#define RF_GDOX_WOR_EVNT0                           0x24        //  5     0   wake-on-radio event 0
#define RF_GDOX_WOR_EVNT1                           0x25        //  5     0   wake-on-radio event 1
#define RF_GDOX_CLK_256                             0x26        //  5     0   256 Hz clock
#define RF_GDOX_CLK_32K                             0x27        //  5     0   32 kHz clock
#define RF_GDOX_CHIP_RDYN                           0x29        //  5     0    (default for GDO2)
#define RF_GDOX_XOSC_STABLE                         0x2B        //  5     0
#define RF_GDOX_HIGH_Z                              0x2E        //  5     0   high impedance state (default for GDO1)
#define RF_GDOX_HW_TO_0                             0x2F        //  5     0
#define RF_GDOX_CLOCK_XOSC_1                        0x30        //  5     0   crystal oscillator clock: f = f(XOSC)/1
#define RF_GDOX_CLOCK_XOSC_1_5                      0x31        //  5     0                             f = f(XOSC)/1.5
#define RF_GDOX_CLOCK_XOSC_2                        0x32        //  5     0                             f = f(XOSC)/2
#define RF_GDOX_CLOCK_XOSC_3                        0x33        //  5     0                             f = f(XOSC)/3
#define RF_GDOX_CLOCK_XOSC_4                        0x34        //  5     0                             f = f(XOSC)/4
#define RF_GDOX_CLOCK_XOSC_6                        0x35        //  5     0                             f = f(XOSC)/6
#define RF_GDOX_CLOCK_XOSC_8                        0x36        //  5     0                             f = f(XOSC)/8
#define RF_GDOX_CLOCK_XOSC_12                       0x37        //  5     0                             f = f(XOSC)/12
#define RF_GDOX_CLOCK_XOSC_16                       0x38        //  5     0                             f = f(XOSC)/16
#define RF_GDOX_CLOCK_XOSC_24                       0x39        //  5     0                             f = f(XOSC)/24
#define RF_GDOX_CLOCK_XOSC_32                       0x3A        //  5     0                             f = f(XOSC)/32
#define RF_GDOX_CLOCK_XOSC_48                       0x3B        //  5     0                             f = f(XOSC)/48
#define RF_GDOX_CLOCK_XOSC_64                       0x3C        //  5     0                             f = f(XOSC)/64
#define RF_GDOX_CLOCK_XOSC_96                       0x3D        //  5     0                             f = f(XOSC)/96
#define RF_GDOX_CLOCK_XOSC_128                      0x3E        //  5     0                             f = f(XOSC)/128
#define RF_GDOX_CLOCK_XOSC_192                      0x3F        //  5     0                             f = f(XOSC)/192 (default for GDO0)


// RF_REG_FSCTRL1
#define RF_FSCTRL1_MASK                             0b00011111
#define RF_FREQ_IF                                  0x0F        //  4     0   IF frequency setting; f_IF = (f(XOSC) / 2^10) * CC1101_FREQ_IF


// RF_REG_MDMCFG4
#define RF_MDMCFG4_CHANBW_E_MASK                    0b11000000
#define RF_MDMCFG4_CHANBW_E                         0b10000000  //  7     6   channel bandwidth: BW_channel = f(XOSC) / (8 * (4 + CHANBW_M)*2^CHANBW_E) [Hz]

#define RF_MDMCFG4_CHANBW_M_MASK                    0b00110000
#define RF_MDMCFG4_CHANBW_M                         0b00000000  //  5     4       default value for 26 MHz crystal: 203 125 Hz

#define RF_MDMCFG4_DRATE_E_MASK                     0b00001111
#define RF_MDMCFG4_DRATE_E                          0x0C        //  3     0   symbol rate: R_data = (((256 + DRATE_M) * 2^DRATE_E) / 2^28) * f(XOSC) [Baud]


// RF_REG_MDMCFG3
#define RF_MDMCFG3_DRATE_M_MASK                     0b11111111
#define RF_MDMCFG3_DRATE_M                          0x22        //  7     0   default value for 26 MHz crystal: 115 051 Baud


// RF_REG_MDMCFG2
#define RF_MDMCFG2_DEM_DCFILT_MASK                  0b10000000
#define RF_MDMCFG2_DEM_DCFILT_OFF                   0b10000000  //  7     7   digital DC filter: disabled
#define RF_MDMCFG2_DEM_DCFILT_ON                    0b00000000  //  7     7                      enabled - only for data rates above 250 kBaud (default)

#define RF_MDMCFG2_MOD_FORMAT_MASK                  0b01110000
#define RF_MDMCFG2_MOD_FORMAT_2_FSK                 0b00000000  //  6     4   modulation format: 2-FSK (default)
#define RF_MDMCFG2_MOD_FORMAT_GFSK                  0b00010000  //  6     4                      GFSK
#define RF_MDMCFG2_MOD_FORMAT_ASK_OOK               0b00110000  //  6     4                      ASK/OOK
#define RF_MDMCFG2_MOD_FORMAT_4_FSK                 0b01000000  //  6     4                      4-FSK
#define RF_MDMCFG2_MOD_FORMAT_MFSK                  0b01110000  //  6     4                      MFSK - only for data rates above 26 kBaud

#define RF_MDMCFG2_MANCHESTER_EN_MASK               0b00001000
#define RF_MDMCFG2_MANCHESTER_EN_OFF                0b00000000  //  3     3   Manchester encoding: disabled (default)
#define RF_MDMCFG2_MANCHESTER_EN_ON                 0b00001000  //  3     3                        enabled

#define RF_MDMCFG2_SYNC_MODE_MASK                   0b00000111
#define RF_MDMCFG2_SYNC_MODE_NONE                   0b00000000  //  2     0   synchronization: no preamble/sync
#define RF_MDMCFG2_SYNC_MODE_15_16                  0b00000001  //  2     0                    15/16 sync word bits
#define RF_MDMCFG2_SYNC_MODE_16_16                  0b00000010  //  2     0                    16/16 sync word bits (default)
#define RF_MDMCFG2_SYNC_MODE_30_32                  0b00000011  //  2     0                    30/32 sync word bits
#define RF_MDMCFG2_SYNC_MODE_NONE_THR               0b00000100  //  2     0                    no preamble sync, carrier sense above threshold
#define RF_MDMCFG2_SYNC_MODE_15_16_THR              0b00000101  //  2     0                    15/16 sync word bits, carrier sense above threshold
#define RF_MDMCFG2_SYNC_MODE_16_16_THR              0b00000110  //  2     0                    16/16 sync word bits, carrier sense above threshold
#define RF_MDMCFG2_SYNC_MODE_30_32_THR              0b00000111  //  2     0                    30/32 sync word bits, carrier sense above threshold




// RF_REG_MDMCFG1
#define RF_MDMCFG1_FEC_MASK                         0b10000000
#define RF_FEC_OFF                                  0b00000000  //  7     7   forward error correction: disabled (default)
#define RF_FEC_ON                                   0b10000000  //  7     7                             enabled - only for fixed packet length

#define RF_MDMCFG1_NUM_PREAMBLE_MASK                0b01110000
#define RF_NUM_PREAMBLE_2                           0b00000000  //  6     4   number of preamble bytes: 2
#define RF_NUM_PREAMBLE_3                           0b00010000  //  6     4                             3
#define RF_NUM_PREAMBLE_4                           0b00100000  //  6     4                             4 (default)
#define RF_NUM_PREAMBLE_6                           0b00110000  //  6     4                             6
#define RF_NUM_PREAMBLE_8                           0b01000000  //  6     4                             8
#define RF_NUM_PREAMBLE_12                          0b01010000  //  6     4                             12
#define RF_NUM_PREAMBLE_16                          0b01100000  //  6     4                             16
#define RF_NUM_PREAMBLE_24                          0b01110000  //  6     4                             24

#define RF_MDMCFG1_CHANSPC_E_MASK                   0b00000010
#define RF_CHANSPC_E                                0x02        //  1     0   channel spacing: df_channel = (f(XOSC) / 2^18) * (256 + CHANSPC_M) * 2^CHANSPC_E [Hz]


// RF_REG_MDMCFG0
#define RF_MDMCFG0_CHANSPC_M_MASK                   0b11111111
#define RF_CHANSPC_M                                0xF8        //  7     0   default value for 26 MHz crystal: 199 951 kHz


// RF_REG_CHANNR
#define RF_CHANNR_CHAN_MASK                         0b11111111
#define RF_CHAN                                     0x00        //  7     0   channel number


// RF_REG_DEVIATN
#define RF_DEVIATION_E_MASK                         0b01110000
#define RF_DEVIATION_E                              0b01000000  //  6     4   frequency deviation: f_dev = (f(XOSC) / 2^17) * (8 + DEVIATION_M) * 2^DEVIATION_E [Hz]
#define RF_DEVIATION_M_MASK                         0b00000111
#define RF_DEVIATION_M                              0b00000111  //  2     0       default value for 26 MHz crystal: +- 47 607 Hz


// RF_REG_FREND1
#define RF_FREND1_LNA_CURRENT_MASK                  0b11000000
#define RF_FREND1_LNA_CURRENT                       0x01        //  7     6   front-end LNA PTAT current output adjustment
#define RF_FREND1_LNA2MIX_CURRENT_MASK              0b00110000
#define RF_FREND1_LNA2MIX_CURRENT                   0x01        //  5     4   front-end PTAT output adjustment
#define RF_FREND1_LODIV_BUF_CURRENT_MASK            0b00001100
#define RF_FREND1_LODIV_BUF_CURRENT_RX              0x01        //  3     2   Rx LO buffer current adjustment
#define RF_FREND1_MIX_CURRENT_MASK                  0b00000011
#define RF_FREND1_MIX_CURRENT                       0x02        //  1     0   mixer current adjustment


// RF_REG_MCSM0
#define RF_MCSM0_FS_AUTOCAL_MASK                    0b00110000
#define RF_MCSM0_FS_AUTOCAL_NEVER                   0b00000000  //  5     4   automatic calibration: never (default)
#define RF_MCSM0_FS_AUTOCAL_IDLE_TO_RXTX            0b00010000  //  5     4                          every transition from idle to Rx/Tx
#define RF_MCSM0_FS_AUTOCAL_RXTX_TO_IDLE            0b00100000  //  5     4                          every transition from Rx/Tx to idle
#define RF_MCSM0_FS_AUTOCAL_RXTX_TO_IDLE_4TH        0b00110000  //  5     4                          every 4th transition from Rx/Tx to idle

#define RF_MCSM0_PO_TIMEOUT_MASK                    0b00001100
#define RF_MCSM0_PO_TIMEOUT_COUNT_1                 0b00000000  //  3     2   number of counter expirations before CHP_RDYN goes low: 1 (default)
#define RF_MCSM0_PO_TIMEOUT_COUNT_16                0b00000100  //  3     2                                                           16
#define RF_MCSM0_PO_TIMEOUT_COUNT_64                0b00001000  //  3     2                                                           64
#define RF_MCSM0_PO_TIMEOUT_COUNT_256               0b00001100  //  3     2                                                           256

#define RF_MCSM0_PIN_CTRL_MASK                      0b00000010
#define RF_MCSM0_PIN_CTRL_OFF                       0b00000000  //  1     1   pin radio control: disabled (default)
#define RF_MCSM0_PIN_CTRL_ON                        0b00000010  //  1     1                      enabled

#define RF_MCSM0_XOSC_FORCE_CTRL_MASK               0b00000001
#define RF_MCSM0_XOSC_FORCE_OFF                     0b00000000  //  0     0   do not force XOSC to remain on in sleep (default)
#define RF_MCSM0_XOSC_FORCE_ON                      0b00000001  //  0     0   force XOSC to remain on in sleep


// RF_REG_FOCCFG
#define RF_FOCCFG_FOC_BS_CS_MASK                    0b00100000
#define RF_FOCCFG_FOC_BS_CS_GATE_OFF                0b00000000  //  5     5   do not freeze frequency compensation until CS goes high
#define RF_FOCCFG_FOC_BS_CS_GATE_ON                 0b00100000  //  5     5   freeze frequency compensation until CS goes high (default)

#define RF_FOCCFG_FOC_PRE_MASK                      0b00011000
#define RF_FOCCFG_FOC_PRE_K                         0b00000000  //  4     3   frequency compensation loop gain before sync word: K
#define RF_FOCCFG_FOC_PRE_2K                        0b00001000  //  4     3                                                      2K
#define RF_FOCCFG_FOC_PRE_3K                        0b00010000  //  4     3                                                      3K (default)
#define RF_FOCCFG_FOC_PRE_4K                        0b00011000  //  4     3                                                      4K

#define RF_FOCCFG_FOC_POST_MASK                     0b00000100
#define RF_FOCCFG_FOC_POST_K                        0b00000000  //  2     2   frequency compensation loop gain after sync word: same as FOC_PRE
#define RF_FOCCFG_FOC_POST_K_2                      0b00000100  //  2     2                                                     K/2 (default)

#define RF_FOCCFG_FOC_LIMIT_MASK                    0b00000011
#define RF_FOCCFG_FOC_LIMIT_NO_COMPENSATION         0b00000000  //  1     0   frequency compensation saturation point: no compensation - required for ASK/OOK
#define RF_FOCCFG_FOC_LIMIT_BW_CHAN_8               0b00000001  //  1     0                                            +- BW_chan/8
#define RF_FOCCFG_FOC_LIMIT_BW_CHAN_4               0b00000010  //  1     0                                            +- BW_chan/4 (default)
#define RF_FOCCFG_FOC_LIMIT_BW_CHAN_2               0b00000011  //  1     0                                            +- BW_chan/2


// RF_REG_BSCFG
#define RF_BSCFG_BS_PRE_KI_MASK                     0b11000000
#define RF_BSCFG_BS_PRE_KI                          0b00000000  //  7     6   clock recovery integral gain before sync word: Ki
#define RF_BSCFG_BS_PRE_2KI                         0b01000000  //  7     6                                                  2Ki (default)
#define RF_BSCFG_BS_PRE_3KI                         0b10000000  //  7     6                                                  3Ki
#define RF_BSCFG_BS_PRE_4KI                         0b11000000  //  7     6                                                  4Ki

#define RF_BSCFG_BS_PRE_KP_MASK                     0b00110000
#define RF_BSCFG_BS_PRE_KP                          0b00000000  //  5     4   clock recovery proportional gain before sync word: Kp
#define RF_BSCFG_BS_PRE_2KP                         0b00010000  //  5     4                                                      2Kp
#define RF_BSCFG_BS_PRE_3KP                         0b00100000  //  5     4                                                      3Kp (default)
#define RF_BSCFG_BS_PRE_4KP                         0b00110000  //  5     4                                                      4Kp

#define RF_BSCFG_BS_POST_KI_MASK                    0b00001000
#define RF_BSCFG_BS_POST_KI                         0b00000000  //  3     3   clock recovery integral gain after sync word: same as BS_PRE
#define RF_BSCFG_BS_POST_KI_2                       0b00001000  //  3     3                                                 Ki/2 (default)

#define RF_BSCFG_BS_POST_KP_MASK                    0b00000100
#define RF_BSCFG_BS_POST_KP                         0b00000000  //  2     2   clock recovery proportional gain after sync word: same as BS_PRE
#define RF_BSCFG_BS_POST_KP_1                       0b00000100  //  2     2                                                     Kp (default)

#define RF_BSCFG_BS_LIMIT_MASK                      0b00000011
#define RF_BSCFG_BS_LIMIT_NO_COMPENSATION           0b00000000  //  1     0   data rate compensation saturation point: no compensation
#define RF_BSCFG_BS_LIMIT_3_125                     0b00000001  //  1     0                                            +- 3.125 %
#define RF_BSCFG_BS_LIMIT_6_25                      0b00000010  //  1     0                                            +- 6.25 %
#define RF_BSCFG_BS_LIMIT_12_5                      0b00000011  //  1     0                                            +- 12.5 %


// RF_REG_AGCCTRL2
#define RF_AGCCTRL2_MAX_DVGA_MASK                   0b11000000
#define RF_AGCCTRL2_MAX_DVGA_GAIN_0                 0b00000000  //  7     6   reduce maximum available DVGA gain: no reduction (default)
#define RF_AGCCTRL2_MAX_DVGA_GAIN_1                 0b01000000  //  7     6                                       disable top gain setting
#define RF_AGCCTRL2_MAX_DVGA_GAIN_2                 0b10000000  //  7     6                                       disable top two gain setting
#define RF_AGCCTRL2_MAX_DVGA_GAIN_3                 0b11000000  //  7     6                                       disable top three gain setting

#define RF_AGCCTRL2_LNA_GAIN_REDUCE_MASK            0b00111000
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_0_DB            0b00000000  //  5     3   reduce maximum LNA gain by: 0 dB (default)
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_2_6_DB          0b00001000  //  5     3                               2.6 dB
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_6_1_DB          0b00010000  //  5     3                               6.1 dB
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_7_4_DB          0b00011000  //  5     3                               7.4 dB
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_9_2_DB          0b00100000  //  5     3                               9.2 dB
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_11_5_DB         0b00101000  //  5     3                               11.5 dB
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_14_6_DB         0b00110000  //  5     3                               14.6 dB
#define RF_AGCCTRL2_LNA_GAIN_REDUCE_17_1_DB         0b00111000  //  5     3                               17.1 dB

#define RF_AGCCTRL2_MAGN_TARGET_MASK                0b00000111
#define RF_AGCCTRL2_MAGN_TARGET_24_DB               0b00000000  //  2     0   average amplitude target for filter: 24 dB
#define RF_AGCCTRL2_MAGN_TARGET_27_DB               0b00000001  //  2     0                                        27 dB
#define RF_AGCCTRL2_MAGN_TARGET_30_DB               0b00000010  //  2     0                                        30 dB
#define RF_AGCCTRL2_MAGN_TARGET_33_DB               0b00000011  //  2     0                                        33 dB (default)
#define RF_AGCCTRL2_MAGN_TARGET_36_DB               0b00000100  //  2     0                                        36 dB
#define RF_AGCCTRL2_MAGN_TARGET_38_DB               0b00000101  //  2     0                                        38 dB
#define RF_AGCCTRL2_MAGN_TARGET_40_DB               0b00000110  //  2     0                                        40 dB
#define RF_AGCCTRL2_MAGN_TARGET_42_DB               0b00000111  //  2     0                                        42 dB


// RF_REG_AGCCTRL1
#define RF_AGCCTRL1_AGC_LNA_PRIORITY_MASK           0b01000000
#define RF_AGCCTRL1_AGC_LNA_PRIORITY_LNA2           0b00000000  //  6     6   LNA priority setting: LNA2 first
#define RF_AGCCTRL1_AGC_LNA_PRIORITY_LNA            0b01000000  //  6     6                         LNA first (default)

#define RF_AGCCTRL1_CARRIER_SENSE_REL_THR_MASK      0b00110000
#define RF_AGCCTRL1_CARRIER_SENSE_REL_THR_OFF       0b00000000  //  5     4   RSSI relative change to assert carrier sense: disabled (default)
#define RF_AGCCTRL1_CARRIER_SENSE_REL_THR_6_DB      0b00010000  //  5     4                                                 6 dB
#define RF_AGCCTRL1_CARRIER_SENSE_REL_THR_10_DB     0b00100000  //  5     4                                                 10 dB
#define RF_AGCCTRL1_CARRIER_SENSE_REL_THR_14_DB     0b00110000  //  5     4                                                 14 dB

#define RF_AGCCTRL1_CARRIER_SENSE_ABS_MASK          0b00001111
#define RF_AGCCTRL1_CARRIER_SENSE_ABS_THR           0x00        //  3     0   RSSI threshold to assert carrier sense in 2s compliment, Thr = MAGN_TARGET + CARRIER_SENSE_ABS_TH [dB]


// RF_REG_AGCCTRL0
#define RF_AGCCTRL0_HYST_LEVEL_MASK                 0b11000000
#define RF_AGCCTRL0_HYST_LEVEL_NONE                 0b00000000  //  7     6   AGC hysteresis level: none
#define RF_AGCCTRL0_HYST_LEVEL_LOW                  0b01000000  //  7     6                         low
#define RF_AGCCTRL0_HYST_LEVEL_MEDIUM               0b10000000  //  7     6                         medium (default)
#define RF_AGCCTRL0_HYST_LEVEL_HIGH                 0b11000000  //  7     6                         high

#define RF_AGCCTRL0_WAIT_TIME_MASK                  0b00110000
#define RF_AGCCTRL0_WAIT_TIME_8_SAMPLES             0b00000000  //  5     4   AGC wait time: 8 samples
#define RF_AGCCTRL0_WAIT_TIME_16_SAMPLES            0b00010000  //  5     4                  16 samples (default)
#define RF_AGCCTRL0_WAIT_TIME_24_SAMPLES            0b00100000  //  5     4                  24 samples
#define RF_AGCCTRL0_WAIT_TIME_32_SAMPLES            0b00110000  //  5     4                  32 samples

#define RF_AGCCTRL0_AGC_FREEZE_MASK                 0b00001100
#define RF_AGCCTRL0_AGC_FREEZE_NEVER                0b00000000  //  3     2   freeze AGC gain: never (default)
#define RF_AGCCTRL0_AGC_FREEZE_SYNC_WORD            0b00000100  //  3     2                    when sync word is found
#define RF_AGCCTRL0_AGC_FREEZE_MANUAL_A             0b00001000  //  3     2                    manually freeze analog control
#define RF_AGCCTRL0_AGC_FREEZE_MANUAL_AD            0b00001100  //  3     2                    manually freeze analog and digital control

#define RF_AGCCTRL0_FILTER_LENGTH_MASK              0b00000011
#define RF_AGCCTRL0_FILTER_LENGTH_8                 0b00000000  //  1     0   averaging length for channel filter: 8 samples
#define RF_AGCCTRL0_FILTER_LENGTH_16                0b00000001  //  1     0                                        16 samples (default)
#define RF_AGCCTRL0_FILTER_LENGTH_32                0b00000010  //  1     0                                        32 samples
#define RF_AGCCTRL0_FILTER_LENGTH_64                0b00000011  //  1     0                                        64 samples

#define RF_AGCCTRL0_ASK_OOK_BOUNDARY_MASK           0b00000011
#define RF_AGCCTRL0_ASK_OOK_BOUNDARY_4_DB           0b00000000  //  1     0   ASK/OOK decision boundary: 4 dB
#define RF_AGCCTRL0_ASK_OOK_BOUNDARY_8_DB           0b00000001  //  1     0                              8 dB (default)
#define RF_AGCCTRL0_ASK_OOK_BOUNDARY_12_DB          0b00000010  //  1     0                              12 dB
#define RF_AGCCTRL0_ASK_OOK_BOUNDARY_16_DB          0b00000011  //  1     0                              16 dB


// RF_REG_FSCAL3
#define RF_FSCAL3_CHP_CURR_CAL_MASK                 0b00110000
#define RF_FSCAL3_CHP_CURR_CAL_OFF                  0b00000000  //  5     4   disable charge pump calibration
#define RF_FSCAL3_CHP_CURR_CAL_ON                   0b00100000  //  5     4   enable charge pump calibration (default)
#define RF_FSCAL3                                   0x09        //  3     0   charge pump output current: I_out = I_0 * 2^(FSCAL3/4) [A]

// RF_REG_FSCAL2
#define RF_FSCAL2_VCO_CORE_MASK                     0b00100000
#define RF_FSCAL2_VCO_CORE_LOW                      0b00000000  //  5     5   VCO: low (default)
#define RF_FSCAL2_VCO_CORE_HIGH                     0b00100000  //  5     5        high
#define RF_FSCAL2                                   0x0A        //  4     0   VCO current result/override

// RF_REG_FSCAL1
#define RF_FSCAL1                                   0x20        //  5     0   capacitor array setting for coarse VCO tuning

// RF_REG_FSCAL0
#define RF_FSCAL0                                   0x0D        //  6     0   frequency synthesizer calibration setting


// RF_REG_PKTCTRL1
#define RF_PKTCTRL1_PQT_MASK                        0b11100000
#define RF_PKTCTRL1_PQT                             0x00        //  7     5   preamble quality threshold

#define RF_PKTCTRL1_CRC_AUTOFLUSH_MASK              0b00001000
#define RF_PKTCTRL1_CRC_AUTOFLUSH_OFF               0b00000000  //  3     3   automatic Rx FIFO flush on CRC check fail: disabled (default)
#define RF_PKTCTRL1_CRC_AUTOFLUSH_ON                0b00001000  //  3     3                                              enabled

#define RF_PKTCTRL1_APPEND_STATUS_MASK              0b00000100
#define RF_PKTCTRL1_APPEND_STATUS_OFF               0b00000000  //  2     2   append 2 status bytes to packet: disabled
#define RF_PKTCTRL1_APPEND_STATUS_ON                0b00000100  //  2     2                                    enabled (default)

#define RF_PKTCTRL1_ADR_CHK_MASK                    0b00000011
#define RF_PKTCTRL1_ADR_CHK_NONE                    0b00000000  //  1     0   address check: none (default)
#define RF_PKTCTRL1_ADR_CHK_NO_BROADCAST            0b00000001  //  1     0                  without broadcast
#define RF_PKTCTRL1_ADR_CHK_SINGLE_BROADCAST        0b00000010  //  1     0                  broadcast address 0x00
#define RF_PKTCTRL1_ADR_CHK_DOUBLE_BROADCAST        0b00000011  //  1     0                  broadcast addresses 0x00 and 0xFF


// RF_REG_PKTCTRL0
#define RF_PKTCTRL0_WHITE_DATA_MASK                 0b01000000
#define RF_PKTCTRL0_WHITE_DATA_OFF                  0b00000000  //  6     6   data whitening: disabled
#define RF_PKTCTRL0_WHITE_DATA_ON                   0b01000000  //  6     6                   enabled (default)

#define RF_PKTCTRL0_PKT_FORMAT_MASK                 0b00110000
#define RF_PKTCTRL0_PKT_FORMAT_NORMAL               0b00000000  //  5     4   packet format: normal (FIFOs)
#define RF_PKTCTRL0_PKT_FORMAT_SYNCHRONOUS          0b00010000  //  5     4                  synchronous serial
#define RF_PKTCTRL0_PKT_FORMAT_RANDOM               0b00100000  //  5     4                  random transmissions
#define RF_PKTCTRL0_PKT_FORMAT_ASYNCHRONOUS         0b00110000  //  5     4                  asynchronous serial

#define RF_PKTCTRL0_CRC_MASK                        0b00000100
#define RF_PKTCTRL0_CRC_OFF                         0b00000000  //  2     2   CRC disabled
#define RF_PKTCTRL0_CRC_ON                          0b00000100  //  2     2   CRC enabled (default)

#define RF_PKTCTRL0_LENGTH_CONFIG_MASK              0b00000011
#define RF_PKTCTRL0_LENGTH_CONFIG_FIXED             0b00000000  //  1     0   packet length: fixed
#define RF_PKTCTRL0_LENGTH_CONFIG_VARIABLE          0b00000001  //  1     0                  variable (default)
#define RF_PKTCTRL0_LENGTH_CONFIG_INFINITE          0b00000010  //  1     0                  infinite


// RF_REG_PKTLEN
#define RF_PACKET_LENGTH                            0xFF        //  7     0   packet length in bytes
*/

#endif  // __CC1101_REGS_FSK_H__