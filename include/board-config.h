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

#ifndef IOHC_BOARD_H
#define IOHC_BOARD_H

#define RADIO_SX127X
#define Regulatory_Domain_EU_868
//#define RADIO_SX126X
#define BOARD_MODEL BOARD_HELTEC32_V3
/*
 * Board pins definitions
 */
// OK Heltec Wifi ESP32 Lora v2.1
#define RADIO_SCLK_PIN       5
#define RADIO_MISO_PIN      19
#define RADIO_MOSI_PIN      27
#define RADIO_CS_PIN        18
#define RADIO_DIO0_PIN      26
#define RADIO_RST_PIN       14
#define BOARD_LED_PIN       25
#ifdef LILYGO
#define RADIO_DIO1_PIN      33 //LILYGO
#define RADIO_DIO2_PIN      32 //LILYGO
#elif defined(HELTEC)
#define RADIO_DIO1_PIN      35 //HELTEC
#define RADIO_DIO2_PIN      34 //HELTEC
#define RADIO_BUSY_PIN      32
#endif

// I2C pin definitions for OLED or peripherals
#if defined(LILYGO)
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#elif defined(HELTEC)
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 15
#else
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#endif

// OK LilyGo Wifi ESP32 Lora v2.1.6
// https://github.com/LilyGO/ESP32-Paxcounter/blob/master/src/hal/ttgov2.h 


#if defined(ESP32)
#define RADIO_MOSI             RADIO_MOSI_PIN //                 23  // Default VSPI
#define RADIO_MISO             RADIO_MISO_PIN //                 19  // Default VSPI
#define RADIO_SCLK             RADIO_SCLK_PIN //                 18  // Default VSPI
#if defined(RADIO_SX127X)
#define RADIO_RESET        RADIO_RST_PIN  //                 12
#define RADIO_NSS          RADIO_CS_PIN   //                 25
#endif
#if defined(RADIO_SX127X)
//#define RADIO_DIO_0                             5   // NodeMCU D1
//#define RADIO_DIO_1                             2   // NodeMCU D4 // Not used - No wire
//#define RADIO_DIO_2                             2   // NodeMCU D4 // Not used - No wire
//#define RADIO_DIO_4                             2   // NodeMCU D4
#define RADIO_DIO_0                             RADIO_DIO0_PIN //                 35
//#define RADIO_DIO_1                             34      // Not used - No wire
//#define RADIO_DIO_2                             34      // Not used - No wire
#define RADIO_DIO_4                             RADIO_DIO2_PIN //                 34
#endif
#if defined(RADIO_SX127X)
#define RADIO_PACKET_AVAIL                      RADIO_DIO_0     // Packet Received / CRC ok from Radio
#define RADIO_DATA_AVAIL                        RADIO_DIO_1     // FIFO empty from Radio
#define RADIO_RXTIMEOUT                         RADIO_DIO_2     // Radio Rx Sequencer timeout (used to switch the receiver frequency)
#define RADIO_PREAMBLE_DETECTED                 RADIO_DIO_4     // Preamble detected from Radio (used instead of FIFO empty)
#endif

#define SPI_CLK_FRQ                                 10000000

/*
 * Defines the time required for the TCXO to wakeup [ms].
 */

#define BOARD_TCXO_WAKEUP_TIME                      0
#define BOARD_READY_AFTER_POR						10000

#define PREAMBLE_MSB                                0x00
#define PREAMBLE_LSB        64//                        52  // 0x34: 12ms to have receiver up and running (52 0x55 bytes - 13,54mS)

#define SYNC_BYTE_1                                 0xff
#define SYNC_BYTE_2                                 0x33    // Sync word - Size must be set to 2; first byte 0xff then 0x33 size-1 times

//#define SYNC_BYTE_2_ENC                             0xB3    // Sync word Inverted + Encoded with start & stop bits

#define CHANNEL1  868250000 //2W
#define CHANNEL2  868950000 //1W 2W
#define CHANNEL3  869850000 //2W

#define FREQS2SCAN              {CHANNEL2, CHANNEL3, CHANNEL1}
#define MAX_FREQS                3       // Number of Frequencies to scan through Fast Hopping set to 1 to disable FHSS

//#if defined(HELTEC)
#define SCAN_LED                  BOARD_LED_PIN //              22
//#endif
#define RX_LED                        SCAN_LED

#endif

#endif
