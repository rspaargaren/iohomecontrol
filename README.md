<img src="https://github.com/user-attachments/assets/f6b606a1-0eca-4fd6-a509-d5d1136b2d31" alt="smlogo" width="50"/>

# IO-Homecontrol ESP32 Project

Based on the wonderful work of:
[Velocet/iown-homecontrol](https://github.com/Velocet/iown-homecontrol)  
[cridp/iown-homecontrol-esp32sx1276](https://github.com/cridp/iown-homecontrol-esp32sx1276)

[☕ Support the project on BuyMeACoffee](https://buymeacoffee.com/dyna_mite)

---

### 📖 Documentation & Wiki
[- Dutch & English help ](https://github.com/rspaargaren/iohomecontrol/wiki)

---

### **Disclaimer**  
Tool designed for educational and testing purposes, provided "as is", without warranty of any kind. Creators and contributors are not responsible for any misuse or damage caused by this tool.

_I don't give any support, especially concerning the obsolete 1W part._

This code doesn’t use the RadioLib. Even if it’s perfect for a start, it doesn’t allow to be as fast as possible.

---

### Documentation
This code was intentionally written with all possible details found in the protocol documentation.  
- (i.e., there are 3 different CRCs, 3 different AES implementations, all detailed)  
- This allows those with little knowledge of C/CPP to understand the sequence of each command.  
- All these details require you to carefully read the work of @Velocet to understand this protocol.  
- There is no magic documentation; only personal work to adapt to your own needs.  
- But all the functional basics for 1W and 2W are there.  
- All AES implementations are included, without using an external library.  
- All 1W / 2W commands for pairing/associating are included as well.  

---

### Usage
Use it like a scanner, perform some commands on your real device to identify the corresponding CMDid and address.  

→ Choose your board before build.  

---

### Brief explanation[^1]:
Uses 2 interrupts:  
- if PAYLOAD and RX → decode the frame [^3]  
- if PAYLOAD and TX_READY → send the frame, decode it [^3]  

---

### platformio[^2] :
_First time or when a JSON in data folder is modified:_  
1. build filesystem image  
2. upload filesystem image  

_After any other changes:_  
- upload and monitor  
- make sure `CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD` remains enabled in `sdkconfig` so ESP timers can run callbacks from ISR context  

[^1]: I use an SX1276. If CC1101/SX1262: Feel free to use the old code (not checked/guaranteed).  
[^2]: I use Visual Studio Code Insider.  
[^3]: Decoding can be verbose (RSSI, Timing, …).  

---

## Web Interface (Experimental)

This project now includes an experimental web interface to control IOHC devices.

### Setup & Access
1. **Configure WiFi** – on first boot the device starts an access point named `iohc-setup`.  
   Connect and follow the WiFiManager portal to enter your WiFi credentials.  
2. **Build and Upload Filesystem** – upload web files in `extras/web_interface_data/` via LittleFS.  
3. **Build and Upload Firmware** – flash main firmware with PlatformIO.  
4. **Find ESP32 IP** – check Serial Monitor for IP.  
5. **Access the Interface** – open browser at IP or `http://miopenio.local` if mDNS is supported.  

---

## Home Assistant Discovery

When MQTT is enabled (`#define MQTT` in `include/user_config.h`), the firmware publishes HA discovery messages for every blind.  

Each blind publishes config to:  
```
homeassistant/cover/<id>/config
```

It supports `travel_time`, pairing state, and sequence numbers.  

Example payload:  
```json
{"name":"IZY1","command_topic":"iown/B60D1A/set","state_topic":"iown/B60D1A/state","position_topic":"iown/B60D1A/position","unique_id":"B60D1A","payload_open":"OPEN","payload_close":"CLOSE","payload_stop":"STOP","device_class":"blind","availability_topic":"iown/status"}
```

---

#### **License**
