#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "DEV"
#endif

inline constexpr const char* firmwareVersion() {
    return FIRMWARE_VERSION;
}

#endif // FIRMWARE_VERSION_H
