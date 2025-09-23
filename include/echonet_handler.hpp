/**
  Copyright (c) 2025. CRIDP https://github.com/cridp

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

          http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Created by Pascal on 15/09/2025.
*/

#ifndef _ECHONET_HANDLER_HPP
#define _ECHONET_HANDLER_HPP

#include <uecho/cpp/uecho.hpp>
#include <uecho/standard/database.hpp>

// See : APPENDIX Detailed Requirements for ECHONET Device objects
//       3.3.29 Requirements for mono functional lighting class
#define LIGHT_OBJECT_CODE 0x029101
#define LIGHT_PROPERTY_POWER_CODE 0x80
#define LIGHT_PROPERTY_POWER_ON 0x30
#define LIGHT_PROPERTY_POWER_OFF 0x31

using Operation = enum {
    OperationClose = 0x42,
    OperationOpen = 0x41,
    OperationStop = 0x43,
  };

using State = enum {
    StateFullyOpen = 0x41,
    StateFullyClosed = 0x42,
    StateOpening = 0x43,
    StateClosing = 0x44,
    StateStoppedHalfway = 0x45,
  };
using EchonetLocation = enum {
    LocationLiving = 0x08,
    LocationDinig = 0x10,
    LocationKitchen = 0x18,
    LocationBathroom = 0x20,
    LocationLavatory = 0x28,
    LocationWashroom = 0x30,
    LocationPassageway = 0x38,
    LocationRoom = 0x40,
    LocationStairway = 0x48,
    LocationFrontDoor = 0x50,
    LocationStoreroom = 0x58,
    LocationGarden = 0x60,
    LocationGarage = 0x68,
    LocationVeranda = 0x70,
    LocationOthers = 0x78,
    LocationIndefinite = 0xFF,
  };

#define UNKNOWN_STRING "?"
#define SEARCH_WAIT_MTIME 200

void setup_uEcho();

#endif //_ECHONET_HANDLER_HPP