#include <syslog_helper.h>

#if defined(SYSLOG)

#include <WiFiUdp.h>
#include <user_config.h>
#include <esp_log.h>

namespace {
    WiFiUDP syslogUdp;
    IPAddress syslogIP;
    bool syslogReady = false;
    static const char *TAG = "syslog_helper";
}

void initSyslog() {
    ESP_LOGD(TAG, "Init syslog: server='%s' port=%u", syslog_server.c_str(), syslog_port);
    if (syslogReady) {
        ESP_LOGD(TAG, "Syslog already initialized");
        return;
    }
    if (syslog_server.empty()) {
        ESP_LOGD(TAG, "Syslog server not set");
        return;
    }
    if (!syslogIP.fromString(syslog_server.c_str())) {
        ESP_LOGD(TAG, "Invalid syslog server address: %s", syslog_server.c_str());
        return;
    }
    syslogUdp.begin(0);
    syslogReady = true;
    ESP_LOGD(TAG, "Syslog initialized with IP: %s", syslogIP.toString().c_str());
}

void sendSyslog(const String &msg) {
    if (!syslogReady) {
        ESP_LOGD(TAG, "Syslog not ready, initializing");
        initSyslog();
    }
    if (!syslogReady) {
        ESP_LOGD(TAG, "Syslog initialization failed");
        return;
    }
    ESP_LOGD(TAG, "Sending syslog message: %s", msg.c_str());
    syslogUdp.beginPacket(syslogIP, syslog_port);
    syslogUdp.write(msg.c_str());
    int result = syslogUdp.endPacket();
    ESP_LOGD(TAG, "Message send result: %d", result);
}

#endif // SYSLOG

