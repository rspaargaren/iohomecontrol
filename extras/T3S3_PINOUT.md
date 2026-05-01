LilyGo T3S3 notes

This project now includes a dedicated `LilyGoT3S3` PlatformIO environment.

Pins used for the T3S3 environment:

- OLED I2C SDA: GPIO18
- OLED I2C SCL: GPIO17
- LoRa SCK: GPIO5
- LoRa MISO: GPIO3
- LoRa MOSI: GPIO6
- LoRa CS/NSS: GPIO7
- LoRa RESET: GPIO8
- LoRa DIO0: GPIO9
- LoRa DIO1: GPIO33
- LoRa DIO2: GPIO34
- LED: GPIO37

These values were taken from the official LilyGo T3S3 pin overview for the SX1276/SX1278 variants:
- https://wiki.lilygo.cc/get_started/en/LoRa_GPS/T3S3/T3S3.html

If the board has no OLED fitted, disable `SSD1306_DISPLAY` in `include/user_config.h`.
