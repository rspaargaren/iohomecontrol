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
#include <fileSystemHelpers.h>
#include <interact.h>
#include <iohcCozyDevice2W.h>
// #include <iohcCryptoHelpers.h>
#include <iohcUtils2W.h>
#include <iohcRemote1W.h>
// #include <oled_display.h>
// #include <wifi_helper.h>
#if defined(MQTT)
#include <mqtt_handler.h>
#endif

ConnState mqttStatus = ConnState::Disconnected;

_cmdEntry* _cmdHandler[MAXCMDS];
uint8_t lastEntry = 0;

void tokenize(std::string const &str, const char delim, Tokens &out) {
  std::stringstream ss(str);
  std::string s;
  while (std::getline(ss, s, delim)) {
    out.push_back(s);
  }
}

namespace Cmd {
    bool verbosity = true;
    bool pairMode = false;
    bool scanMode = false;
#if defined(ESP32)
        TimersUS::TickerUsESP32 kbd_tick;
#endif
    TimerHandle_t consoleTimer;

    static char _rxbuffer[512];
    static uint8_t _len = 0;
    static uint8_t _avail = 0;

    /**
     * The function `createCommands()` initializes and adds various command handlers for controlling
     * different devices and functionalities.
     */
    void createCommands() {
        // Atlantic 2W
        Cmd::addHandler("powerOn", "Permit to retrieve paired devices", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::powerOn, nullptr);
        });
        Cmd::addHandler("setTemp", "7.0 to 28.0 - 0 get actual temp", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setTemp, cmd /*cmd->at(1).c_str()*/);
        });
        Cmd::addHandler("setMode", "auto prog manual off - FF to get actual mode", [](Tokens *cmd)-> void {
                            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setMode, cmd /*cmd->at(1).c_str()*/);
        });
        Cmd::addHandler("setPresence", "on off", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setPresence, cmd /*cmd->at(1).c_str()*/);
        });
        Cmd::addHandler("setWindow", "open close", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setWindow, cmd /*cmd->at(1).c_str()*/);
        });
        Cmd::addHandler("midnight", "Synchro Paired", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::midnight, nullptr);
        });
        Cmd::addHandler("associate", "Synchro Paired", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::associate, nullptr);
        });
        Cmd::addHandler("custom", "test unknown commands", [](Tokens *cmd)-> void {
            /*scanMode = true;*/
            IOHC::iohcUtils2W::getInstance()->cmd(IOHC::Other2WButton::custom, cmd /*cmd->at(1).c_str()*/);
        });
        Cmd::addHandler("custom60", "test 0x60 commands", [](Tokens *cmd)-> void {
            /*scanMode = true;*/
            IOHC::iohcUtils2W::getInstance()->cmd(IOHC::Other2WButton::custom60, cmd /*cmd->at(1).c_str()*/);
        });
        // 1W
        Cmd::addHandler("pair", "1W put device in pair mode", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Pair, cmd);
        });
        Cmd::addHandler("add", "1W add controller to device", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Add, cmd);
        });
        Cmd::addHandler("remove", "1W remove controller from device", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Remove, cmd);
        });
        Cmd::addHandler("open", "1W open device", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Open, cmd);
        });
        Cmd::addHandler("close", "1W close device", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Close, cmd);
        });
        Cmd::addHandler("stop", "1W stop device", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Stop, cmd);
        });
        Cmd::addHandler("vent", "1W vent device", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Vent, cmd);
        });
        Cmd::addHandler("force", "1W force device open", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::ForceOpen, cmd);
        });
        Cmd::addHandler("mode1", "1W Mode1", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode1, cmd);
        });
        Cmd::addHandler("mode2", "1W Mode2", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode2, cmd);
        });
        Cmd::addHandler("mode3", "1W Mode3", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode3, cmd);
        });
        Cmd::addHandler("mode4", "1W Mode4", [](Tokens *cmd)-> void {
            IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode4, cmd);
        });
        // Other 2W
        Cmd::addHandler("discovery", "Send discovery on air", [](Tokens *cmd)-> void {
            IOHC::iohcUtils2W::getInstance()->cmd(IOHC::Other2WButton::discovery, nullptr);
        });
        Cmd::addHandler("getName", "Name Of A Device", [](Tokens *cmd)-> void {
            IOHC::iohcUtils2W::getInstance()->cmd(IOHC::Other2WButton::getName, cmd);
        });
        Cmd::addHandler("scanMode", "scanMode", [](Tokens *cmd)-> void {
            scanMode = true;
            IOHC::iohcUtils2W::getInstance()->cmd(IOHC::Other2WButton::checkCmd, nullptr);
        });
        Cmd::addHandler("scanDump", "Dump Scan Results", [](Tokens *cmd)-> void {
            scanMode = false;
            IOHC::iohcUtils2W::getInstance()->scanDump();
        });
        Cmd::addHandler("verbose", "Toggle verbose output on packets list", [](Tokens *cmd)-> void { verbosity = !verbosity;
        });
        Cmd::addHandler("pairMode", "pairMode", [](Tokens *cmd)-> void { pairMode = !pairMode; });

        // Utils
        Cmd::addHandler("dump", "Dump Transceiver registers", [](Tokens *cmd)-> void {
            Radio::dump();
    //        Serial.printf("*%d packets in memory\t", nextPacket);
    //        Serial.printf("*%d devices discovered\n\n", sysTable->size());
        });
        /*
        //    Cmd::addHandler((char *)"dump2", (char *)"Dump Transceiver registers 1Col", [](Tokens*cmd)->void {Radio::dump2(); Serial.printf("*%d packets in memory\t", nextPacket); Serial.printf("*%d devices discovered\n\n", sysTable->size());});
        Cmd::addHandler((char *) "list1W", (char *) "List received packets", [](Tokens *cmd)-> void {
            for (uint8_t i = 0; i < nextPacket; i++) msgRcvd(radioPackets[i]);
            sysTable->dump1W();
        });
        Cmd::addHandler((char *) "save", (char *) "Saves Objects table", [](Tokens *cmd)-> void {
            sysTable->save(true); });
        Cmd::addHandler((char *) "erase", (char *) "Erase received packets", [](Tokens *cmd)-> void {
            for (uint8_t i = 0; i < nextPacket; i++) free(radioPackets[i]);
            nextPacket = 0;
        });
        Cmd::addHandler((char *) "send", (char *) "Send packet from cmd line",
                        [](Tokens *cmd)-> void { txUserBuffer(cmd); });
    */
        Cmd::addHandler("ls", "List filesystem", [](Tokens *cmd)-> void { listFS(); });
        Cmd::addHandler("cat", "Print file content", [](Tokens *cmd)-> void { cat(cmd->at(1).c_str()); });
        Cmd::addHandler("rm", "Remove file", [](Tokens *cmd)-> void { rm(cmd->at(1).c_str()); });
    /*
        Cmd::addHandler((char *) "list2W", (char *) "List received packets", [](Tokens *cmd)-> void {
            for (uint8_t i = 0; i < nextPacket; i++) msgRcvd(radioPackets[i]);
            sysTable->dump2W();
        });
        // Unnecessary just for test
        Cmd::addHandler((char *) "discover28", (char *) "discover28", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::discover28, nullptr);
        });
    */
        Cmd::addHandler("discover2A", "discover2A", [](Tokens *cmd)-> void {
            IOHC::iohcUtils2W::getInstance()->cmd(IOHC::Other2WButton::discover2A, nullptr);
        });
    /*
        Cmd::addHandler((char *) "fake0", (char *) "fake0", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::fake0, nullptr);
        });
        Cmd::addHandler((char *) "ack", (char *) "ack33", [](Tokens *cmd)-> void {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::ack, nullptr);
        });
    */
    }

    bool addHandler(const char *cmd, const char *description, void (*handler)(Tokens*)) {
      for (uint8_t idx = 0; idx < MAXCMDS; ++idx) {
        if (_cmdHandler[idx] != nullptr) {
            //
        } else {
          void *alloc = malloc(sizeof(struct _cmdEntry));
          if (!alloc) return false;

          _cmdHandler[idx] = static_cast<_cmdEntry *>(alloc);
          memset(alloc, 0, sizeof(_cmdEntry));
          strncpy(_cmdHandler[idx]->cmd, cmd, strlen(cmd) < sizeof(_cmdHandler[idx]->cmd)
              ? strlen(cmd)
              : sizeof(_cmdHandler[idx]->cmd) - 1);
          strncpy(_cmdHandler[idx]->description, description, strlen(cmd) < sizeof(_cmdHandler[idx]->description)
              ? strlen(description)
              : sizeof(_cmdHandler[idx]->description) - 1);
          _cmdHandler[idx]->handler = handler;

          if (idx > lastEntry) lastEntry = idx;
          return true;
        }
      }
      return false;
    }

    const char *cmdReceived(bool echo) {
      _avail = Serial.available();
      if (_avail) {
        _len += Serial.readBytes(&_rxbuffer[_len], _avail);
        if (echo) {
          _rxbuffer[_len] = '\0';
          Serial.printf("%s", &_rxbuffer[_len - _avail]);
        }
      }
      if (_rxbuffer[_len - 1] == 0x0a) {
        _rxbuffer[_len - 2] = '\0';
        _len = 0;
        return _rxbuffer;
      }
      return nullptr;
    }

    void cmdFuncHandler() {
      constexpr char delim = ' ';
      Tokens segments;

      const auto cmd = cmdReceived(true);
      if (!cmd) return;
      if (!strlen(cmd)) return;

      tokenize(cmd, delim, segments);
      if (strcmp("help", segments[0].c_str()) == 0) {
        ets_printf("\nRegistered commands:\n");
        for (uint8_t idx = 0; idx <= lastEntry; ++idx) {
          if (_cmdHandler[idx] == nullptr)
            continue;
          ets_printf("- %s\t%s\n", _cmdHandler[idx]->cmd, _cmdHandler[idx]->description);
        }
        ets_printf("- %s\t%s\n\n", "help", "This command");
        ets_printf("\n");
        return;
      }
      for (uint8_t idx = 0; idx <= lastEntry; ++idx) {
        if (_cmdHandler[idx] == nullptr) continue;
        if (strcmp(_cmdHandler[idx]->cmd, segments[0].c_str()) == 0) {
          _cmdHandler[idx]->handler(&segments);
          return;
        }
      }
      ets_printf("*> Unknown <*\n");
    }

    void init() {
      kbd_tick.attach_ms(500, cmdFuncHandler);
    }
}
