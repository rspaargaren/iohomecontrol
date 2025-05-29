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
#include <iohcRemote1W.h>
#include <iohcCozyDevice2W.h>
#include <iohcOtherDevice2W.h>
#include <interact.h>

namespace Cmd {
/**
 * The function `createCommands()` initializes and adds various command handlers for controlling
 * different devices and functionalities.
 */
void createCommands() {
    Cmd::init(); // Initialize Serial commands reception and handlers
    // Atlantic 2W
    Cmd::addHandler((char *) "powerOn", (char *) "Permit to retrieve paired devices", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::powerOn, nullptr);
    });
    Cmd::addHandler((char *) "setTemp", (char *) "7.0 to 28.0 - 0 get actual temp", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setTemp, cmd /*cmd->at(1).c_str()*/);
    });
    Cmd::addHandler((char *) "setMode", (char *) "auto prog manual off - FF to get actual mode",
                    [](Tokens *cmd)-> void {
                        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setMode, cmd /*cmd->at(1).c_str()*/);
                    });
    Cmd::addHandler((char *) "setPresence", (char *) "on off", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setPresence, cmd /*cmd->at(1).c_str()*/);
    });
    Cmd::addHandler((char *) "setWindow", (char *) "open close", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setWindow, cmd /*cmd->at(1).c_str()*/);
    });
    Cmd::addHandler((char *) "midnight", (char *) "Synchro Paired", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::midnight, nullptr);
    });
    Cmd::addHandler((char *) "associate", (char *) "Synchro Paired", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::associate, nullptr);
    });
    Cmd::addHandler((char *) "custom", (char *) "test unknown commands", [](Tokens *cmd)-> void {
        /*scanMode = true;*/
        IOHC::iohcOtherDevice2W::getInstance()->cmd(IOHC::Other2WButton::custom, cmd /*cmd->at(1).c_str()*/);
    });
    Cmd::addHandler((char *) "custom60", (char *) "test 0x60 commands", [](Tokens *cmd)-> void {
        /*scanMode = true;*/
        IOHC::iohcOtherDevice2W::getInstance()->cmd(IOHC::Other2WButton::custom60, cmd /*cmd->at(1).c_str()*/);
    });
    // 1W
    Cmd::addHandler((char *) "pair", (char *) "1W put device in pair mode", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Pair, cmd);
    });
    Cmd::addHandler((char *) "add", (char *) "1W add controller to device", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Add, cmd);
    });
    Cmd::addHandler((char *) "remove", (char *) "1W remove controller from device", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Remove, cmd);
    });
    Cmd::addHandler((char *) "open", (char *) "1W open device", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Open, cmd);
    });
    Cmd::addHandler((char *) "close", (char *) "1W close device", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Close, cmd);
    });
    Cmd::addHandler((char *) "stop", (char *) "1W stop device", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Stop, cmd);
    });
    Cmd::addHandler((char *) "vent", (char *) "1W vent device", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Vent, cmd);
    });
    Cmd::addHandler((char *) "force", (char *) "1W force device open", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::ForceOpen, cmd);
    });
    Cmd::addHandler((char *) "mode1", (char *) "1W Mode1", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode1, cmd);
    });
    Cmd::addHandler((char *) "mode2", (char *) "1W Mode2", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode2, cmd);
    });
    Cmd::addHandler((char *) "mode3", (char *) "1W Mode3", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode3, cmd);
    });
    Cmd::addHandler((char *) "mode4", (char *) "1W Mode4", [](Tokens *cmd)-> void {
        IOHC::iohcRemote1W::getInstance()->cmd(IOHC::RemoteButton::Mode4, cmd);
    });
    // Other 2W
    Cmd::addHandler((char *) "discovery", (char *) "Send discovery on air", [](Tokens *cmd)-> void {
        IOHC::iohcOtherDevice2W::getInstance()->cmd(IOHC::Other2WButton::discovery, nullptr);
    });
    Cmd::addHandler((char *) "getName", (char *) "Name Of A Device", [](Tokens *cmd)-> void {
        IOHC::iohcOtherDevice2W::getInstance()->cmd(IOHC::Other2WButton::getName, cmd);
    });
    Cmd::addHandler((char *) "scanMode", (char *) "scanMode", [](Tokens *cmd)-> void {
        scanMode = true;
        IOHC::iohcOtherDevice2W::getInstance()->cmd(IOHC::Other2WButton::checkCmd, nullptr);
    });
    Cmd::addHandler((char *) "scanDump", (char *) "Dump Scan Results", [](Tokens *cmd)-> void {
        scanMode = false;
        IOHC::iohcOtherDevice2W::getInstance()->scanDump();
    });
    Cmd::addHandler((char *) "verbose", (char *) "Toggle verbose output on packets list",
                    [](Tokens *cmd)-> void { verbosity = !verbosity; });

    Cmd::addHandler((char *) "pairMode", (char *) "pairMode", [](Tokens *cmd)-> void { pairMode = !pairMode; });

    // Utils
    Cmd::addHandler((char *) "dump", (char *) "Dump Transceiver registers", [](Tokens *cmd)-> void {
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
    Cmd::addHandler((char *) "ls", (char *) "List filesystem", [](Tokens *cmd)-> void { listFS(); });
    Cmd::addHandler((char *) "cat", (char *) "Print file content", [](Tokens *cmd)-> void { cat(cmd->at(1).c_str()); });
    Cmd::addHandler((char *) "rm", (char *) "Remove file", [](Tokens *cmd)-> void { rm(cmd->at(1).c_str()); });
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
    Cmd::addHandler((char *) "discover2A", (char *) "discover2A", [](Tokens *cmd)-> void {
        IOHC::iohcOtherDevice2W::getInstance()->cmd(IOHC::Other2WButton::discover2A, nullptr);
    });
/*
    Cmd::addHandler((char *) "fake0", (char *) "fake0", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::fake0, nullptr);
    });
    Cmd::addHandler((char *) "ack", (char *) "ack33", [](Tokens *cmd)-> void {
        IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::ack, nullptr);
    });
*/
    /*
        options.add_options()
          ("d,debug", "Enable debugging") // a bool parameter
          ("i,integer", "Int param", cxxopts::value<int>())
          ("f,file", "File name", cxxopts::value<std::string>())
          ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        options.add_options()
                ("b,bar", "Param bar", cxxopts::value<std::string>())
                ("d,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false"))
                ("f,foo", "Param foo", cxxopts::value<int>()->default_value("10"))
                ("h,help", "Print usage")
    */
    //Customize the options of the console object. See https://github.com/jarro2783/cxxopts for explaination
    /*
        OptionsConsoleCommand powerOn("powerOn", [](int argc, char **argv, ParseResult result, Options options)-> int {
            IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::powerOn, nullptr);
            return EXIT_SUCCESS;
        }, "Permit to retrieve paired devices", "v.0.0.1", "");
        //powerOn.options.add_options()("i,integer", "Int param", cxxopts::value<int>());
        //Register it like any other command
        console.registerCommand(powerOn);
        OptionsConsoleCommand setTemp("t", [](int argc, char **argv, ParseResult result, Options options)-> int {
            // auto tempTok = new Tokens();
            // auto temp = static_cast<String>(result["t"].as<float>());
            // tempTok->push_back(temp.c_str());
        printf(result["t"].as<String>().c_str());
        //    IOHC::iohcCozyDevice2W::getInstance()->cmd(IOHC::DeviceButton::setTemp, result["t"].as<Tokens>() );
            return EXIT_SUCCESS;
        }, ".0 to 28.0 - 0 get actual temp", "v.0.0.1", "");
        //Customize the options of the console object. See https://github.com/jarro2783/cxxopts for explaination
        setTemp.options.add_options()("t", "Temperature", cxxopts::value< std::vector<std::string> >());
        //Register it like any other command
        console.registerCommand(setTemp);
    */
}
}
