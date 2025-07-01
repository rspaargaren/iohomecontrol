Based on the wonderfull work of [Velocet/iown-homecontrol](https://github.com/Velocet/iown-homecontrol)


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

[^1]: I use an SX1276. If CC1101/SX1262: Feel free to use the old code (Not checked/garanted)

[^2]: I use Visual Studio Code Insider

[^3]: Decoding can be verbose (RSSI, Timing, ...)

## Web Interface (Experimental)

This project now includes an experimental web interface to control IOHC devices.

### Setup & Access

1.  **Configure WiFi:**
    *   Open `src/main.cpp`.
    *   Locate the `YOUR_SSID` and `YOUR_PASSWORD` placeholders.
    *   Replace them with your actual WiFi network SSID and password.

2.  **Build and Upload Filesystem:**
    *   The web interface files (`index.html`, `style.css`, `script.js`) are located in `extras/web_interface_data/`.
    *   These files need to be uploaded to the ESP32's LittleFS filesystem.
    *   Using PlatformIO:
        *   First, build the filesystem image: `pio run --target buildfs` (or use the PlatformIO IDE option for building the filesystem image).
        *   Then, upload the filesystem image: `pio run --target uploadfs` (or use the PlatformIO IDE option for uploading).
    *   **Note:** You only need to rebuild and re-upload the filesystem image if you make changes to the files in `extras/web_interface_data/`.

3.  **Build and Upload Firmware:**
    *   Build and upload the main firmware to your ESP32 as usual using PlatformIO (`pio run --target upload` or via the IDE).

4.  **Find ESP32 IP Address:**
    *   After uploading, open the Serial Monitor.
    *   When the ESP32 connects to your WiFi network, it will print its IP address. Look for a line like: `Connected to WiFi. IP Address: XXX.XXX.X.XXX`.

5.  **Access the Interface:**
    *   Open a web browser on a device connected to the same WiFi network as your ESP32.
    *   Navigate to the IP address you found in the Serial Monitor (e.g., `http://XXX.XXX.X.XXX`).

### Usage

The web interface allows you to:

*   **View a list of devices:** The device list is currently populated with placeholder examples. (Future development will integrate this with actual detected/configured devices).
*   **Send commands:** Select a device, type a command string (e.g., `setTemp 21.0`), and click "Send". (Command processing is currently a placeholder and will acknowledge receipt).

This feature is under development, and functionality will be expanded in the future.

## Home Assistant Discovery

When MQTT is enabled (`#define MQTT` in `include/user_config.h`) the firmware publishes Home Assistant discovery messages for every blind defined in `extras/1W.json` as soon as the MQTT connection is established.

Each blind configuration is sent to the topic `homeassistant/cover/<id>/config` where `<id>` is the hexadecimal address from `1W.json`.

Example payload for `B60D1A`:

```json
{"name":"IZY1","command_topic":"iown/B60D1A/set","state_topic":"iown/B60D1A/state","unique_id":"B60D1A","payload_open":"OPEN","payload_close":"CLOSE","payload_stop":"STOP","device_class":"blind"}
```

Configure your MQTT broker settings in `include/user_config.h` (`MQTT_SERVER`, `MQTT_USER`, `MQTT_PASSWD`). After boot and connection, Home Assistant should automatically discover the covers.

Once discovery is complete you can control a blind by publishing `OPEN`, `CLOSE`
or `STOP` to `iown/<id>/set`. The firmware listens on these topics and issues the
corresponding command to the device.

#### **License**
