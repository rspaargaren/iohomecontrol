Based on the wonderfull work of [Velocet/iown-homecontrol](https://github.com/Velocet/iown-homecontrol)

![smlogo](https://github.com/user-attachments/assets/f6b606a1-0eca-4fd6-a509-d5d1136b2d31)

### **Disclaimer**  
Tool designed for educational and testing purposes, provided "as is", without warranty of any kind. Creators and contributors are not responsible for any misuse or damage caused by this tool.

_I don't give any support, especially concerning the obsolete 1W part._

This code doesnt use the RadioLib, even perfect for start, it doesnt allow to be fast as possible


### Documentation
This code was intentionally written with all possible details found in the protocol documentation.
- (ie: there's 3 differents CRC, 3 differents AES implementation, all detailled)

- This allows those who have little knowledge of C/CPP to understand the sequence of each command.
All these details require you to carefully read the work of @Velocet to understand this protocol.
- There is no magic documentation, there is only personal work to adapt to your own needs. 
- But all the functional basis for the 1W and 2W is there.
- All AES are there, without using an external library.
- All 1W / 2W commands pairing/associating answers are there too.

### Usage
Use it like a scanner, do some commands on your real device to identify the corresponding CMDid and address

-> Choose you board before build.

### Brief explanation[^1]:
Use 2 interrupts 
  - if PAYLOAD and RX -> decode the frame [^3]
  - if PAYLOAD and TX_READY -> send the frame, decode it [^3]

### platformio[^2] :
_At first time or when a JSON in data folder is modified:_
  1. build filesystem image
  2. upload filesystem image
     
_After any other changes:_
  - upload and monitor
  - make sure `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD` remains enabled in
    `sdkconfig` so ESP timers can run callbacks from ISR context

[^1]: I use an SX1276. If CC1101/SX1262: Feel free to use the old code (Not checked/garanted)

[^2]: I use Visual Studio Code Insider

[^3]: Decoding can be verbose (RSSI, Timing, ...)

## Web Interface (Experimental)

This project now includes an experimental web interface to control IOHC devices.

### Setup & Access

1.  **Configure WiFi:**
    *   On first boot the device starts an access point named `iohc-setup`.
    *   Connect to this AP and follow the WiFiManager captive portal to enter
        your WiFi credentials.

2.  **Build and Upload Filesystem:**
    *   The web interface files (`index.html`, `style.css`, `script.js`) are located in `extras/web_interface_data/`.
    *   These files need to be uploaded to the ESP32's LittleFS filesystem.
    *   Using PlatformIO:
        *   First, build the filesystem image: `pio run --target buildfs` (or use the PlatformIO IDE option for building the filesystem image).
        *   Then, upload the filesystem image: `pio run --target uploadfs` (or use the PlatformIO IDE option for uploading).
    *   **Note:** You only need to rebuild and re-upload the filesystem image if you make changes to the files in `extras/web_interface_data/`.
    *   **Device files:** Copy your device definition files (for example `extras/1W.json`) into the LittleFS root before building.
        Without these files the `/api/devices` endpoint returns an empty list and the web interface will show no devices.

3.  **Build and Upload Firmware:**
    *   Build and upload the main firmware to your ESP32 as usual using PlatformIO (`pio run --target upload` or via the IDE).

4.  **Find ESP32 IP Address:**
    *   After uploading, open the Serial Monitor.
    *   When the ESP32 connects to your WiFi network, it will print its IP address. Look for a line like: `Connected to WiFi. IP Address: XXX.XXX.X.XXX`.

5.  **Access the Interface:**
    *   Open a web browser on a device connected to the same WiFi network as your ESP32.
    *   Navigate to the IP address you found in the Serial Monitor (e.g., `http://XXX.XXX.X.XXX`) or use `http://miopenio.local` if your network supports mDNS.

### Usage

The web interface allows you to:

*   **View a list of devices:** The device list is currently populated with placeholder examples. (Future development will integrate this with actual detected/configured devices).
*   **Send commands:** Select a device, type a command string (e.g., `setTemp 21.0`), and click "Send". (Command processing is currently a placeholder and will acknowledge receipt).

This feature is under development, and functionality will be expanded in the future.

## Home Assistant Discovery

When MQTT is enabled (`#define MQTT` in `include/user_config.h`) the firmware publishes Home Assistant discovery messages for every blind defined in `extras/1W.json` as soon as the MQTT connection is established. Comment out the `MQTT` definition if you do not wish to compile the firmware with MQTT support.

Each blind configuration is sent to the topic `homeassistant/cover/<id>/config` where `<id>` is the hexadecimal address from `1W.json`.

The discovery payload's `device` section now also exposes the device's AES `key` as read from `1W.json`.

The `1W.json` file now accepts an optional `travel_time` field per device. This value represents the time in milliseconds a blind takes to move from fully closed to fully open. It allows the firmware to estimate the current position when no feedback is available. The estimated position is printed to the serial console and shown on the OLED display every second while the blind is moving. When a command is transmitted or received, this position feedback is appended below the action information on the display so that the original message remains visible.
If these fields (`name` and `travel_time`) are missing, default values are applied using the device description and a 10 second travel time. These defaults are saved back to `1W.json` so subsequent boots load the updated values automatically.

Each blind also publishes a Home Assistant number entity for the travel time. Adjusting this entity updates the `travel_time` value in `1W.json`, allowing calibration directly from the Home Assistant UI without editing files manually.

Each entry can also contain a `paired` boolean that indicates if the blind is paired to a screen. If the field is missing, it is automatically added with a default value of `false` when the file is loaded. The flag is updated automatically when the `pair` or `remove` commands are used.

Sequence numbers for each remote are stored both in `extras/1W.json` and in NVS.
On boot the value from the file is compared to the one in NVS and the highest
value is kept so sequence numbers continue uninterrupted even after filesystem
uploads or resets.

Example payload for `B60D1A`:

```json
{"name":"IZY1","command_topic":"iown/B60D1A/set","state_topic":"iown/B60D1A/state","position_topic":"iown/B60D1A/position","set_position_topic":"iown/B60D1A/position/set","unique_id":"B60D1A","payload_open":"OPEN","payload_close":"CLOSE","payload_stop":"STOP","device_class":"blind","availability_topic":"iown/status"}
```

For each blind the firmware also publishes MQTT button entities that allow
executing pairing or controller management commands directly from Home Assistant.
The discovery topics are:

- `homeassistant/button/<id>_pair/config`
- `homeassistant/button/<id>_add/config`
- `homeassistant/button/<id>_remove/config`

Each blind along with its control buttons is exposed as an individual device in
Home Assistant, so the gateway no longer groups all entities into a single
device list.

Sending `PRESS` to `iown/<id>/pair`, `iown/<id>/add` or `iown/<id>/remove`
triggers the corresponding command on the blind.

Configure your MQTT broker settings in `include/user_config.h` (`mqtt_server`, `mqtt_user`, `mqtt_password`, `mqtt_discovery_topic`). These values can also be changed at runtime via the `mqttIp`, `mqttUser`, `mqttPass` and `mqttDiscovery` commands. After boot and connection, Home Assistant should automatically discover the covers.

If you don't have an OLED display connected, comment out the `DISPLAY` definition in `include/user_config.h` to disable all display related code.

Once discovery is complete you can control a blind by publishing `OPEN`, `CLOSE`
or `STOP` to `iown/<id>/set`, or a number between `0` and `100` to
`iown/<id>/position/set` to move the blind to a specific position. The firmware
listens on these topics and issues the corresponding command to the device.
When an `OPEN` or `CLOSE` command is received, it immediately publishes the new
state (`open` or `closed`) to `iown/<id>/state` so Home Assistant can update the
cover status.

While a blind is in motion the current position percentage is published every
second to `iown/<id>/position`. The `state` topic is also updated with
`OPENING`, `CLOSING` or `STOP` depending on the movement. When the blind stops
moving, the state reverts to `OPEN`, `CLOSE` or `STOP` according to the final
position.

The gateway publishes `online` every minute to `iown/status` and has a Last Will
configured to send `offline` on the same topic if it disconnects unexpectedly.
Home Assistant uses this message to mark all covers as unavailable when the
gateway goes offline.

#### **License**
