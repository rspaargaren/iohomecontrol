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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define SX127x_OSCILLATOR_FREQUENCY 32000000.0f

namespace Radio {
void print_op_mode(uint8_t value) {
  printf("0x01 RegOpMode: %2.2x\n", value);
  if ((value & 0b10000000) == 0b10000000) {
    printf("\tLongRangeMode=LORA\n");
  } else {
    printf("\tLongRangeMode=FSK\n");
  }
  if (((value & 0b01000000) >> 6) == 0) {
    printf("\tAccessSharedReg=Access LoRa registers\n");
  } else {
    printf("\tAccessSharedReg=Access FSK registers\n");
  }
  if (((value & 0b00001000) >> 3) == 0) {
    printf("\tLowFrequencyModeOn=High Frequency Mode\n");
  } else {
    printf("\tLowFrequencyModeOn=Low Frequency Mode\n");
  }
  printf("sx127x_set_init(SX127x_MODE_STANDBY, SX127x_MODULATION_FSK, device)\n");
  uint8_t mode = (value & 0b111);
  switch (mode) {
    case 0b000:
      printf("\tMode=SLEEP\n");
      break;
    case 0b001:
      printf("\tMode=STDBY\n");
      break;
    case 0b010:
      printf("\tMode=Frequency synthesis TX\n");
      break;
    case 0b011:
      printf("\tMode=Transmit (TX)\n");
      break;
    case 0b100:
      printf("\tMode=Frequency synthesis RX (FSRX)\n");
      break;
    case 0b101:
      printf("\tMode=Receive continuous\n");
      break;
    case 0b110:
      printf("\tMode=receive single\n");
      break;
    case 0b111:
      printf("\tMode=Channel activity detection\n");
      break;
  }
  printf("sx127x_set_mode(sx127x_mode_t opmode, device);\n");
}

void print_ocp(uint8_t value) {
  printf("0x0b: RegOcp:\n");
  if ((value & 0b100000) != 0) {
    printf("\tOcpOn=OCP enabled\n");
  } else {
    printf("\tOcpOn=OCP disabled\n");
  }
  printf("\tOcpTrim=0x%x\n", (value & 0b11111));
}

double calculate_bw(uint8_t value) {
  uint8_t mantissa = 0;
  switch (((value & 0b11000) >> 3)) {
    case 0b10:
      mantissa = 24;
      break;
    case 0b01:
      mantissa = 20;
      break;
    case 0b00:
      mantissa = 16;
      break;
    case 0b11:
      // invalid - should fail
      mantissa = 0;
      break;
  }
  return 32000000.0 / (mantissa * ((uint32_t) 1 << ((value & 0b111) + 2)));
}

void print_lna(uint8_t value) {
  printf("0x0c: RegLna:\n");
  printf("\tLnaGain=%d\n", ((value & 0b11100000) >> 5));
  printf("\tLnaBoostLf=%d\n", ((value & 0b11000) >> 3));
  if (((value & 0b11)) != 0) {
    printf("\tLnaBoostHf=Boost on\n");
  } else {
    printf("\tLnaBoostHf=Default LNA current\n");
  }
}

int dump_fsk_registers(const uint8_t *regs) {
  uint8_t value = regs[0x01];
  print_op_mode(value);
  printf("0x02: RegBitrateMsb:\n");
  double bit_rate = SX127x_OSCILLATOR_FREQUENCY / (((regs[0x02] << 8) | (regs[0x03])) + regs[0x5D] / 16.0);
  printf("\tBitRate=%f\n", bit_rate);
  printf("sx127x_fsk_ook_set_bitrate(%d, device);\n", (uint16_t)bit_rate);
  printf("0x04: RegFdevMsb\n");
  double freq_deviation = (SX127x_OSCILLATOR_FREQUENCY / (1 << 19)) * (((regs[0x04] & 0b111111) << 8) | regs[0x05]);
  printf("\tFdev=%f\n", freq_deviation);
  printf("sx127x_fsk_set_fdev(%d, device);\n", (uint16_t)freq_deviation);
  uint64_t freq = (((uint64_t) regs[0x06]) << 16) | (((uint64_t) regs[0x07]) << 8) | (regs[0x08]);
  printf("0x06: RegFr:\n");
  freq = (freq * SX127x_OSCILLATOR_FREQUENCY) / (1 << 19);
  printf("\tFrf=%" PRIu64 "\n", freq);
  printf("sx127x_set_frequency(%llu, device);\n", freq);
  printf("0x09: RegPaConfig:\n");
  value = regs[0x09];
  if ((value & 0b10000000) == 0b10000000) {
    printf("\tPaSelect=PA_BOOST pin\n");
  } else {
    printf("\tPaSelect=RFO pin\n");
  }
  printf("\tMaxPower=0x%x\n", ((value & 0b110000) >> 4));
  printf("\tOutputPower=0x%x\n", ((value & 0b1111)));
  printf("sx127x_tx_set_pa_config(sx127x_pa_pin_t pin, int power, sx127x *device);");
  printf("0x0a: RegPaRamp:\n");
  printf("\tModulationShaping=");
  value = (regs[0x0a] & 0b01100000) >> 5;
  switch (value) {
    case 0b00:
      printf("No shaping");
      break;
    case 0b01:
      printf("Gaussian filter BT = 1.0");
      break;
    case 0b10:
      printf("Gaussian filter BT = 0.5");
      break;
    case 0b11:
      printf("Gaussian filter BT = 0.3");
      break;
  }
  printf("\n");
  printf("\tPaRamp=0x%x\n", (regs[0x0a] & 0b1111));
  print_ocp(regs[0x0b]);
  print_lna(regs[0x0c]);
  printf("0x0d: RegRxConfig\n");
  if ((regs[0x0d] & 0b10000000) != 0) {
    printf("\tRestartRxOnCollision=Automatic restart On\n");
  } else {
    printf("\tRestartRxOnCollision=No automatic Restart\n");
  }
  if ((regs[0x0d] & 0b1000000) != 0) {
    printf("\tRestartRxWithoutPllLock=Manual Restart of the Receiver\n");
  } else {
    printf("\tRestartRxWithoutPllLock=No restart\n");
  }
  if ((regs[0x0d] & 0b100000) != 0) {
    printf("\tRestartRxWithPllLock=Manual Restart of the Receiver\n");
  } else {
    printf("\tRestartRxWithPllLock=No restart\n");
  }
  if ((regs[0x0d] & 0b10000) != 0) {
    printf("\tAfcAutoOn=AFC is performed at each receiver startup\n");
  } else {
    printf("\tAfcAutoOn=No AFC performed at receiver startup\n");
  }
  if ((regs[0x0d] & 0b1000) != 0) {
    printf("\tAgcAutoOn=LNA gain is controlled by the AGC\n");
  } else {
    printf("\tAgcAutoOn=LNA gain forced by the LnaGain Setting\n");
  }
  uint8_t trigger_event = (regs[0x0d] & 0b111);
  switch (trigger_event) {
    case 0b000:
      printf("\tRxTrigger=None\n");
      break;
    case 0b001:
      printf("\tRxTrigger=RSSI\n");
      break;
    case 0b110:
      printf("\tRxTrigger=PreambleDetect\n");
      break;
    case 0b111:
      printf("\tRxTrigger=RSSI and PreambleDetect\n");
      break;
  }
  printf("sx127x_fsk_ook_rx_set_trigger(SX127X_RX_TRIGGER_PREAMBLE, device)");
  printf("0x0e: RegRssiConfig\n");
  printf("\tRssiOffset=%d\n", (regs[0x0e] & 0b11111000) >> 3);
  printf("\tRssiSmoothing=%d\n", (regs[0x0e] & 0b111));
  printf("0x0f: RegRssiCollision\n");
  printf("\tRssiCollisionThreshold=%d\n", (regs[0x0f]));
  printf("0x10: RegRssiThresh\n");
  printf("\tRssiThreshold=%d\n", (regs[0x10] / 2));
  printf("0x11: RegRssiValue\n");
  printf("\tRssiValue=%f\n", (regs[0x11] / 2.0f));
  printf("0x12: RegRxBw\n");
  printf("\tRxBw=%f\n", calculate_bw(regs[0x12]));
  printf("0x13: RegAfcBw\n");
  printf("\tAfcBw=%f\n", calculate_bw(regs[0x13]));
  printf("0x17 0x18 0x19: RegRes TODO\n");
  printf("0x1a 0x1b 0x1c 0x1d 0x1e: RegAfcFei TODO\n");
  if ((regs[0x1a] & 0b1) == 1) {
    printf("\tAfcAutoClearOn=AFC register is not cleared at the beginning of the automatic AFC phase\n");
  } else {
    printf("\tAfcAutoClearOn=AFC register is cleared at the beginning of the automatic AFC phase\n");
  }
  /*    From SKA
				PreambleDetectorOn = ((r.Value >> 7) & 1) == 1;
				PreambleDetectorSize = (byte)(((r.Value >> 5) & 3) + 1);
				PreambleDetectorTol = (byte)(r.Value & 0x1Fu);
*/
  printf("0x1f: RegPreambleDetect\n");
  if ((regs[0x1f] & 0b10000000) != 0) {
    printf("\tPreambleDetectorOn=1\n");
  } else {
    printf("\tPreambleDetectorOn=0\n");
  }
  printf("\tPreambleDetectorSize=%d\n", ((regs[0x1f] & 0b1100000) >> 5) + 1);
  printf("\tPreambleDetectorTol=%d\n", (regs[0x1f] & 0b11111));
  printf("sx127x_fsk_ook_rx_set_preamble_detector(true, 2, 0x0A, device)");
  printf("0x20: RegRxTimeout1\n");
  printf("\tTimeoutRxRssi=%d\n", regs[0x20]);
  printf("0x21: RegRxTimeout2\n");
  printf("\tTimeoutRxPreamble=%d\n", regs[0x21]);
  printf("0x22: RegRxTimeout3\n");
  printf("\tTimeoutSignalSync=%d\n", regs[0x22]);
  printf("0x23: RegRxDelay\n");
  printf("\tInterPacketRxDelay=%d\n", regs[0x23]);
  printf("0x24: RegOsc\n");
  switch (regs[0x24] & 0b111) {
    case 0b000:
      printf("\tClkOut=FXOSC\n");
      break;
    case 0b001:
      printf("\tClkOut=FXOSC/2\n");
      break;
    case 0b010:
      printf("\tClkOut=FXOSC/4\n");
      break;
    case 0b011:
      printf("\tClkOut=FXOSC/8\n");
      break;
    case 0b100:
      printf("\tClkOut=FXOSC/16\n");
      break;
    case 0b101:
      printf("\tClkOut=FXOSC/32\n");
      break;
    case 0b110:
      printf("\tClkOut=RC (automatically enabled)\n");
      break;
    case 0b111:
      printf("\tClkOut=OFF\n");
      break;
  }
  printf("0x25 0x26: RegPreambleMsb\n");
  printf("\tPreambleSize=%d\n", (regs[0x25] << 8) | regs[0x26]);
  printf("0x27: RegSyncConfig\n");
  switch ((regs[0x27] & 0b11000000) >> 6) {
    case 0b00:
      printf("\tAutoRestartRxMode=Off\n");
      break;
    case 0b01:
      printf("\tAutoRestartRxMode=On, without waiting for the PLL to re-lock\n");
      break;
    case 0b10:
      printf("\tAutoRestartRxMode=On, wait for the PLL to lock (frequency changed)\n");
      break;
    case 0b11:
      printf("\tAutoRestartRxMode=Invalid\n");
      break;
  }
  if ((regs[0x27] & 0b100000) != 0) {
    printf("\tPreamblePolarity=0x55\n");
  } else {
    printf("\tPreamblePolarity=0xAA\n");
  }
  if ((regs[0x27] & 0b10000) != 0) {
    printf("\tSyncOn=On\n");
  } else {
    printf("\tSyncOn=Off\n");
  }
  printf("sx127x_fsk_ook_set_preamble_type(SX127X_PREAMBLE_55, device)");
  uint8_t sync_size = (regs[0x27] & 0b111) + 1;
  printf("\tSyncSize=%d\n", sync_size);
  printf("0x28 -> 0x2F: RegSyncValue\n");
  printf("\tSyncValue=");
  for (int i = 0; i < sync_size; i++) {
    printf("%x", regs[0x28 + i]);
  }
  printf("sx127x_fsk_ook_set_syncword(syncWord, %d, device)\n",sync_size);
  printf("0x30: RegPacketConfig1\n");
  if ((regs[0x30] & 0b10000000) != 0) {
    printf("\tPacketFormat=Variable\n");
  } else {
    printf("\tPacketFormat=Fixed\n");
  }
  switch ((regs[0x30] & 0b1100000) >> 5) {
    case 0b00:
      printf("\tDcFree=Off (NRZ)\n");
      break;
    case 0b01:
      printf("\tDcFree=Manchester\n");
      break;
    case 0b10:
      printf("\tDcFree=Whitening\n");
      break;
    case 0b11:
      printf("\tDcFree=Invalid\n");
      break;
  }
  printf("sx127x_fsk_ook_set_packet_format(SX127X_VARIABLE, 255, device)\n");
  if ((regs[0x30] & 0b10000) != 0) {
    printf("\tCrcOn=1\n");
  } else {
    printf("\tCrcOn=0\n");
  }
  if ((regs[0x30] & 0b1000) != 0) {
    printf("\tCrcAutoClearOff=Do not clear FIFO\n");
  } else {
    printf("\tCrcAutoClearOff=Clear FIFO and restart new packet reception\n");
  }
  switch ((regs[0x30] & 0b110) >> 1) {
    case 0b00:
      printf("\tAddressFiltering=None\n");
      break;
    case 0b01:
      printf("\tAddressFiltering=Address field must match NodeAddress\n");
      break;
    case 0b10:
      printf("\tAddressFiltering=Address field must match NodeAddress or BroadcastAddress\n");
      break;
    case 0b11:
      printf("\tAddressFiltering=Invalid\n");
      break;
  }
  if ((regs[0x30] & 0b1) != 0) {
    printf("\tCrcWhiteningType=IBM CRC\n");
  } else {
    printf("\tCrcWhiteningType=CCITT CRC\n");
  }
  printf("sx127x_fsk_ook_set_crc(SX127X_CRC_NONE, device)\n");
  printf("0x31: RegPacketConfig2\n");
  if ((regs[0x31] & 0b1000000) != 0) {
    printf("\tDataMode=Packet\n");
  } else {
    printf("\tDataMode=Continuous\n");
  }
  if ((regs[0x31] & 0b100000) != 0) {
    printf("\tIoHomeOn=1\n");
  } else {
    printf("\tIoHomeOn=0\n");
  }
  if ((regs[0x31] & 0b1000) != 0) {
    printf("\tBeaconOn=1\n");
  } else {
    printf("\tBeaconOn=0\n");
  }
  printf("\tPayloadLength=%d\n", ((regs[0x31] & 0b111) << 8) | regs[0x32]);
  printf("0x33: RegNodeAdrs\n");
  printf("\tNodeAddress=%d\n", regs[0x33]);
  printf("0x34: RegBroadcastAdrs\n");
  printf("\tBroadcastAddress=%d\n", regs[0x34]);
  printf("0x35: RegFifoThresh\n");
  if ((regs[0x35] & 0b10000000) != 0) {
    printf("\tTxStartCondition=FifoEmpty goes low\n");
  } else {
    printf("\tTxStartCondition=FifoLevel\n");
  }
  printf("\tFifoThreshold=%d\n", (regs[0x35] & 0b111111));
  printf("0x36: RegSeqConfig1\n");
  if ((regs[0x36] & 0b100000) != 0) {
    printf("\tIdleMode=Sleep mode\n");
  } else {
    printf("\tIdleMode=Standby mode\n");
  }
  switch ((regs[0x36] & 0b11000) >> 3) {
    case 0b00:
      printf("\tFromStart=to LowPowerSelection\n");
      break;
    case 0b01:
      printf("\tFromStart=to Receive state\n");
      break;
    case 0b10:
      printf("\tFromStart=to Transmit state\n");
      break;
    case 0b11:
      printf("\tFromStart=to Transmit state on a FifoLevel interrupt\n");
      break;
  }
  if ((regs[0x36] & 0b100) != 0) {
    printf("\tLowPowerSelection=Idle state with chip on Standby or Sleep mode\n");
  } else {
    printf("\tLowPowerSelection=SequencerOff state\n");
  }
  if ((regs[0x36] & 0b10) != 0) {
    printf("\tFromIdle=to Receive state\n");
  } else {
    printf("\tFromIdle=to Transmit state\n");
  }
  if ((regs[0x36] & 0b1) != 0) {
    printf("\tFromTransmit=to Receive state on a PacketSent interrupt\n");
  } else {
    printf("\tFromTransmit=to LowPowerSelection on a PacketSent interrupt\n");
  }
  printf("0x37: RegSeqConfig2\n");
  switch ((regs[0x37] & 0b11100000) >> 5) {
    case 0b000:
    case 0b111:
      printf("\tFromReceive=unused\n");
      break;
    case 0b001:
      printf("\tFromReceive=to PacketReceived state on a PayloadReady interrupt\n");
      break;
    case 0b010:
      printf("\tFromReceive=to LowPowerSelection on a PayloadReady interrupt\n");
      break;
    case 0b011:
      printf("\tFromReceive=to PacketReceived state on a CrcOk interrupt\n");
      break;
    case 0b100:
      printf("\tFromReceive=to SequencerOff state on a Rssi interrupt\n");
      break;
    case 0b101:
      printf("\tFromReceive=to SequencerOff state on a SyncAddress interrupt\n");
      break;
    case 0b110:
      printf("\tFromReceive=to SequencerOff state on a PreambleDetect interrupt\n");
      break;
  }
  switch ((regs[0x37] & 0b11000) >> 3) {
    case 0b00:
      printf("\tFromRxTimeout=to Receive State, via ReceiveRestart\n");
      break;
    case 0b01:
      printf("\tFromRxTimeout=to Transmit state\n");
      break;
    case 0b10:
      printf("\tFromRxTimeout=to LowPowerSelection\n");
      break;
    case 0b11:
      printf("\tFromRxTimeout=to SequencerOff state\n");
      break;
  }
  switch ((regs[0x37] & 0b111)) {
    case 0b000:
      printf("\tFromPacketReceived=to SequencerOff state\n");
      break;
    case 0b001:
      printf("\tFromPacketReceived=to Transmit state on a FifoEmpty interrupt\n");
      break;
    case 0b010:
      printf("\tFromPacketReceived=to LowPowerSelection\n");
      break;
    case 0b011:
      printf("\tFromPacketReceived=to Receive via FS mode, if frequency was changed\n");
      break;
    case 0b100:
      printf("\tFromPacketReceived=to Receive state\n");
      break;
  }
  printf("0x38: RegTimerResol\n");
  switch ((regs[0x38] & 0b1100) >> 2) {
    case 0b00:
      printf("\tTimer1Resolution=disabled\n");
      break;
    case 0b01:
      printf("\tTimer1Resolution=64 us\n");
      break;
    case 0b10:
      printf("\tTimer1Resolution=4.1 ms\n");
      break;
    case 0b11:
      printf("\tTimer1Resolution=262 ms\n");
      break;
  }
  switch ((regs[0x38] & 0b11)) {
    case 0b00:
      printf("\tTimer2Resolution=disabled\n");
      break;
    case 0b01:
      printf("\tTimer2Resolution=64 us\n");
      break;
    case 0b10:
      printf("\tTimer2Resolution=4.1 ms\n");
      break;
    case 0b11:
      printf("\tTimer2Resolution=262 ms\n");
      break;
  }
  printf("0x39: RegTimer1Coef\n");
  printf("\tTimer1Coefficient=%d\n", regs[0x39]);
  printf("0x3a: RegTimer2Coef\n");
  printf("\tTimer2Coefficient=%d\n", regs[0x3a]);
  printf("0x3b: RegImageCal\n");
  if ((regs[0x3b] & 0b10000000) != 0) {
    printf("\tAutoImageCalOn=1\n");
  } else {
    printf("\tAutoImageCalOn=0\n");
  }
  if ((regs[0x3b] & 0b1000) != 0) {
    printf("\tTempChange=Temperature change greater than TempThreshold\n");
  } else {
    printf("\tTempChange=Temperature change lower than TempThreshold\n");
  }
  switch ((regs[0x3b] & 0b110) >> 1) {
    case 0b00:
      printf("\tTempThreshold=5 °C\n");
      break;
    case 0b01:
      printf("\tTempThreshold=10 °C\n");
      break;
    case 0b10:
      printf("\tTempThreshold=15 °C\n");
      break;
    case 0b11:
      printf("\tTempThreshold=20 °C\n");
      break;
  }
  if ((regs[0x3b] & 0b1) != 0) {
    printf("\tTempChange=Temperature monitoring stopped\n");
  } else {
    printf("\tTempChange=Temperature monitoring done in all modes except Sleep and Standby\n");
  }
  printf("0x3c: RegTemp\n");
  printf("\tTempValue=%d\n", regs[0x3c]);
  printf("0x3d: RegLowBat\n");
  if ((regs[0x3d] & 0b1000) != 0) {
    printf("\tLowBatOn=1\n");
  } else {
    printf("\tLowBatOn=0\n");
  }
  switch ((regs[0x3d] & 0b111)) {
    case 0b000:
      printf("\tLowBatTrim=1.695 V\n");
      break;
    case 0b001:
      printf("\tLowBatTrim=1.764 V\n");
      break;
    case 0b010:
      printf("\tLowBatTrim=1.835 V\n");
      break;
    case 0b011:
      printf("\tLowBatTrim=1.905 V\n");
      break;
    case 0b100:
      printf("\tLowBatTrim=1.976 V\n");
      break;
    case 0b101:
      printf("\tLowBatTrim=2.045 V\n");
      break;
    case 0b110:
      printf("\tLowBatTrim=2.116 V\n");
      break;
    case 0b111:
      printf("\tLowBatTrim=2.185 V\n");
      break;
  }
  printf("0x40: RegDioMapping1, check 0x31 RegPacketCnfig2\n");
  if ((regs[0x31] & 0b1000000) != 0) {
    uint8_t dio0 = ((regs[0x40] & 0b11000000) >> 6);
    uint8_t dio1 = ((regs[0x40] & 0b00110000) >> 4);
    uint8_t dio2 = ((regs[0x40] & 0b00001100) >> 2);
    uint8_t dio3 = ((regs[0x40] & 0b00000011));
    uint8_t dio4 = ((regs[0x41] & 0b11000000) >> 6);
    uint8_t dio5 = ((regs[0x41] & 0b00110000) >> 4);
    uint8_t mode = (regs[0x01] & 0b111);
    switch (mode) {
      case 0b000:
        switch (dio1) {
          case 0b00:
            printf("\tDIO1=FifoLevel\n");
            break;
          case 0b01:
            printf("\tDIO1=FifoEmpty\n");
            break;
          case 0b10:
            printf("\tDIO1=FifoFull\n");
            break;
        }
        switch (dio2) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO2=FifoFull\n");
            break;
        }
        switch (dio3) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO3=FifoEmpty\n");
            break;
        }
        if (dio5 == 0b00) {
          printf("\tDIO5=ClkOut if RC\n");
        }
        break;
      case 0b001:
      case 0b010:
      case 0b100:
        if (dio0 == 0b11) {
          printf("\tDIO0=TempChange / LowBat\n");
        }
        switch (dio1) {
          case 0b00:
            printf("\tDIO1=FifoLevel\n");
            break;
          case 0b01:
            printf("\tDIO1=FifoEmpty\n");
            break;
          case 0b10:
            printf("\tDIO1=FifoFull\n");
            break;
        }
        switch (dio2) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO2=FifoFull\n");
            break;
        }
        switch (dio3) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO3=FifoEmpty\n");
            break;
        }
        if (dio4 == 0b00) {
          printf("\tDIO4=TempChange / LowBat\n");
        }
        switch (dio5) {
          case 0b00:
            printf("\tDIO5=ClkOut\n");
            break;
          case 0b11:
            printf("\tDIO5=ModeReady\n");
            break;
        }
        break;
      case 0b011:
        switch (dio0) {
          case 0b00:
            printf("\tDIO0=PacketSent\n");
            break;
          case 0b11:
            printf("\tDIO0=TempChange / LowBat\n");
            break;
        }
        switch (dio1) {
          case 0b00:
            printf("\tDIO1=FifoLevel\n");
            break;
          case 0b01:
            printf("\tDIO1=FifoEmpty\n");
            break;
          case 0b10:
            printf("\tDIO1=FifoFull\n");
            break;
        }
        switch (dio2) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO2=FifoFull\n");
            break;
        }
        switch (dio3) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO3=FifoEmpty\n");
            break;
          case 0b01:
            printf("\tDIO3=TxReady\n");
            break;
        }
        if (dio4 == 0b00) {
          printf("\tDIO4=TempChange / LowBat\n");
        }
        switch (dio5) {
          case 0b00:
            printf("\tDIO5=ClkOut\n");
            break;
          case 0b01:
            printf("\tDIO5=PllLock\n");
            break;
          case 0b10:
            printf("\tDIO5=Data\n");
            break;
          case 0b11:
            printf("\tDIO5=ModeReady\n");
            break;
        }
        break;
      case 0b101:
      case 0b110:
        switch (dio0) {
          case 0b00:
            printf("\tDIO0=PayloadReady\n");
            break;
          case 0b01:
            printf("\tDIO0=CrcOk\n");
            break;
          case 0b11:
            printf("\tDIO0=TempChange / LowBat\n");
            break;
        }
        switch (dio1) {
          case 0b00:
            printf("\tDIO1=FifoLevel\n");
            break;
          case 0b01:
            printf("\tDIO1=FifoEmpty\n");
            break;
          case 0b10:
            printf("\tDIO1=FifoFull\n");
            break;
        }
        switch (dio2) {
          case 0b00:
            printf("\tDIO2=FifoFull\n");
            break;
          case 0b01:
            printf("\tDIO2=RxReady\n");
            break;
          case 0b10:
            printf("\tDIO2=Timeout\n");
            break;
          case 0b11:
            printf("\tDIO2=SyncAddress\n");
            break;
        }
        switch (dio3) {
          case 0b00:
          case 0b10:
          case 0b11:
            printf("\tDIO3=FifoEmpty\n");
            break;
        }
        switch (dio4) {
          case 0b00:
            printf("\tDIO4=TempChange / LowBat\n");
            break;
          case 0b01:
            printf("\tDIO4=PllLock\n");
            break;
          case 0b10:
            printf("\tDIO4=Timeout\n");
            break;
          case 0b11:
            printf("\tDIO4=Rssi/Preamble Detect\n");
            break;
        }
        switch (dio5) {
          case 0b00:
            printf("\tDIO5=ClkOut\n");
            break;
          case 0b01:
            printf("\tDIO5=PllLock\n");
            break;
          case 0b10:
            printf("\tDIO5=Data\n");
            break;
          case 0b11:
            printf("\tDIO5=ModeReady\n");
            break;
        }
        break;
    }
  } else {
    printf("\tDIO mapping in Continuous mode\n");
  }
  printf("0x41: RegDioMapping2\n");
  if ((regs[0x41] & 0b1) != 0) {
    printf("\tMapPreambleDetect=PreambleDetect interrupt\n");
  } else {
    printf("\tMapPreambleDetect=Rssi interrupt\n");
  }
  printf("0x44: RegPllHop\n");
  printf("\tFastHopOn=%d\n", ((regs[0x44] & 0b10000000) >> 7));
  printf("0x4b: RegTcxo\n");
  printf("\tTcxoInputOn=%d\n", ((regs[0x4b] & 0b10000) >> 4));
  printf("0x4d: RegPaDac\n");
  if ((regs[0x4d] & 0b111) == 0x04) {
    printf("\tPaDac=Default\n");
  } else if ((regs[0x4d] & 0b111) == 0x07) {
    printf("\tPaDac=+20dBm on PA_BOOST\n");
  } else {
    printf("\tPaDac=Invalid\n");
  }
  return EXIT_SUCCESS;
}

int dump_lora_registers(uint8_t *regs) {
  uint8_t value = regs[0x01];
  print_op_mode(value);
  uint64_t freq = (((uint64_t) regs[0x06]) << 16) | (((uint64_t) regs[0x07]) << 8) | (regs[0x08]);
  printf("0x06: RegFr:\n");
  printf("\tFrf=%" PRIu64 "\n", ((freq * 32000000) / (1 << 19)));
  printf("0x09: RegPaConfig:\n");
  value = regs[0x07];
  if ((value & 0b10000000) == 0b10000000) {
    printf("\tPaSelect=PA_BOOST pin\n");
  } else {
    printf("\tPaSelect=RFO pin\n");
  }
  printf("\tMaxPower=0x%x\n", ((value & 0b110000) >> 4));
  printf("\tOutputPower=0x%x\n", ((value & 0b1111)));
  printf("0x0a: RegPaRamp:\n");
  printf("\tPaRamp=0x%x\n", regs[0x0a]);
  print_ocp(regs[0x0b]);
  print_lna(regs[0x0c]);
  printf("0x0d: RegFifoAddrPtr:\n");
  printf("\tFifoAddrPtr=%x\n", regs[0x0d]);
  printf("0x0e: RegFifoTxBaseAddr:\n");
  printf("\tFifoTxBaseAddr=%x\n", regs[0x0e]);
  printf("0x0f: RegFifoRxBaseAddr:\n");
  printf("\tFifoRxBaseAddr=%x\n", regs[0x0f]);
  printf("0x10: RegFifoRxCurrentAddr:\n");
  printf("\tFifoRxCurrentAddr=%x\n", regs[0x10]);
  value = regs[0x1d];
  printf("0x1d: RegModemConfig1:\n");
  switch (((value & 0b11110000) >> 4)) {
    case 0b0000:
      printf("\tBw=7.8 kHz\n");
      break;
    case 0b0001:
      printf("\tBw=10.4 kHz\n");
      break;
    case 0b0010:
      printf("\tBw=15.6 kHz\n");
      break;
    case 0b0011:
      printf("\tBw=20.8kHz\n");
      break;
    case 0b0100:
      printf("\tBw=31.25 kHz\n");
      break;
    case 0b0101:
      printf("\tBw=41.7 kHz\n");
      break;
    case 0b0110:
      printf("\tBw=62.5 kHz\n");
      break;
    case 0b0111:
      printf("\tBw=125 kHz\n");
      break;
    case 0b1000:
      printf("\tBw=250 kHz\n");
      break;
    case 0b1001:
      printf("\tBw=500 kHz\n");
      break;
  }
  switch (((value & 0b1110) >> 1)) {
    case 0b001:
      printf("\tCodingRate=4/5\n");
      break;
    case 0b010:
      printf("\tCodingRate=4/6\n");
      break;
    case 0b011:
      printf("\tCodingRate=4/7\n");
      break;
    case 0b100:
      printf("\tCodingRate=4/8\n");
      break;
  }
  printf("\tImplicitHeaderModeOn=%d\n", (value & 0b1));
  printf("0x1e: RegModemConfig2\n");
  value = regs[0x1e];
  printf("\tSpreadingFactor=%d\n", ((value & 0b11110000) >> 4));
  if (((value & 0b1000) >> 3) != 0) {
    printf("\tTxContinuousMode=continuous mode\n");
  } else {
    printf("\tTxContinuousMode=normal mode\n");
  }
  printf("\tRxPayloadCrcOn=%d\n", ((value & 0b100) >> 2));
  printf("0x1f: RegSymbTimeoutLsb:\n");
  printf("\tSymbTimeout=%d\n", ((regs[0x1e] & 0b11) << 8) | regs[0x1f]);
  printf("0x20: RegPreamble:\n");
  printf("\tPreambleLength=%d\n", (regs[0x20] << 8) | regs[0x21]);
  printf("0x22: RegPayloadLength:\n");
  printf("\tPayloadLength=%d\n", regs[0x22]);
  printf("0x23: RegMaxPayloadLength:\n");
  printf("\tPayloadMaxLength=%d\n", regs[0x23]);
  printf("0x24: RegHopPeriod:\n");
  printf("\tFreqHoppingPeriod=%d\n", regs[0x24]);
  printf("0x25: RegFifoRxByteAddr:\n");
  printf("\tFifoRxByteAddrPtr=%d\n", regs[0x25]);
  printf("0x26: RegModemConfig3:\n");
  value = regs[0x26];
  if ((value & 0b1000) != 0) {
    printf("\tLowDataRateOptimize=Enabled\n");
  } else {
    printf("\tLowDataRateOptimize=Disabled\n");
  }
  printf("\tAgcAutoOn=%d\n", ((value & 0b100) >> 2));
  printf("0x27: PpmCorrection:\n");
  printf("\tPpmCorrection=%d\n", regs[0x27]);
  printf("0x31: RegDetectOptimize:\n");
  printf("\tDetectionOptimize=%d\n", (regs[0x31] & 0b111));
  printf("0x37: RegDetectionThreshold:\n");
  printf("\tDetectionThreshold=%d\n", regs[0x37]);
  printf("0x39: RegSyncWord:\n");
  printf("\tSyncWord=%d\n", regs[0x39]);
  printf("0x40: RegDioMapping1\n");
  value = ((regs[0x40] & 0b11000000) >> 6);
  switch (value) {
    case 0b00:
      printf("\tDIO0=RxDone\n");
      break;
    case 0b01:
      printf("\tDIO0=TxDone\n");
      break;
    case 0b10:
      printf("\tDIO0=CadDone\n");
      break;
    case 0b11:
      printf("\tDIO0=Invalid\n");
      break;
  }
  value = ((regs[0x40] & 0b00110000) >> 4);
  switch (value) {
    case 0b00:
      printf("\tDIO1=RxTimeout\n");
      break;
    case 0b01:
      printf("\tDIO1=FhssChangeChannel\n");
      break;
    case 0b10:
      printf("\tDIO1=CadDetected\n");
      break;
    case 0b11:
      printf("\tDIO1=Invalid\n");
      break;
  }
  value = ((regs[0x40] & 0b00001100) >> 2);
  switch (value) {
    case 0b00:
      printf("\tDIO2=FhssChangeChannel\n");
      break;
    case 0b01:
      printf("\tDIO2=FhssChangeChannel\n");
      break;
    case 0b10:
      printf("\tDIO2=FhssChangeChannel\n");
      break;
    case 0b11:
      printf("\tDIO2=Invalid\n");
      break;
  }
  value = ((regs[0x40] & 0b11));
  switch (value) {
    case 0b00:
      printf("\tDIO3=CadDone\n");
      break;
    case 0b01:
      printf("\tDIO3=ValidHeader\n");
      break;
    case 0b10:
      printf("\tDIO3=PayloadCrcError\n");
      break;
    case 0b11:
      printf("\tDIO3=Invalid\n");
      break;
  }
  printf("0x41: RegDioMapping2\n");
  value = ((regs[0x41] & 0b11000000) >> 6);
  switch (value) {
    case 0b00:
      printf("\tDIO4=CadDetected\n");
      break;
    case 0b01:
      printf("\tDIO4=PllLock\n");
      break;
    case 0b10:
      printf("\tDIO4=PllLock\n");
      break;
    case 0b11:
      printf("\tDIO4=Invalid\n");
      break;
  }
  value = ((regs[0x41] & 0b110000) >> 4);
  switch (value) {
    case 0b00:
      printf("\tDIO5=ModeReady\n");
      break;
    case 0b01:
      printf("\tDIO5=ClkOut\n");
      break;
    case 0b10:
      printf("\tDIO5=ClkOut\n");
      break;
    case 0b11:
      printf("\tDIO5=Invalid\n");
      break;
  }
  return 0;
}

int at_util_string2hex(const char *str, uint8_t **output, size_t *output_length) {
  size_t len = 0;
  size_t str_len = strlen(str);
  for (size_t i = 0; i < str_len; i++) {
    if (str[i] == ' ' || str[i] == ':' || str[i] == ',') {
      continue;
    }
    //expect 0x99 so each "x" is for separate number
    if (str[i] == 'x') {
      len++;
    }
  }
  size_t bytes = len;
  auto result = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * bytes));
  if (result == nullptr) {
    return -1;
  }
  uint8_t curByte = 0;
  for (size_t i = 0, j = 0; i < strlen(str);) {
    char curChar = str[i];
    if (curChar == ' ' || str[i] == ':') {
      i++;
      continue;
    }
    if (str[i] == ',') {
      result[j] = curByte;
      curByte = 0;
      i++;
      j++;
      continue;
    }
    if (str[i] == '0' && str[i + 1] == 'x') {
      i += 2;
      continue;
    }
    curByte *= 16;
    if (curChar >= '0' && curChar <= '9') {
      curByte += curChar - '0';
    } else if (curChar >= 'A' && curChar <= 'F') {
      curByte += (curChar - 'A') + 10;
    } else if (curChar >= 'a' && curChar <= 'f') {
      curByte += (curChar - 'a') + 10;
    } else {
      return -1;
    }
    i++;
  }
  *output = result;
  *output_length = bytes;
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "missing argument\n");
    return EXIT_FAILURE;
  }
  uint8_t *output = NULL;
  size_t output_length = 0;
  int code = at_util_string2hex(argv[1], &output, &output_length);
  if (code != 0) {
    return EXIT_FAILURE;
  }
  if ((output[0x01] & 0b10000000) == 0b10000000) {
    return dump_lora_registers(output);
  }
  if ((output[0x01] & 0b00000000) == 0b00000000) {
    return dump_fsk_registers(output);
  }
  if ((output[0x01] & 0b00100000) == 0b00100000) {
    printf("ook is not supported yet");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
}