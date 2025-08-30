#include <syslog_helper.h>

#if defined(SYSLOG)

#include <WiFiUdp.h>
#include <user_config.h>

namespace {
    WiFiUDP syslogUdp;
    IPAddress syslogIP;
    bool syslogReady = false;
}

void initSyslog() {
    if (syslogReady) {
        return;
    }
    if (syslog_server.empty()) {
        return;
    }
    if (!syslogIP.fromString(syslog_server.c_str())) {
        return;
    }
    syslogUdp.begin(0);
    syslogReady = true;
}

void sendSyslog(const String &msg) {
    if (!syslogReady) {
        initSyslog();
    }
    if (!syslogReady) {
        return;
    }
    syslogUdp.beginPacket(syslogIP, syslog_port);
    syslogUdp.write(msg.c_str());
    syslogUdp.endPacket();
}

#endif // SYSLOG

