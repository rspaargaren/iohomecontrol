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

#### **License**
