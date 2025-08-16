> > > > > > ### Registered commands
2W SAUTER/ATLANTIC/THERMOR
- **powerOn**     _Permit to retrieve paired devices_
- **setTemp**     _7.0 to 28.0 - 0 get actual temp_
- **setMode**     _auto prog manual off - FF to get actual mode_
- **setPresence** _on off_
- **setWindow**   _open close_
- **midnight**    _Synchro Paired_

VARIOUS 2W
- **associate**   _Synchro Paired_
- **custom**      _test unknown commands_
- **custom60**    _test 0x60 commands_
- **discovery**   _Send discovery on air_
- **getName**     _Name Of A Device_
- **scanMode**    _scanMode_
- **scanDump**    _Dump Scan Results_
- **pairMode**    _pairMode_

VARIOUS 1W (Use the description name in 1W.json as argument)
- **pair**      _1W put device in pair mode_
- **add**       _1W add controller to device_
- **remove**    _1W remove controller from device_

- **pair** and **remove** update the `paired` status in `1W.json`. The field is automatically created with value `false` if it is missing when the file is loaded.

- **open**      _1W open device_
- **close**     _1W close device_
- **stop**      _1W stop device_
- **position**  _1W set position (0-100)_
- **absolute**  _1W set absolute position (0-100)_
- **vent**      _1W vent device_
- **force**     _1W force device open_
- **mode1**     _1W Mode1_
- **mode2**     _1W Mode2_
- **mode3**     _1W Mode3_
- **mode4**     _1W Mode4_
- **new1W**    _Add new 1W device_
- **del1W**    _Remove 1W device_
- **edit1W**   _Edit 1W device name_
- **time1W**   _Set 1W device travel time in seconds_
- **list1W**   _List 1W devices_

COMMON
- **newRemote** _Create remote map entry_
- **linkRemote** _Link device to remote map entry_
- **unlinkRemote** _Remove device from remote map entry_
- **verbose**   _Toggle verbose output on packets list_
- **help**      _This command_
- **ls**        _List filesystem_
- **cat**       _Print file content_
- **rm**        _Remove file_
- **lastAddr**  _Show last received address_
- **mqttIp**    _Set MQTT server IP_
- **mqttUser**  _Set MQTT username_
- **mqttPass**  _Set MQTT password_
- **mqttDiscovery** _Set MQTT discovery topic_
